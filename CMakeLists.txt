cmake_minimum_required(VERSION 3.15)

# Linux: set compiler before project(). Prefer GCC 9 when available to test packet layout.
if(UNIX AND NOT APPLE)
  if(DBO_USE_CLANG)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
  else()
    find_program(DBO_GCC_9_CXX NAMES g++-9 DOC "GCC 9 C++ compiler")
    find_program(DBO_GCC_9_C NAMES gcc-9 DOC "GCC 9 C compiler")
    if(DBO_GCC_9_CXX AND DBO_GCC_9_C)
      set(CMAKE_C_COMPILER "${DBO_GCC_9_C}")
      set(CMAKE_CXX_COMPILER "${DBO_GCC_9_CXX}")
      set(DBO_LINUX_GCC_9 ON CACHE INTERNAL "Using GCC 9 for Linux build")
    else()
      set(CMAKE_C_COMPILER gcc)
      set(CMAKE_CXX_COMPILER g++)
      set(DBO_LINUX_GCC_9 OFF CACHE INTERNAL "Using default g++ for Linux build")
    endif()
  endif()
endif()

project(DboServerFiles LANGUAGES C CXX)

# Enforce Linux-only configuration.
if(WIN32)
  message(FATAL_ERROR "Windows builds are not supported. Configure and build this project on WSL/Ubuntu (Linux).")
endif()

# Use the directory of this file so Linux builds see the actual project root.
set(DBO_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "DBO_PROJECT_ROOT=${DBO_PROJECT_ROOT}")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/third_party.cmake")
include(ExternalProject)

function(dbo_collect_sources out_var base_dir)
  file(GLOB_RECURSE _dbo_src
    "${base_dir}/*.c"
    "${base_dir}/*.cc"
    "${base_dir}/*.cpp"
    "${base_dir}/*.cxx"
  )

  # Exclude build output and other non-source trees.
  list(FILTER _dbo_src EXCLUDE REGEX "/(Debug|Release|ReleaseX64|x64|out|build|build_win|ipch|tlog|node_modules)/")

  # Exclude MSVC precompiled-header translation unit name
  list(FILTER _dbo_src EXCLUDE REGEX ".*[/\\\\]stdafx\\.cpp$")

  set(${out_var} ${_dbo_src} PARENT_SCOPE)
endfunction()

add_library(dbo_includes INTERFACE)
target_include_directories(dbo_includes INTERFACE
  "${DBO_PROJECT_ROOT}/Shared"
  "${DBO_PROJECT_ROOT}/Shared/Util"
  "${DBO_PROJECT_ROOT}/Shared/Lua"
  "${DBO_PROJECT_ROOT}/Shared/Lua/lua/src"
  "${DBO_PROJECT_ROOT}/Shared/NtlTrigger"
  "${DBO_PROJECT_ROOT}/Shared/NtlXMLLoader"
  "${DBO_PROJECT_ROOT}/Shared/NtlNavi"
  "${DBO_PROJECT_ROOT}/Shared/NtlNavi/Source"
  "${DBO_PROJECT_ROOT}/Shared/Zip/zlib123"
  "${DBO_PROJECT_ROOT}/Server/NtlSystem"
  "${DBO_PROJECT_ROOT}/Server/NtlThread"
  "${DBO_PROJECT_ROOT}/Server/NtlNetwork"
  "${DBO_PROJECT_ROOT}/Server/NtlSfx"
  "${DBO_PROJECT_ROOT}/Server/NtlShared2"
  "${DBO_PROJECT_ROOT}/Server/NtlGameTable"
  "${DBO_PROJECT_ROOT}/Server/Database"
  "${DBO_PROJECT_ROOT}/Server/servercommon"
  "${DBO_PROJECT_ROOT}/Server/DboTrigger"
  "${DBO_PROJECT_ROOT}/Server/SCSCaptcha"
  "${DBO_PROJECT_ROOT}/extern/boost_1_64_0"
  "${DBO_PROJECT_ROOT}/extern/mysql57"
)

# Linux defaults and compatibility defines
add_compile_definitions(
  BOOST_ERROR_CODE_HEADER_ONLY
  BOOST_SYSTEM_NO_DEPRECATED
)

