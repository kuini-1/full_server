#!/usr/bin/env python3
"""
One-shot generated wire layer: parse layout dumps (clang -fdump-record-layouts),
compute copy ops for each packet struct, emit PacketWireLayout.generated.cpp
with one table and one encode/decode loop for all opcodes.

RULE: This script must never contain packet-specific code. No struct names
(sAU_LOGIN_RES, etc.), no opcodes (1002, etc.), no per-packet branches or tables.
All copy ops are derived only from layout dumps (wire_layout.txt, our_layout.txt)
and generic rules (region count, sizes, gcd for arrays, two-region fixed-tail rule).
Adding a new packet = add it to the layout dumps and re-run; no edits here.

VALIDATION: For every rule, ask: "If I use this exact code for another packet
without looking at its structure, would the wire hex be correct?" If a rule
assumes a layout (e.g. "last 2 bytes of fixed = same logical field on both sides"),
then any packet that does not match that layout can produce a hex mismatch.
The two-region fixed-tail rule assumes: exactly 2 regions, fixed part ends with
2 bytes that are the same logical field on wire and our; extra our bytes are
padding at the end of the fixed part. New packets that hit this path must be
verified (wire hex compare) until we have member-level layout or a safer rule.
"""

import re
import sys
import os
import subprocess
import argparse
from pathlib import Path

# Project root (parent of tools/)
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent
NTL_SHARED = PROJECT_ROOT / "Server" / "NtlShared2"
WIRE_DIR = PROJECT_ROOT / "wire"
NETWORK_DIR = PROJECT_ROOT / "Server" / "NtlNetwork"

# Include paths matching CMake dbo_includes (for --dump-stub preprocessing)
DUMP_INCLUDE_DIRS = [
    PROJECT_ROOT / "Shared",
    PROJECT_ROOT / "Shared/Util",
    PROJECT_ROOT / "Shared/Lua",
    PROJECT_ROOT / "Shared/Lua/lua/src",
    PROJECT_ROOT / "Shared/NtlTrigger",
    PROJECT_ROOT / "Shared/NtlXMLLoader",
    PROJECT_ROOT / "Shared/NtlNavi",
    PROJECT_ROOT / "Shared/NtlNavi/Source",
    PROJECT_ROOT / "Shared/Zip/zlib123",
    PROJECT_ROOT / "Server/NtlSystem",
    PROJECT_ROOT / "Server/NtlThread",
    PROJECT_ROOT / "Server/NtlNetwork",
    PROJECT_ROOT / "Server/NtlSfx",
    PROJECT_ROOT / "Server/NtlShared2",
    PROJECT_ROOT / "Server/NtlGameTable",
    PROJECT_ROOT / "Server/Database",
    PROJECT_ROOT / "Server/servercommon",
    PROJECT_ROOT / "Server/DboTrigger",
    PROJECT_ROOT / "Server/SCSCaptcha",
]


def extract_protocols_from_headers():
    """Extract (protocol_name, struct_name) and opcode value from NtlPacket*.h."""
    structs = []  # (struct_name, opcode_value)
    # Opcode ranges per file (from NtlPacketAll.h comment)
    opcode_base = {
        "NtlPacketAU.h": 1000,
        "NtlPacketUA.h": 100,
        "NtlPacketUC.h": 2000,
        "NtlPacketCU.h": 3000,
        "NtlPacketUG.h": 4000,
        "NtlPacketGU.h": 5000,
        "NtlPacketUT.h": 6000,
        "NtlPacketTU.h": 7000,
        "NtlPacketMA.h": 8000,
        "NtlPacketAM.h": 9000,
        "NtlPacketMC.h": 10000,
        "NtlPacketCM.h": 11000,
        "NtlPacketMG.h": 12000,
        "NtlPacketGM.h": 13000,
        "NtlPacketNG.h": 37000,
        "NtlPacketGN.h": 36000,
        "NtlPacketQG.h": 39000,
        "NtlPacketGQ.h": 38000,
        "NtlPacketGT.h": 40000,
        "NtlPacketTG.h": 41000,
        "NtlPacketMW.h": 16000,
        "NtlPacketWM.h": 17000,
        "NtlPacketMT.h": 18000,
        "NtlPacketTM.h": 19000,
        "NtlPacketTL.h": 50000,
        "NtlPacketTQ.h": 57000,
        "NtlPacketQT.h": 56000,
        "NtlPacketXR.h": 59000,
        "NtlPacketQM.h": 33000,
        "NtlPacketMQ.h": 32000,
        "NtlPacketQC.h": 33000,
        "NtlPacketCQ.h": 32000,
    }
    enum_counter = {}  # file -> next value
    for h in sorted(NTL_SHARED.glob("NtlPacket*.h")):
        if h.name in ("NtlPacketCommon.h", "NtlPacketAll.h", "NtlPacketUtil.h"):
            continue
        base = opcode_base.get(h.name)
        if base is None:
            base = 0
        # Enum typically has OPCODE_BEGIN=base, first opcode=base+1, second=base+2, ...
        counter = base + 1
        content = h.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r"BEGIN_PROTOCOL\s*\(\s*([A-Za-z0-9_]+)\s*\)", content):
            name = m.group(1)
            if name.startswith("//"):
                continue
            struct_name = "s" + name
            structs.append((struct_name, counter))
            counter += 1
    return structs


