cmake_minimum_required(VERSION 3.15)

project(DboServerFiles LANGUAGES C CXX)

# Use the directory of this file so Linux builds see the actual project root.
set(DBO_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
# When VS copies only CMakeLists.txt to .vs/full_server, use the WSL mount so GLOB finds sources. Build output stays under .vs so run/debug finds the executable.
if(UNIX AND NOT APPLE)
  if(NOT EXISTS "${DBO_PROJECT_ROOT}/Shared/Lua")
    if(EXISTS "/mnt/e/SERVER/full_server/CMakeLists.txt")
      set(DBO_PROJECT_ROOT "/mnt/e/SERVER/full_server")
      message(STATUS "Linux: using WSL mount for sources (full project not in .vs): ${DBO_PROJECT_ROOT}")
    endif()
  endif()
endif()
message(STATUS "DBO_PROJECT_ROOT=${DBO_PROJECT_ROOT}")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/third_party.cmake")

function(dbo_collect_sources out_var base_dir)
  file(GLOB_RECURSE _dbo_src CONFIGURE_DEPENDS
    "${base_dir}/*.c"
    "${base_dir}/*.cc"
    "${base_dir}/*.cpp"
    "${base_dir}/*.cxx"
  )

  # Exclude MSVC build output and other non-source trees.
  list(FILTER _dbo_src EXCLUDE REGEX ".*[/\\\\](Debug|Release|ReleaseX64|x64|\\.vs|ipch|tlog|node_modules)[/\\\\].*")

  # Precompiled-header translation units (MSVC-only)
  if(NOT WIN32)
    list(FILTER _dbo_src EXCLUDE REGEX ".*[/\\\\]stdafx\\.cpp$")
  endif()

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

if(MSVC)
  add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_DEPRECATE
    BOOST_ALL_NO_LIB
  )

  # Match vendored Boost libs (libboost_*-mt-s-*.lib => /MT runtime)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  # Linux: Use header-only Boost.System to avoid version mismatch issues
  # Boost 1.64.0 headers expect symbols in libboost_system.so, but
  # system Boost 1.83.0 libraries don't have them (header-only in newer versions)
  # Defining BOOST_ERROR_CODE_HEADER_ONLY makes Boost 1.64.0 headers use inline implementations
  # BOOST_SYSTEM_NO_DEPRECATED disables deprecated 'throws' variable that causes multiple definition errors
  add_compile_definitions(
    BOOST_ERROR_CODE_HEADER_ONLY
    BOOST_SYSTEM_NO_DEPRECATED
  )
endif()

if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  set(DBO_PACK_STRUCT_FOR_LINUX 1)
  # Optional: use Clang for Linux (may produce correct packet struct layout). Configure with -DDBO_USE_CLANG=ON
  if(DBO_USE_CLANG)
    set(CMAKE_C_COMPILER clang)
    set(CMAKE_CXX_COMPILER clang++)
    message(STATUS "Linux: using Clang (DBO_USE_CLANG=ON)")
  endif()
endif()

# Boost (required: Database uses boost::thread/mutex/condition)
# This repo vendors prebuilt Boost libs under extern/boost_1_64_0/*.lib.
# Avoid FindBoost (CMake 4.x policy/module changes) by creating imported targets directly.
if(WIN32)
  set(DBO_BOOST_ROOT "${DBO_PROJECT_ROOT}/extern/boost_1_64_0" CACHE PATH "Boost root containing boost/ and libboost_*.lib")

  add_library(dbo_boost_thread UNKNOWN IMPORTED)
  set_target_properties(dbo_boost_thread PROPERTIES
    IMPORTED_LOCATION "${DBO_BOOST_ROOT}/libboost_thread-vc141-mt-s-1_64.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${DBO_BOOST_ROOT}"
  )
  add_library(dbo::boost_thread ALIAS dbo_boost_thread)

  add_library(dbo_boost_system UNKNOWN IMPORTED)
  set_target_properties(dbo_boost_system PROPERTIES
    IMPORTED_LOCATION "${DBO_BOOST_ROOT}/libboost_system-vc141-mt-s-1_64.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${DBO_BOOST_ROOT}"
  )
  add_library(dbo::boost_system ALIAS dbo_boost_system)
