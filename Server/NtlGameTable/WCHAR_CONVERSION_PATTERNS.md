# WCHAR/wchar_t Conversion Patterns Reference

This document provides a reference for all conversion patterns needed when fixing table files in `Server/NtlGameTable/`.

## Pattern 1: Static String Literal Declaration

**Location:** Top of file, before `m_pwszSheetList` declaration

**Before:**
```cpp
const WCHAR* CXXXTable::m_pwszSheetList[] =
{
    L"Table_Data_KOR",
    NULL
};
```

**After:**
```cpp
// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
    WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CXXXTable::m_pwszSheetList[] =
{
    g_wszTableDataKOR,
    NULL
};
```

**Note:** If file has multiple sheet names, create separate static arrays:
- `g_wszTableDataKOR[]`
- `g_wszTableDataENG[]` (if needed)
- etc.

---

## Pattern 2: Sheet Name Comparisons (3 locations per file)

**Locations:** `AllocNewTable()`, `DeallocNewTable()`, `SetTableData()`

**Before:**
```cpp
if (0 == wcscmp(pwszSheetName, L"Table_Data_KOR"))
```

**After:**
```cpp
if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))
```

**All 3 locations must be fixed in each file.**

---

## Pattern 3: Field Name Comparisons

**Location:** Inside `SetTableData()`, when comparing `pstrDataName` with string literals

**Before:**
```cpp
if (0 == wcscmp(pstrDataName->c_str(), L"Tblidx"))
else if (0 == wcscmp(pstrDataName->c_str(), L"Validity_Able"))
```

**After:**
```cpp
if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))
else if (0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able"))
```

**Note:** This pattern applies to ALL `wcscmp(pstrDataName->c_str(), L"...")` comparisons.

---

## Pattern 4: Field Name Parameter Conversion

**Location:** When passing `pstrDataName->c_str()` to functions expecting `const WCHAR*`

**Functions affected:**
- `READ_BOOL(bstrData, pstrDataName->c_str())`
- `READ_BYTE(bstrData, pstrDataName->c_str())`
- `READ_WORD(bstrData, pstrDataName->c_str())`
- `READ_FLOAT(bstrData, pstrDataName->c_str())`
- `READ_STRINGW(bstrData, ..., pstrDataName->c_str())`

**Before:**
```cpp
pAction->bValidity_Able = READ_BOOL(bstrData, pstrDataName->c_str());
pAction->byAction_Type = READ_BYTE(bstrData, pstrDataName->c_str());
```

**After:**
```cpp
WCHAR wszFieldNameBuf[256];
WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
pAction->bValidity_Able = READ_BOOL(bstrData, wszFieldNameBuf);
pAction->byAction_Type = READ_BYTE(bstrData, wszFieldNameBuf);
```

**Note:** 
- Declare `wszFieldNameBuf` once per `if/else if` block that needs it
- Reuse the same buffer for multiple READ_* calls in the same block
- Only convert when the function actually needs `const WCHAR*` parameter

---

## Pattern 5: Format String in CallErrorCallbackFunction

**Location:** `AddTable()` and `SetTableData()` methods

**Before:**
```cpp
CTable::CallErrorCallbackFunction(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", m_wszXmlFileName, pTbldat->tblidx);
CTable::CallErrorCallbackFunction(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", m_wszXmlFileName, pstrDataName->c_str());
```

**After:**
```cpp
WCHAR wszFormatBuf[512];
FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx);

// For format strings with pstrDataName->c_str() as parameter:
WCHAR wszFormatBuf[512];
FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));
WCHAR wszFieldNameBuf[256];
WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);
```

**Note:** Every `CallErrorCallbackFunction` call with `L"..."` format string needs conversion.

---

## Pattern 6: Other String Functions

### wcsncmp → WCHARNCmp

**Before:**
```cpp
if (0 == wcsncmp(pstrDataName->c_str(), L"Chat_Command_", wcslen(L"Chat_Command_")))
```

**After:**
```cpp
static const WCHAR g_wszChatCommand[] = { 'C', 'h', 'a', 't', '_', 'C', 'o', 'm', 'm', 'a', 'n', 'd', '_', 0 };
WCHAR wszFieldNameBuf[256];
WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
if (0 == WCHARNCmp(wszFieldNameBuf, g_wszChatCommand, WCHARLen(g_wszChatCommand)))
```

### swprintf → NTL_SWPRINTF

**Before:**
```cpp
WCHAR szBuffer[1024] = { 0x00, };
swprintf(szBuffer, 1024, L"Chat_Command_%d", i + 1);
```

**After:**
```cpp
WCHAR szBuffer[1024] = { 0x00, };
NTL_SWPRINTF(szBuffer, 1024, L"Chat_Command_%d", i + 1);
```

### wcslen → WCHARLen (when used with WCHAR*)

**Before:**
```cpp
wcslen(pwszSheetName)
```

**After:**
```cpp
WCHARLen(pwszSheetName)
```

---

## Pattern 7: CheckNegativeInvalid

**Important:** `CheckNegativeInvalid` expects `const wchar_t*`, so `pstrDataName->c_str()` can be used **directly** (no conversion needed).