def parse_clang_layout_dump(text):
    """
    Parse clang -fdump-record-layouts output.
    Returns dict: struct_name -> (total_size, [(offset, size), ...])
    """
    result = {}
    current = None
    offsets = []
    total_size = 0
    for line in text.splitlines():
        if "*** Dumping AST Record Layout" in line:
            if current and offsets is not None:
                # compute sizes from offsets
                regions = []
                for i, off in enumerate(offsets):
                    if i + 1 < len(offsets):
                        sz = offsets[i + 1] - off
                    else:
                        sz = total_size - off
                    if sz > 0:
                        regions.append((off, sz))
                result[current] = (total_size, regions)
            current = None
            offsets = []
            total_size = 0
            continue
        m = re.match(r"\s*(\d+)\s+\|\s+struct\s+(\w+)", line)
        if m and "(base)" not in line:
            if current and offsets is not None:
                regions = []
                for i, off in enumerate(offsets):
                    if i + 1 < len(offsets):
                        sz = offsets[i + 1] - off
                    else:
                        sz = total_size - off
                    if sz > 0:
                        regions.append((off, sz))
                result[current] = (total_size, regions)
            current = "s" + m.group(2) if not m.group(2).startswith("s") else m.group(2)
            offsets = [int(m.group(1))]
            total_size = 0
            continue
        m = re.match(r"\s*(\d+)\s+\|\s+(?:struct\s+\w+\s+\(base\)|[\w:]+)", line)
        if m and current:
            offsets.append(int(m.group(1)))
        m = re.search(r"sizeof=(\d+)", line)
        if m and current:
            total_size = int(m.group(1))
    if current and offsets and total_size:
        regions = []
        for i, off in enumerate(offsets):
            if i + 1 < len(offsets):
                sz = offsets[i + 1] - off
            else:
                sz = total_size - off
            if sz > 0:
                regions.append((off, sz))
        result[current] = (total_size, regions)
    return result


def gcd(a, b):
    while b:
        a, b = b, a % b
    return a