else()
  # Linux: find Boost libraries
  set(DBO_BOOST_ROOT "${DBO_PROJECT_ROOT}/extern/boost_1_64_0" CACHE PATH "Boost root containing boost/ and libboost_*.so")
  
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
    # Store library paths in cache for later use
    set(BOOST_SYSTEM_LIB_PATH "${BOOST_SYSTEM_LIB}" CACHE FILEPATH "Boost system library path")
    set(BOOST_THREAD_LIB_PATH "${BOOST_THREAD_LIB}" CACHE FILEPATH "Boost thread library path")
    
    # Extract library names without path and extension for -l linking
    get_filename_component(BOOST_SYSTEM_NAME "${BOOST_SYSTEM_LIB}" NAME_WE)
    get_filename_component(BOOST_THREAD_NAME "${BOOST_THREAD_LIB}" NAME_WE)
    # Remove "lib" prefix if present
    string(REGEX REPLACE "^lib" "" BOOST_SYSTEM_NAME "${BOOST_SYSTEM_NAME}")
    string(REGEX REPLACE "^lib" "" BOOST_THREAD_NAME "${BOOST_THREAD_NAME}")
    set(BOOST_SYSTEM_LIB_NAME "${BOOST_SYSTEM_NAME}" CACHE STRING "Boost system library name")
    set(BOOST_THREAD_LIB_NAME "${BOOST_THREAD_NAME}" CACHE STRING "Boost thread library name")
    
    # Create imported targets for shared libraries
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
    message(WARNING "Boost libraries not found!")
    message(WARNING "  BOOST_THREAD_LIB: ${BOOST_THREAD_LIB}")
    message(WARNING "  BOOST_SYSTEM_LIB: ${BOOST_SYSTEM_LIB}")
    message(WARNING "  Searched in: ${DBO_BOOST_ROOT}/lib and ${DBO_BOOST_ROOT}/stage/lib")
    message(WARNING "  Install Boost with: sudo apt-get install libboost-thread-dev libboost-system-dev")
    message(WARNING "  Or build Boost from source in extern/boost_1_64_0/")
  endif()
endif()

#
# Third-party / shared libs
#

# zlib123 (minimal zlib core)
file(GLOB _dbo_zlib_src CONFIGURE_DEPENDS
  "${DBO_PROJECT_ROOT}/Shared/Zip/zlib123/*.c"
)
list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\](minigzip|minizip|miniunz|example)\\.c$")
if(NOT WIN32)
  list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\]iowin32\\.c$")
endif()
add_library(zlib STATIC ${_dbo_zlib_src})
target_link_libraries(zlib PUBLIC dbo_includes)
if(WIN32)
  # CNtlUnzip is used by DboTrigger on Windows.
  target_sources(zlib PRIVATE "${DBO_PROJECT_ROOT}/Shared/Zip/zlib123/NtlUnzip.cpp")
endif()

# Lua (build as a library; exclude standalone executables)
dbo_collect_sources(_dbo_lua_src "${DBO_PROJECT_ROOT}/Shared/Lua")
list(FILTER _dbo_lua_src EXCLUDE REGEX ".*[/\\\\](lua|luac)\\.c$")
list(LENGTH _dbo_lua_src _dbo_lua_count)
if(_dbo_lua_count EQUAL 0)
  message(FATAL_ERROR "No sources found under ${DBO_PROJECT_ROOT}. Ensure the full project is copied to the remote machine so build and run both use the same path (e.g. ~/.vs/full_server). In VS: Project > Copy Sources to Remote Machine, or open the folder from the WSL path where the full repo exists.")
endif()
add_library(Lua STATIC ${_dbo_lua_src})
target_link_libraries(Lua PUBLIC dbo_includes)