# Use Threads
find_package(Threads REQUIRED)

if(DBO_USE_CLANG)
  message(STATUS "Linux: using Clang (DBO_USE_CLANG=ON)")
elseif(DBO_LINUX_GCC_9)
  message(STATUS "Linux: using GCC 9 (g++-9) for packet layout test")
else()
  message(STATUS "Linux: using default g++ (install g++-9 to try GCC 9)")
endif()

# Boost (Linux): find shared or system libraries
set(DBO_BOOST_ROOT "${DBO_PROJECT_ROOT}/extern/boost_1_64_0" CACHE PATH "Boost root containing boost libs")
find_library(BOOST_THREAD_LIB
  NAMES boost_thread boost_thread-mt
  PATHS "${DBO_BOOST_ROOT}/lib" "${DBO_BOOST_ROOT}/stage/lib"
  NO_DEFAULT_PATH
)
if(NOT BOOST_THREAD_LIB)
  find_library(BOOST_THREAD_LIB NAMES boost_thread boost_thread-mt)
endif()

find_library(BOOST_SYSTEM_LIB
  NAMES boost_system boost_system-mt
  PATHS "${DBO_BOOST_ROOT}/lib" "${DBO_BOOST_ROOT}/stage/lib"
  NO_DEFAULT_PATH
)
if(NOT BOOST_SYSTEM_LIB)
  find_library(BOOST_SYSTEM_LIB NAMES boost_system boost_system-mt)
endif()

if(BOOST_THREAD_LIB AND BOOST_SYSTEM_LIB)
  message(STATUS "Found Boost thread: ${BOOST_THREAD_LIB}")
  message(STATUS "Found Boost system: ${BOOST_SYSTEM_LIB}")
  set(BOOST_SYSTEM_LIB_PATH "${BOOST_SYSTEM_LIB}" CACHE FILEPATH "Boost system library path")
  set(BOOST_THREAD_LIB_PATH "${BOOST_THREAD_LIB}" CACHE FILEPATH "Boost thread library path")
  get_filename_component(BOOST_SYSTEM_NAME "${BOOST_SYSTEM_LIB}" NAME_WE)
  get_filename_component(BOOST_THREAD_NAME "${BOOST_THREAD_LIB}" NAME_WE)
  string(REGEX REPLACE "^lib" "" BOOST_SYSTEM_NAME "${BOOST_SYSTEM_NAME}")
  string(REGEX REPLACE "^lib" "" BOOST_THREAD_NAME "${BOOST_THREAD_NAME}")
  set(BOOST_SYSTEM_LIB_NAME "${BOOST_SYSTEM_NAME}" CACHE STRING "Boost system library name")
  set(BOOST_THREAD_LIB_NAME "${BOOST_THREAD_NAME}" CACHE STRING "Boost thread library name")

  add_library(dbo_boost_system SHARED IMPORTED GLOBAL)
  set_target_properties(dbo_boost_system PROPERTIES
    IMPORTED_LOCATION "${BOOST_SYSTEM_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${DBO_BOOST_ROOT}"
    IMPORTED_NO_SONAME TRUE
  )
  add_library(dbo::boost_system ALIAS dbo_boost_system)

  add_library(dbo_boost_thread SHARED IMPORTED GLOBAL)
  set_target_properties(dbo_boost_thread PROPERTIES
    IMPORTED_LOCATION "${BOOST_THREAD_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${DBO_BOOST_ROOT}"
    INTERFACE_LINK_LIBRARIES dbo::boost_system
    IMPORTED_NO_SONAME TRUE
  )
  add_library(dbo::boost_thread ALIAS dbo_boost_thread)

  message(STATUS "Created Boost imported targets: dbo::boost_system, dbo::boost_thread")
  message(STATUS "Boost library names: ${BOOST_SYSTEM_LIB_NAME}, ${BOOST_THREAD_LIB_NAME}")
else()
  message(WARNING "Boost libraries not found. Install libboost-thread-dev and libboost-system-dev or provide libraries in ${DBO_BOOST_ROOT}.")