def compute_copy_ops(wire_size, wire_regions, our_size, our_regions):
    """
    Compute list of (wire_off, our_off, len) for encode (our -> wire).
    Regions are sorted by offset; we match 1:1 by order. If region sizes differ,
    treat as array and use gcd to get element count.
    When there are exactly two regions and the fixed (first) region sizes differ,
    use a generic rule: copy common prefix (wire_fixed - 2), then 2-byte tail
    (last 2 bytes of fixed region, e.g. flags/count before array) from our_fixed-2
    to wire_fixed-2, then array. Applies to any struct with two regions; no packet names.
    """
    if not wire_regions and not our_regions:
        if wire_size == our_size and wire_size > 0:
            return [(0, 0, wire_size)]
        return []
    if len(wire_regions) != len(our_regions):
        if wire_size == our_size:
            return [(0, 0, wire_size)]
        return []
    # Two regions with differing fixed-part size: split fixed into prefix + 2-byte tail.
    # Assumption: last 2 bytes of fixed region are the same logical field on wire and our
    # (e.g. count/flag before array). If another packet has 1-byte tail, or padding in the
    # middle, this would produce wrong hex—verify wire for any new packet using this path.
    if len(wire_regions) == 2 and len(our_regions) == 2:
        w_off0, w_sz0 = wire_regions[0]
        o_off0, o_sz0 = our_regions[0]
        w_off1, w_sz1 = wire_regions[1]
        o_off1, o_sz1 = our_regions[1]
        if w_sz0 != o_sz0 and w_sz0 >= 2 and o_sz0 >= 2:
            prefix_len = w_sz0 - 2
            tail_wire_off = w_off0 + prefix_len
            tail_our_off = o_off0 + o_sz0 - 2
            ops = []
            if prefix_len > 0:
                ops.append((w_off0, o_off0, prefix_len))
            ops.append((tail_wire_off, tail_our_off, 2))
            # Array region
            n = gcd(w_sz1, o_sz1)
            w_el = w_sz1 // n
            o_el = o_sz1 // n
            copy_len = min(w_el, o_el)
            for i in range(n):
                ops.append((w_off1 + i * w_el, o_off1 + i * o_el, copy_len))
            return ops
    ops = []
    for (w_off, w_sz), (o_off, o_sz) in zip(wire_regions, our_regions):
        if w_sz == o_sz:
            ops.append((w_off, o_off, w_sz))
        else:
            n = gcd(w_sz, o_sz)
            w_el = w_sz // n
            o_el = o_sz // n
            copy_len = min(w_el, o_el)
            for i in range(n):
                ops.append((w_off + i * w_el, o_off + i * o_el, copy_len))
    return ops