dbo_collect_sources(_dbo_util_src "${DBO_PROJECT_ROOT}/Shared/Util")
if(NOT WIN32)
  # These legacy FixedString implementations are stored in UTF-16LE and also use
  # Windows-style precompiled-header naming. They are not referenced by the servers
  # and block Linux compilation.
  list(FILTER _dbo_util_src EXCLUDE REGEX ".*[/\\\\]CFixedString.*\\.cpp$")
  # CLogMessageManager uses Windows-only APIs (TCHAR, WriteFile, GetLocalTime,
  # SYSTEMTIME, _tcscpy_s, etc.). Exclude from Linux build.
  list(FILTER _dbo_util_src EXCLUDE REGEX ".*[/\\\\]CLogMessageManager.*\\.cpp$")
endif()
add_library(Util STATIC ${_dbo_util_src})
target_link_libraries(Util PUBLIC dbo_includes)
if(UNIX AND NOT APPLE)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(Util PUBLIC OpenSSL::Crypto)
  # iconv is needed for UTF-8 <-> UTF-16LE conversion (WCHAR is unsigned short on Linux)
  # On most Linux systems, iconv is part of glibc and doesn't need a separate library.
  # Try to find libiconv (separate library), but if not found, assume it's in libc (common case).
  find_library(ICONV_LIB iconv)
  if(ICONV_LIB)
    target_link_libraries(Util PUBLIC ${ICONV_LIB})
    message(STATUS "Found iconv library: ${ICONV_LIB}")
  else()
    # iconv is usually in libc (glibc) on Linux, so no separate library needed
    # The code will compile and link fine without explicitly linking libiconv
    message(STATUS "iconv not found as separate library - assuming it's in libc (glibc)")
  endif()
endif()

dbo_collect_sources(_dbo_ntltrigger_src "${DBO_PROJECT_ROOT}/Shared/NtlTrigger")
add_library(NtlTrigger STATIC ${_dbo_ntltrigger_src})
target_link_libraries(NtlTrigger PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlxmlloader_src "${DBO_PROJECT_ROOT}/Shared/NtlXMLLoader")
# msxml2_i.c and xmlparser_i.c are MIDL-generated Windows RPC/COM interface stubs;
# NtlXMLDoc has Linux stubs that don't use MSXML.
if(NOT WIN32)
  list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]msxml2_i\\.c$")
  list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]xmlparser_i\\.c$")
endif()
add_library(NtlXMLLoader STATIC ${_dbo_ntlxmlloader_src})
target_link_libraries(NtlXMLLoader PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlnavi_src "${DBO_PROJECT_ROOT}/Shared/NtlNavi")
# Match the original NtlNavi.vcxproj: exclude legacy/tool-only sources that are not built there.
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviWorldDataLoader\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviEntityFactory\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviPathDataLoader\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviField.*\\.cpp$")
list(FILTER _dbo_ntlnavi_src EXCLUDE REGEX ".*[/\\\\]Source[/\\\\]NtlNaviWorld(Prop|Group)?_(WE|PE)\\.cpp$")
add_library(NtlNavi STATIC ${_dbo_ntlnavi_src})
target_link_libraries(NtlNavi PUBLIC dbo_includes)
if(UNIX AND NOT APPLE)
  target_link_libraries(NtlNavi PUBLIC dl)
endif()

#
# Server core libs
#

dbo_collect_sources(_dbo_ntlsystem_src "${DBO_PROJECT_ROOT}/Server/NtlSystem")
if(NOT WIN32)
  # NtlPdh.cpp and NtlMiniDump.cpp have Linux stubs (#if !defined(_WIN32)) and must be built on both platforms
endif()
add_library(NtlSystem STATIC ${_dbo_ntlsystem_src})
target_link_libraries(NtlSystem PUBLIC dbo_includes)
if(WIN32)
  target_link_libraries(NtlSystem PUBLIC dbghelp pdh)
endif()

dbo_collect_sources(_dbo_ntlthread_src "${DBO_PROJECT_ROOT}/Server/NtlThread")
add_library(NtlThread STATIC ${_dbo_ntlthread_src})
target_link_libraries(NtlThread PUBLIC dbo_includes NtlSystem)

dbo_collect_sources(_dbo_ntlnetwork_src "${DBO_PROJECT_ROOT}/Server/NtlNetwork")
add_library(NtlNetwork STATIC ${_dbo_ntlnetwork_src})
target_link_libraries(NtlNetwork PUBLIC dbo_includes NtlSystem NtlThread Util)
if(WIN32)
  target_link_libraries(NtlNetwork PUBLIC ws2_32)