endif()

#
# Third-party / shared libs
#

# zlib123 (minimal zlib core)
file(GLOB _dbo_zlib_src
  "${DBO_PROJECT_ROOT}/Shared/Zip/zlib123/*.c"
)
list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\](minigzip|minizip|miniunz|example)\\.c$")
list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\]iowin32\\.c$")
add_library(zlib STATIC ${_dbo_zlib_src})
target_link_libraries(zlib PUBLIC dbo_includes)

# Lua (build as a library; exclude standalone executables)
dbo_collect_sources(_dbo_lua_src "${DBO_PROJECT_ROOT}/Shared/Lua")
list(FILTER _dbo_lua_src EXCLUDE REGEX ".*[/\\\\](lua|luac)\\.c$")
list(LENGTH _dbo_lua_src _dbo_lua_count)
if(_dbo_lua_count EQUAL 0)
  message(WARNING "No Lua sources found under ${DBO_PROJECT_ROOT}. Ensure the full project is present at that path. Continuing configure (build may still fail).")
endif()
add_library(Lua STATIC ${_dbo_lua_src})
target_link_libraries(Lua PUBLIC dbo_includes)

dbo_collect_sources(_dbo_util_src "${DBO_PROJECT_ROOT}/Shared/Util")
# Exclude Windows-only legacy sources that block Linux compilation
list(FILTER _dbo_util_src EXCLUDE REGEX ".*[/\\\\]CFixedString.*\\.cpp$")
list(FILTER _dbo_util_src EXCLUDE REGEX ".*[/\\\\]CLogMessageManager.*\\.cpp$")
add_library(Util STATIC ${_dbo_util_src})
target_link_libraries(Util PUBLIC dbo_includes)
find_package(OpenSSL REQUIRED)
target_link_libraries(Util PUBLIC OpenSSL::Crypto)
find_library(ICONV_LIB iconv)
if(ICONV_LIB)
  target_link_libraries(Util PUBLIC ${ICONV_LIB})
  message(STATUS "Found iconv library: ${ICONV_LIB}")
else()
  message(STATUS "iconv not found as separate library - assuming it's in libc (glibc)")
endif()

dbo_collect_sources(_dbo_ntltrigger_src "${DBO_PROJECT_ROOT}/Shared/NtlTrigger")
add_library(NtlTrigger STATIC ${_dbo_ntltrigger_src})
target_link_libraries(NtlTrigger PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlxmlloader_src "${DBO_PROJECT_ROOT}/Shared/NtlXMLLoader")
list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]msxml2_i\\.c$")
list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]xmlparser_i\\.c$")
add_library(NtlXMLLoader STATIC ${_dbo_ntlxmlloader_src})
target_link_libraries(NtlXMLLoader PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlnavi_src "${DBO_PROJECT_ROOT}/Shared/NtlNavi")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviWorldDataLoader\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviEntityFactory\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviPathDataLoader\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviField.*\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviWorld(Prop|Group)?_(WE|PE)\\.cpp$")
add_library(NtlNavi STATIC ${_dbo_ntlnavi_src})
target_link_libraries(NtlNavi PUBLIC dbo_includes dl)

#
# Server core libs
#

dbo_collect_sources(_dbo_ntlsystem_src "${DBO_PROJECT_ROOT}/Server/NtlSystem")
add_library(NtlSystem STATIC ${_dbo_ntlsystem_src})
target_link_libraries(NtlSystem PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlthread_src "${DBO_PROJECT_ROOT}/Server/NtlThread")
add_library(NtlThread STATIC ${_dbo_ntlthread_src})
target_link_libraries(NtlThread PUBLIC dbo_includes NtlSystem)