**Correct:**
```cpp
CheckNegativeInvalid(pstrDataName->c_str(), bstrData);
```

**Do NOT convert this** - it already returns `const wchar_t*` which is what the function expects.

---

## Complete Example: Simple File Pattern

Here's a complete example showing all patterns in a typical simple file:

```cpp
// At top of file:
// Static WCHAR arrays for string literals
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };

// Helper function to convert format string literal to WCHAR* buffer
static inline void FormatStringToWCHAR(const wchar_t* fmt, WCHAR* dest, size_t destSize) {
    WCharTLiteralToWCHAR(fmt, dest, destSize);
}

const WCHAR* CXXXTable::m_pwszSheetList[] =
{
    g_wszTableDataKOR,
    NULL
};

// In AllocNewTable():
void* CXXXTable::AllocNewTable(WCHAR* pwszSheetName, DWORD dwCodePage)
{
    if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))  // Pattern 2
    {
        // ... implementation
    }
    return NULL;
}

// In DeallocNewTable():
bool CXXXTable::DeallocNewTable(void* pvTable, WCHAR* pwszSheetName)
{
    if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))  // Pattern 2
    {
        // ... implementation
    }
    return false;
}

// In AddTable():
bool CXXXTable::AddTable(void * pvTable, bool bReload, bool bUpdate)
{
    // ... other code ...
    
    if( false == m_mapTableList.insert(...).second )
    {
        WCHAR wszFormatBuf[512];
        FormatStringToWCHAR(L"[File] : %s\r\n Table Tblidx[%u] is Duplicated ", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));  // Pattern 5
        CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, pTbldat->tblidx);
        return false;
    }
    return true;
}

// In SetTableData():
bool CXXXTable::SetTableData(void* pvTable, WCHAR* pwszSheetName, std::wstring* pstrDataName, BSTR bstrData)
{
    if (0 == WCHARCmp(pwszSheetName, g_wszTableDataKOR))  // Pattern 2
    {
        sXXX_TBLDAT* pData = (sXXX_TBLDAT*)pvTable;

        if (0 == WStringCmpLiteral(*pstrDataName, L"Tblidx"))  // Pattern 3
        {
            CheckNegativeInvalid(pstrDataName->c_str(), bstrData);  // Pattern 7 - no conversion needed
            pData->tblidx = READ_DWORD(bstrData);
        }
        else if (0 == WStringCmpLiteral(*pstrDataName, L"Validity_Able"))  // Pattern 3
        {
            CheckNegativeInvalid(pstrDataName->c_str(), bstrData);  // Pattern 7
            WCHAR wszFieldNameBuf[256];
            WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));  // Pattern 4
            pData->bValidity_Able = READ_BOOL(bstrData, wszFieldNameBuf);  // Pattern 4
        }
        else
        {
            WCHAR wszFormatBuf[512];
            FormatStringToWCHAR(L"[File] : %s\n[Error] : Unknown field name found!(Field Name = %s)", wszFormatBuf, sizeof(wszFormatBuf)/sizeof(WCHAR));  // Pattern 5
            WCHAR wszFieldNameBuf[256];
            WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));  // Pattern 4
            CTable::CallErrorCallbackFunction(wszFormatBuf, m_wszXmlFileName, wszFieldNameBuf);  // Pattern 5
            return false;
        }
    }
    return true;
}
```

---

## Edge Cases

### Multiple Sheet Names
If a file has multiple sheet names, create separate static arrays:
```cpp
static const WCHAR g_wszTableDataKOR[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'K', 'O', 'R', 0 };
static const WCHAR g_wszTableDataENG[] = { 'T', 'a', 'b', 'l', 'e', '_', 'D', 'a', 't', 'a', '_', 'E', 'N', 'G', 0 };

const WCHAR* CXXXTable::m_pwszSheetList[] =
{
    g_wszTableDataKOR,
    g_wszTableDataENG,
    NULL
};
```

### Buffer Reuse
In `SetTableData()`, reuse the same buffer when possible:
```cpp
WCHAR wszFieldNameBuf[256];  // Declare once at start of if block
WStringCStrToWCHAR(*pstrDataName, wszFieldNameBuf, sizeof(wszFieldNameBuf)/sizeof(WCHAR));
// Use wszFieldNameBuf for multiple READ_* calls in the same block
```

---

## Verification Checklist

After fixing each file, verify:
- [ ] Static WCHAR array `g_wszTableDataKOR` declared
- [ ] `FormatStringToWCHAR` helper function added
- [ ] All 3 `wcscmp(pwszSheetName, L"Table_Data_KOR")` replaced with `WCHARCmp`
- [ ] All `wcscmp(pstrDataName->c_str(), L"...")` replaced with `WStringCmpLiteral`
- [ ] All `pstrDataName->c_str()` in READ_* functions converted with `WStringCStrToWCHAR`
- [ ] All `CallErrorCallbackFunction(L"...", ...)` format strings converted
- [ ] Any `wcsncmp` replaced with `WCHARNCmp`
- [ ] Any `swprintf` replaced with `NTL_SWPRINTF`
- [ ] `CheckNegativeInvalid` calls use `pstrDataName->c_str()` directly (no conversion)