endif()

dbo_collect_sources(_dbo_ntlsfx_src "${DBO_PROJECT_ROOT}/Server/NtlSfx")
add_library(NtlSfx STATIC ${_dbo_ntlsfx_src})
target_link_libraries(NtlSfx PUBLIC dbo_includes NtlNetwork NtlSystem NtlThread)

dbo_collect_sources(_dbo_ntlshared_src "${DBO_PROJECT_ROOT}/Server/NtlShared2")
add_library(NtlShared STATIC ${_dbo_ntlshared_src})
target_link_libraries(NtlShared PUBLIC dbo_includes NtlSystem NtlTrigger)
# Linux: Pack structs for packet wire layout (match Windows client). NtlShared compiles packet headers; must pack here for sizeof checks.
if(DBO_PACK_STRUCT_FOR_LINUX)
  target_compile_options(NtlShared PRIVATE -fpack-struct)
  message(STATUS "Linux: -fpack-struct applied to NtlShared (packet wire format for Windows client)")
endif()

dbo_collect_sources(_dbo_ntlgametable_src "${DBO_PROJECT_ROOT}/Server/NtlGameTable")
add_library(NtlGameTable STATIC ${_dbo_ntlgametable_src})
target_link_libraries(NtlGameTable PUBLIC dbo_includes NtlSystem NtlShared NtlTrigger NtlXMLLoader)

dbo_collect_sources(_dbo_dbotrigger_src "${DBO_PROJECT_ROOT}/Server/DboTrigger")
# Match the original DboTrigger.vcxproj: exclude actions that aren't in the project.
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
if(WIN32)
  target_link_libraries(Database PUBLIC dbo_includes NtlThread dbo::boost_thread dbo::boost_system)
else()
  target_link_libraries(Database PUBLIC dbo_includes NtlThread)
  # Boost libraries: Use PRIVATE (not PUBLIC) so they don't propagate early
  # They will be linked directly to executables at the end
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
endif()
dbo_configure_mysqlclient()
target_link_libraries(Database PUBLIC dbo::mysqlclient)

#
# Server executables
#