dbo_collect_sources(_dbo_ntlnetwork_src "${DBO_PROJECT_ROOT}/Server/NtlNetwork")
list(FILTER _dbo_ntlnetwork_src EXCLUDE REGEX ".*[/\\\\]PacketWireLayout\\.cpp$")
list(FILTER _dbo_ntlnetwork_src EXCLUDE REGEX ".*[/\\\\]PacketWireLayout\\.generated\\.cpp$")
# One-shot generated wire layer: run generator to produce PacketWireLayout.generated.cpp
set(_wire_gen_out "${CMAKE_BINARY_DIR}/PacketWireLayout.generated.cpp")
add_custom_command(OUTPUT "${_wire_gen_out}"
  COMMAND "${CMAKE_COMMAND}" -E env PYTHONIOENCODING=utf-8
    python3 "${DBO_PROJECT_ROOT}/tools/gen_packet_wire.py"
    --wire "${DBO_PROJECT_ROOT}/wire/wire_layout.txt"
    --our "${DBO_PROJECT_ROOT}/wire/our_layout.txt"
    -o "${_wire_gen_out}"
  DEPENDS "${DBO_PROJECT_ROOT}/wire/wire_layout.txt"
          "${DBO_PROJECT_ROOT}/wire/our_layout.txt"
          "${DBO_PROJECT_ROOT}/tools/gen_packet_wire.py"
  WORKING_DIRECTORY "${DBO_PROJECT_ROOT}"
  COMMENT "Generating packet wire layout (one-shot)")
add_library(NtlNetwork STATIC ${_dbo_ntlnetwork_src} "${_wire_gen_out}")
target_link_libraries(NtlNetwork PUBLIC dbo_includes NtlSystem NtlThread Util)

dbo_collect_sources(_dbo_ntlsfx_src "${DBO_PROJECT_ROOT}/Server/NtlSfx")
add_library(NtlSfx STATIC ${_dbo_ntlsfx_src})
target_link_libraries(NtlSfx PUBLIC dbo_includes NtlNetwork NtlSystem NtlThread)

dbo_collect_sources(_dbo_ntlshared_src "${DBO_PROJECT_ROOT}/Server/NtlShared2")
add_library(NtlShared STATIC ${_dbo_ntlshared_src})
target_link_libraries(NtlShared PUBLIC dbo_includes NtlSystem NtlTrigger)
# Option 1 global packet layout: MS-compatible struct layout for client wire format (no -fpack-struct)
if(UNIX)
  target_compile_options(NtlShared PRIVATE -mms-bitfields)
endif()

dbo_collect_sources(_dbo_ntlgametable_src "${DBO_PROJECT_ROOT}/Server/NtlGameTable")
add_library(NtlGameTable STATIC ${_dbo_ntlgametable_src})
target_link_libraries(NtlGameTable PUBLIC dbo_includes NtlSystem NtlShared NtlTrigger NtlXMLLoader)

dbo_collect_sources(_dbo_dbotrigger_src "${DBO_PROJECT_ROOT}/Server/DboTrigger")
list(FILTER _dbo_dbotrigger_src EXCLUDE REGEX ".*[/\\\\]DboTSActBroadCast\\.cpp$")
list(FILTER _dbo_dbotrigger_src EXCLUDE REGEX ".*[/\\\\]DboTSActOpenInventory\\.cpp$")
add_library(DboTrigger STATIC ${_dbo_dbotrigger_src})
target_link_libraries(DboTrigger PUBLIC dbo_includes NtlTrigger zlib NtlGameTable NtlShared)

dbo_collect_sources(_dbo_scscaptcha_src "${DBO_PROJECT_ROOT}/Server/SCSCaptcha")
add_library(SCSCaptcha STATIC ${_dbo_scscaptcha_src})
target_link_libraries(SCSCaptcha PUBLIC dbo_includes)

dbo_collect_sources(_dbo_servercommon_src "${DBO_PROJECT_ROOT}/Server/servercommon")
add_library(servercommon STATIC ${_dbo_servercommon_src})
target_link_libraries(servercommon PUBLIC dbo_includes NtlSystem)

dbo_collect_sources(_dbo_database_src "${DBO_PROJECT_ROOT}/Server/Database")
add_library(Database STATIC ${_dbo_database_src})
target_link_libraries(Database PUBLIC dbo_includes NtlThread)
if(BOOST_SYSTEM_LIB AND EXISTS "${BOOST_SYSTEM_LIB}")
  target_link_libraries(Database PRIVATE "${BOOST_SYSTEM_LIB}")