def generate_cpp(structs_with_ops, opcode_to_struct):
    """Emit PacketWireLayout.generated.cpp with one table and encode/decode loops."""
    lines = []
    lines.append("// Generated by tools/gen_packet_wire.py - do not edit")
    lines.append("#if !defined(_WIN32)")
    lines.append("")
    lines.append('#include "stdafx.h"')
    lines.append('#include "PacketWireLayout.h"')
    lines.append('#include "NtlSharedCommon.h"')
    lines.append("")
    # Table: opcode -> wire_size, our_size, n_ops, ops[]
    # We'll use a flat array and a separate index for opcode -> table row
    lines.append("static const unsigned int PACKET_WIRE_MAX_OPS = 32;")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("  unsigned int wire_size;")
    lines.append("  unsigned int our_size;")
    lines.append("  unsigned int n_ops;")
    lines.append("  unsigned int wire_off[32];")
    lines.append("  unsigned int our_off[32];")
    lines.append("  unsigned int len[32];")
    lines.append("} packet_wire_row_t;")
    lines.append("")
    # Build rows for each opcode that has conversion; opcodes without conversion use wire_size=0
    rows = []  # list of (opcode, row)
    for opcode, struct_name in sorted(opcode_to_struct.items(), key=lambda x: x[0]):
        if struct_name not in structs_with_ops:
            continue
        wire_size, our_size, ops = structs_with_ops[struct_name]
        if not ops:
            continue
        n_ops = min(len(ops), 32)
        rows.append((opcode, wire_size, our_size, ops[:n_ops]))
    # Sort by opcode for binary search or use a sparse table
    lines.append("static const packet_wire_row_t PACKET_WIRE_TABLE[] = {")
    for opcode, wire_size, our_size, ops in rows:
        w_offs = ", ".join(str(o[0]) for o in ops) + ", " + ", ".join(["0"] * (32 - len(ops)))
        o_offs = ", ".join(str(o[1]) for o in ops) + ", " + ", ".join(["0"] * (32 - len(ops)))
        lens = ", ".join(str(o[2]) for o in ops) + ", " + ", ".join(["0"] * (32 - len(ops)))
        lines.append("  { %u, %u, %u, { %s }, { %s }, { %s } }," % (wire_size, our_size, len(ops), w_offs, o_offs, lens))
    lines.append("};")
    lines.append("")
    lines.append("static const unsigned int PACKET_WIRE_TABLE_OPCODES[] = {")
    lines.append("  " + ", ".join(str(opcode) for opcode, _, _, _ in rows))
    lines.append("};")
    lines.append("static const unsigned int PACKET_WIRE_TABLE_COUNT = %u;" % len(rows))
    lines.append("")
    lines.append("static const packet_wire_row_t* find_row(unsigned int wOpCode) {")
    lines.append("  for (unsigned int i = 0; i < PACKET_WIRE_TABLE_COUNT; i++) {")
    lines.append("    if (PACKET_WIRE_TABLE_OPCODES[i] == wOpCode)")
    lines.append("      return &PACKET_WIRE_TABLE[i];")
    lines.append("  }")
    lines.append("  return NULL;")
    lines.append("}")
    lines.append("")
    lines.append("unsigned int PacketWire_GetWirePayloadSize(unsigned int wOpCode) {")
    lines.append("  const packet_wire_row_t* r = find_row(wOpCode);")
    lines.append("  return r ? r->wire_size : 0;")
    lines.append("}")
    lines.append("")
    lines.append("unsigned int PacketWire_GetOurPayloadSize(unsigned int wOpCode) {")
    lines.append("  const packet_wire_row_t* r = find_row(wOpCode);")
    lines.append("  return r ? r->our_size : 0;")
    lines.append("}")
    lines.append("")
    lines.append("unsigned int PacketWire_EncodePayload(unsigned int wOpCode,")
    lines.append("  const unsigned char* pOurPayload, unsigned int ourPayloadSize,")
    lines.append("  unsigned char* pWirePayload, unsigned int wirePayloadSize) {")
    lines.append("  const packet_wire_row_t* r = find_row(wOpCode);")
    lines.append("  if (!r || r->wire_size > wirePayloadSize || r->our_size > ourPayloadSize) return 0;")
    lines.append("  for (unsigned int i = 0; i < r->n_ops; i++) {")
    lines.append("    CopyMemory(pWirePayload + r->wire_off[i], pOurPayload + r->our_off[i], r->len[i]);")
    lines.append("  }")
    lines.append("  return r->wire_size;")
    lines.append("}")
    lines.append("")
    lines.append("unsigned int PacketWire_DecodePayload(unsigned int wOpCode,")
    lines.append("  const unsigned char* pWirePayload, unsigned int wirePayloadSize,")
    lines.append("  unsigned char* pOurPayload, unsigned int ourPayloadSize) {")
    lines.append("  const packet_wire_row_t* r = find_row(wOpCode);")
    lines.append("  if (!r || r->wire_size > wirePayloadSize || r->our_size > ourPayloadSize) return 0;")
    lines.append("  ZeroMemory(pOurPayload, r->our_size);")
    lines.append("  for (unsigned int i = 0; i < r->n_ops; i++) {")
    lines.append("    CopyMemory(pOurPayload + r->our_off[i], pWirePayload + r->wire_off[i], r->len[i]);")
    lines.append("  }")
    lines.append("  return r->our_size;")
    lines.append("}")
    lines.append("")
    lines.append("#else")
    lines.append("// Windows: pass-through")
    lines.append("unsigned int PacketWire_GetWirePayloadSize(unsigned int wOpCode) { (void)wOpCode; return 0; }")
    lines.append("unsigned int PacketWire_GetOurPayloadSize(unsigned int wOpCode) { (void)wOpCode; return 0; }")
    lines.append("unsigned int PacketWire_EncodePayload(unsigned int wOpCode,")
    lines.append("  const unsigned char* pOurPayload, unsigned int ourPayloadSize,")
    lines.append("  unsigned char* pWirePayload, unsigned int wirePayloadSize) {")
    lines.append("  (void)wOpCode; (void)ourPayloadSize;")
    lines.append("  if (pOurPayload && pWirePayload && wirePayloadSize >= ourPayloadSize) {")
    lines.append("    CopyMemory(pWirePayload, pOurPayload, ourPayloadSize);")
    lines.append("    return ourPayloadSize;")
    lines.append("  }")
    lines.append("  return 0;")
    lines.append("}")
    lines.append("unsigned int PacketWire_DecodePayload(unsigned int wOpCode,")
    lines.append("  const unsigned char* pWirePayload, unsigned int wirePayloadSize,")
    lines.append("  unsigned char* pOurPayload, unsigned int ourPayloadSize) {")
    lines.append("  (void)wOpCode; (void)wirePayloadSize;")
    lines.append("  if (pWirePayload && pOurPayload && ourPayloadSize >= wirePayloadSize) {")
    lines.append("    CopyMemory(pOurPayload, pWirePayload, wirePayloadSize);")
    lines.append("    return wirePayloadSize;")
    lines.append("  }")
    lines.append("  return 0;")
    lines.append("}")
    lines.append("#endif")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser(description="Generate PacketWireLayout from layout dumps")
    ap.add_argument("--wire", default=str(WIRE_DIR / "wire_layout.txt"), help="Wire (Windows) layout dump")
    ap.add_argument("--our", default=str(WIRE_DIR / "our_layout.txt"), help="Our (Linux) layout dump")
    ap.add_argument("-o", "--output", default=str(NETWORK_DIR / "PacketWireLayout.generated.cpp"), help="Output C++ file")
    ap.add_argument("--dump-stub", action="store_true", help="Generate layout dump stub and run clang to produce our_layout.txt")
    args = ap.parse_args()

    structs = extract_protocols_from_headers()
    opcode_to_struct = {opcode: name for name, opcode in structs}

    if args.dump_stub:
        # Generate stub that includes all packet headers and one global per struct to force layout dump
        WIRE_DIR.mkdir(parents=True, exist_ok=True)
        stub_path = WIRE_DIR / "PacketLayoutDump.cpp"
        includes = '#include "NtlSharedCommon.h"\n#include "NtlPacketAll.h"\n#include "NtlPacketWM.h"\n'
        # One global per struct so clang dumps each layout (global triggers dump)
        globals_list = "\n".join("char _ref_%s[sizeof(%s)];" % (n.replace("s", ""), n) for n, _ in structs)
        stub_content = '''// Generated stub for -fdump-record-layouts
%s
%s
''' % (includes, globals_list)
        stub_path.write_text(stub_content, encoding="utf-8")
        # Preprocess with full driver so system and project includes resolve
        pp_path = WIRE_DIR / "PacketLayoutDump.i"
        include_args = []
        for d in DUMP_INCLUDE_DIRS:
            if d.exists():
                include_args.append("-I" + str(d))
        cmd_pp = ["clang++", "-E", "-x", "c++"] + include_args + ["-o", str(pp_path), str(stub_path)]
        try:
            out_pp = subprocess.run(cmd_pp, cwd=str(PROJECT_ROOT), capture_output=True, text=True, timeout=120)
            if out_pp.returncode != 0:
                sys.stderr.write(out_pp.stderr or out_pp.stdout or "Preprocess failed\n")
                sys.exit(1)
            # Run -cc1 -fdump-record-layouts on preprocessed file (no system includes needed)
            cmd = ["clang++", "-cc1", "-fdump-record-layouts", "-x", "c++"] + include_args + [str(pp_path)]
            out = subprocess.run(cmd, cwd=str(PROJECT_ROOT), capture_output=True, text=True, timeout=120)
            sys.stderr.write(out.stderr)
            if out.returncode == 0:
                our_text = out.stdout + out.stderr
                (WIRE_DIR / "our_layout.txt").write_text(our_text, encoding="utf-8")
                sys.stderr.write("Wrote %s (full layout dump)\n" % (WIRE_DIR / "our_layout.txt"))
            else:
                sys.stderr.write("Layout dump failed (exit %d); our_layout.txt not overwritten.\n" % out.returncode)
                sys.exit(1)
        except Exception as e:
            sys.stderr.write("Layout dump failed: %s\n" % e)
            sys.exit(1)
        return

    wire_path = Path(args.wire)
    our_path = Path(args.our)
    if not wire_path.exists():
        sys.stderr.write("Wire layout file not found: %s\n" % wire_path)
        sys.exit(1)
    if not our_path.exists():
        sys.stderr.write("Our layout file not found: %s\n" % our_path)
        sys.exit(1)

    wire_layout = parse_clang_layout_dump(wire_path.read_text(encoding="utf-8", errors="replace"))
    our_layout = parse_clang_layout_dump(our_path.read_text(encoding="utf-8", errors="replace"))

    struct_names_with_opcodes = set(opcode_to_struct.values())
    structs_with_ops = {}
    for struct_name in wire_layout.keys():
        if struct_name not in our_layout:
            continue
        w_size, w_regions = wire_layout[struct_name]
        o_size, o_regions = our_layout[struct_name]
        ops = compute_copy_ops(w_size, w_regions, o_size, o_regions)
        if ops and struct_name in struct_names_with_opcodes:
            structs_with_ops[struct_name] = (w_size, o_size, ops)

    cpp = generate_cpp(structs_with_ops, opcode_to_struct)
    Path(args.output).write_text(cpp, encoding="utf-8")
    print("Wrote %s (%u structs with conversion)" % (args.output, len(structs_with_ops)))


if __name__ == "__main__":
    main()