dbo_collect_sources(_dbo_authserver_src "${DBO_PROJECT_ROOT}/Server/AuthServer")
add_executable(AuthServer ${_dbo_authserver_src})
# Link static libraries first
target_link_libraries(AuthServer PRIVATE dbo_includes Util Database NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
if(WIN32)
  target_link_libraries(AuthServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(AuthServer PRIVATE Threads::Threads)
  # Boost shared libraries MUST come at the very end after ALL static libraries
  # Database is a static library that uses Boost internally, so AuthServer must link Boost directly
  # Use --start-group/--end-group to handle circular dependencies in static libs
  # Then link Boost libraries after the end-group with --no-as-needed to force inclusion
  
  message(STATUS "Configuring Boost library linking for AuthServer...")
  
  # Find Boost libraries in standard apt-get installation paths
  set(BOOST_LIB_PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
  find_library(BOOST_SYSTEM_LIB_FINAL 
    NAMES boost_system 
    PATHS ${BOOST_LIB_PATHS}
    NO_DEFAULT_PATH
  )
  if(NOT BOOST_SYSTEM_LIB_FINAL)
    find_library(BOOST_SYSTEM_LIB_FINAL NAMES boost_system)
  endif()
  
  find_library(BOOST_THREAD_LIB_FINAL 
    NAMES boost_thread 
    PATHS ${BOOST_LIB_PATHS}
    NO_DEFAULT_PATH
  )
  if(NOT BOOST_THREAD_LIB_FINAL)
    find_library(BOOST_THREAD_LIB_FINAL NAMES boost_thread)
  endif()
  
  # NOTE: Boost.System is now header-only (BOOST_ERROR_CODE_HEADER_ONLY defined above)
  # So we only need to link Boost.Thread library
  # Boost.Thread still requires Boost.System headers, but no library linking needed for System
  
  # Link Boost.Thread library (still requires library, not header-only)
  # Use --no-as-needed to force inclusion since symbols are referenced from static libraries
  target_link_options(AuthServer PRIVATE "SHELL:-Wl,--no-as-needed")
  
  if(BOOST_THREAD_LIB_FINAL AND EXISTS "${BOOST_THREAD_LIB_FINAL}")
    message(STATUS "  Found and linking Boost.Thread: ${BOOST_THREAD_LIB_FINAL}")
    target_link_libraries(AuthServer PRIVATE "${BOOST_THREAD_LIB_FINAL}")
  else()
    message(STATUS "  Linking Boost.Thread by name: boost_thread")
    target_link_libraries(AuthServer PRIVATE boost_thread)
  endif()
  
  # Re-enable --as-needed after Boost libraries
  target_link_options(AuthServer PRIVATE "SHELL:-Wl,--as-needed")
  
  message(STATUS "Boost libraries configured for AuthServer (System is header-only)")
endif()

dbo_collect_sources(_dbo_charserver_src "${DBO_PROJECT_ROOT}/Server/CharServer")
add_executable(CharServer ${_dbo_charserver_src})
target_link_libraries(CharServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
if(WIN32)
  target_link_libraries(CharServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(CharServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_chatserver_src "${DBO_PROJECT_ROOT}/Server/ChatServer")
add_executable(ChatServer ${_dbo_chatserver_src})
target_link_libraries(ChatServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
if(WIN32)
  target_link_libraries(ChatServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(ChatServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_queryserver_src "${DBO_PROJECT_ROOT}/Server/QueryServer")
# Match the original QueryServer.vcxproj: this file references a missing header and isn't built there.
list(FILTER _dbo_queryserver_src EXCLUDE REGEX ".*[/\\\\]GameDatabase\\.cpp$")
add_executable(QueryServer ${_dbo_queryserver_src})
target_link_libraries(QueryServer PRIVATE dbo_includes Util Database NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
if(WIN32)
  target_link_libraries(QueryServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(QueryServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_masterserver_src "${DBO_PROJECT_ROOT}/Server/MasterServer")
add_executable(MasterServer ${_dbo_masterserver_src})
target_link_libraries(MasterServer PRIVATE dbo_includes Util NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
if(WIN32)
  target_link_libraries(MasterServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(MasterServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_gameserver_src "${DBO_PROJECT_ROOT}/Server/GameServer")
add_executable(GameServer ${_dbo_gameserver_src})
target_link_libraries(GameServer PRIVATE dbo_includes Lua NtlTrigger NtlXMLLoader Util zlib Database DboTrigger NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlNavi SCSCaptcha)
if(WIN32)
  target_link_libraries(GameServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(GameServer PRIVATE Threads::Threads)
endif()

# Pack structs: NtlShared gets -fpack-struct for packet wire layout. Do NOT apply to entire executables
# (Fix 9f): -fpack-struct on executables broke CNtlIniFile (std::map corruption in destructor).
# (Fix 9g): #pragma pack(1) in packet headers is insufficient for GCC; Linux got 839 bytes vs Windows 795.
# Apply -fpack-struct only to source files that BUILD packets sent to client (MasterServerPacket.cpp).
if(DBO_PACK_STRUCT_FOR_LINUX)
  set_source_files_properties(
    "${DBO_PROJECT_ROOT}/Server/AuthServer/MasterServerPacket.cpp"
    PROPERTIES COMPILE_OPTIONS "-fpack-struct"
  )
  message(STATUS "Linux: -fpack-struct applied to AuthServer/MasterServerPacket.cpp (AU_LOGIN_RES wire format)")
endif()

# Windows resources (optional)
if(WIN32)
  foreach(_exe IN ITEMS AuthServer CharServer ChatServer QueryServer MasterServer GameServer)
    if(EXISTS "${DBO_PROJECT_ROOT}/Server/${_exe}/${_exe}.rc")
      target_sources(${_exe} PRIVATE "${DBO_PROJECT_ROOT}/Server/${_exe}/${_exe}.rc")
    endif()
  endforeach()
endif()