elseif(TARGET dbo::boost_system)
  target_link_libraries(Database PRIVATE dbo::boost_system)
endif()
if(BOOST_THREAD_LIB AND EXISTS "${BOOST_THREAD_LIB}")
  target_link_libraries(Database PRIVATE "${BOOST_THREAD_LIB}")
elseif(TARGET dbo::boost_thread)
  target_link_libraries(Database PRIVATE dbo::boost_thread)
endif()
dbo_configure_mysqlclient()
target_link_libraries(Database PUBLIC dbo::mysqlclient)

#
# Server executables
#

dbo_collect_sources(_dbo_authserver_src "${DBO_PROJECT_ROOT}/Server/AuthServer")
add_executable(AuthServer ${_dbo_authserver_src})
target_link_libraries(AuthServer PRIVATE dbo_includes Util Database NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
target_link_libraries(AuthServer PRIVATE Threads::Threads)
set(BOOST_LIB_PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
find_library(BOOST_THREAD_LIB_FINAL 
  NAMES boost_thread 
  PATHS ${BOOST_LIB_PATHS}
  NO_DEFAULT_PATH
)
if(NOT BOOST_THREAD_LIB_FINAL)
  find_library(BOOST_THREAD_LIB_FINAL NAMES boost_thread)
endif()
target_link_options(AuthServer PRIVATE "SHELL:-Wl,--no-as-needed")
if(BOOST_THREAD_LIB_FINAL AND EXISTS "${BOOST_THREAD_LIB_FINAL}")
  message(STATUS "  Found and linking Boost.Thread: ${BOOST_THREAD_LIB_FINAL}")
  target_link_libraries(AuthServer PRIVATE "${BOOST_THREAD_LIB_FINAL}")
else()
  message(STATUS "  Linking Boost.Thread by name: boost_thread")
  target_link_libraries(AuthServer PRIVATE boost_thread)
endif()
# Fix 9g/19b/19d: Do NOT use -fpack-struct on MasterServerPacket.cpp. It parses incoming MA_ON_PLAYER_CHECK_RES (69 bytes from Master); packed layout caused segfault when handling that packet (fix log 19d). AU_LOGIN_RES we send remains default layout (839 bytes).

dbo_collect_sources(_dbo_charserver_src "${DBO_PROJECT_ROOT}/Server/CharServer")
add_executable(CharServer ${_dbo_charserver_src})
target_link_libraries(CharServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
target_link_libraries(CharServer PRIVATE Threads::Threads)

dbo_collect_sources(_dbo_chatserver_src "${DBO_PROJECT_ROOT}/Server/ChatServer")
add_executable(ChatServer ${_dbo_chatserver_src})
target_link_libraries(ChatServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
target_link_libraries(ChatServer PRIVATE Threads::Threads)

dbo_collect_sources(_dbo_queryserver_src "${DBO_PROJECT_ROOT}/Server/QueryServer")
list(FILTER _dbo_queryserver_src EXCLUDE REGEX ".*[/\\\\]GameDatabase\\.cpp$")
add_executable(QueryServer ${_dbo_queryserver_src})
target_link_libraries(QueryServer PRIVATE dbo_includes Util Database NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
target_link_libraries(QueryServer PRIVATE Threads::Threads)

dbo_collect_sources(_dbo_masterserver_src "${DBO_PROJECT_ROOT}/Server/MasterServer")
add_executable(MasterServer ${_dbo_masterserver_src})
target_link_libraries(MasterServer PRIVATE dbo_includes Util NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
target_link_libraries(MasterServer PRIVATE Threads::Threads)

dbo_collect_sources(_dbo_gameserver_src "${DBO_PROJECT_ROOT}/Server/GameServer")
add_executable(GameServer ${_dbo_gameserver_src})
target_link_libraries(GameServer PRIVATE dbo_includes Lua NtlTrigger NtlXMLLoader Util zlib Database DboTrigger NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlNavi SCSCaptcha)
target_link_libraries(GameServer PRIVATE Threads::Threads)

message(STATUS "Linux-only build configured. Struct packing via #pragma pack(1) in headers.")

