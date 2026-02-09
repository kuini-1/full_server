cmake_minimum_required(VERSION 3.15)

project(DboServerFiles LANGUAGES C CXX)

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
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Util"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Lua"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Lua/lua/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlTrigger"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlXMLLoader"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlNavi"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlNavi/Source"
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Zip/zlib123"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlSystem"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlThread"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlNetwork"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlSfx"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlShared2"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlGameTable"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/Database"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/servercommon"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/DboTrigger"
  "${CMAKE_CURRENT_SOURCE_DIR}/Server/SCSCaptcha"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/boost_1_64_0"
  "${CMAKE_CURRENT_SOURCE_DIR}/extern/mysql57"
)

if(MSVC)
  add_compile_definitions(
    _CRT_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_DEPRECATE
    BOOST_ALL_NO_LIB
  )

  # Match vendored Boost libs (libboost_*-mt-s-*.lib => /MT runtime)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
endif()

# Boost (required: Database uses boost::thread/mutex/condition)
# This repo vendors prebuilt Boost libs under extern/boost_1_64_0/*.lib.
# Avoid FindBoost (CMake 4.x policy/module changes) by creating imported targets directly.
if(WIN32)
  set(DBO_BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/boost_1_64_0" CACHE PATH "Boost root containing boost/ and libboost_*.lib")

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
  set(DBO_BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/extern/boost_1_64_0" CACHE PATH "Boost root containing boost/ and libboost_*.so")
  
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
  "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Zip/zlib123/*.c"
)
list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\](minigzip|minizip|miniunz|example)\\.c$")
if(NOT WIN32)
  list(FILTER _dbo_zlib_src EXCLUDE REGEX ".*[/\\\\]iowin32\\.c$")
endif()
add_library(zlib STATIC ${_dbo_zlib_src})
target_link_libraries(zlib PUBLIC dbo_includes)
if(WIN32)
  # CNtlUnzip is used by DboTrigger on Windows.
  target_sources(zlib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Zip/zlib123/NtlUnzip.cpp")
endif()

# Lua (build as a library; exclude standalone executables)
dbo_collect_sources(_dbo_lua_src "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Lua")
list(FILTER _dbo_lua_src EXCLUDE REGEX ".*[/\\\\](lua|luac)\\.c$")
add_library(Lua STATIC ${_dbo_lua_src})
target_link_libraries(Lua PUBLIC dbo_includes)

dbo_collect_sources(_dbo_util_src "${CMAKE_CURRENT_SOURCE_DIR}/Shared/Util")
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
endif()

dbo_collect_sources(_dbo_ntltrigger_src "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlTrigger")
add_library(NtlTrigger STATIC ${_dbo_ntltrigger_src})
target_link_libraries(NtlTrigger PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlxmlloader_src "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlXMLLoader")
# msxml2_i.c and xmlparser_i.c are MIDL-generated Windows RPC/COM interface stubs;
# NtlXMLDoc has Linux stubs that don't use MSXML.
if(NOT WIN32)
  list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]msxml2_i\\.c$")
  list(FILTER _dbo_ntlxmlloader_src EXCLUDE REGEX ".*[/\\\\]xmlparser_i\\.c$")
endif()
add_library(NtlXMLLoader STATIC ${_dbo_ntlxmlloader_src})
target_link_libraries(NtlXMLLoader PUBLIC dbo_includes)

dbo_collect_sources(_dbo_ntlnavi_src "${CMAKE_CURRENT_SOURCE_DIR}/Shared/NtlNavi")
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

dbo_collect_sources(_dbo_ntlsystem_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlSystem")
if(NOT WIN32)
  # NtlPdh.cpp and NtlMiniDump.cpp have Linux stubs (#if !defined(_WIN32)) and must be built on both platforms
endif()
add_library(NtlSystem STATIC ${_dbo_ntlsystem_src})
target_link_libraries(NtlSystem PUBLIC dbo_includes)
if(WIN32)
  target_link_libraries(NtlSystem PUBLIC dbghelp pdh)
endif()

dbo_collect_sources(_dbo_ntlthread_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlThread")
add_library(NtlThread STATIC ${_dbo_ntlthread_src})
target_link_libraries(NtlThread PUBLIC dbo_includes NtlSystem)

dbo_collect_sources(_dbo_ntlnetwork_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlNetwork")
add_library(NtlNetwork STATIC ${_dbo_ntlnetwork_src})
target_link_libraries(NtlNetwork PUBLIC dbo_includes NtlSystem NtlThread Util)
if(WIN32)
  target_link_libraries(NtlNetwork PUBLIC ws2_32)
endif()

dbo_collect_sources(_dbo_ntlsfx_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlSfx")
add_library(NtlSfx STATIC ${_dbo_ntlsfx_src})
target_link_libraries(NtlSfx PUBLIC dbo_includes NtlNetwork NtlSystem NtlThread)

dbo_collect_sources(_dbo_ntlshared_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlShared2")
add_library(NtlShared STATIC ${_dbo_ntlshared_src})
target_link_libraries(NtlShared PUBLIC dbo_includes NtlSystem NtlTrigger)

dbo_collect_sources(_dbo_ntlgametable_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/NtlGameTable")
add_library(NtlGameTable STATIC ${_dbo_ntlgametable_src})
target_link_libraries(NtlGameTable PUBLIC dbo_includes NtlSystem NtlShared NtlTrigger NtlXMLLoader)

dbo_collect_sources(_dbo_dbotrigger_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/DboTrigger")
# Match the original DboTrigger.vcxproj: exclude actions that aren't in the project.
list(FILTER _dbo_dbotrigger_src EXCLUDE REGEX ".*[/\\\\]DboTSActBroadCast\\.cpp$")
list(FILTER _dbo_dbotrigger_src EXCLUDE REGEX ".*[/\\\\]DboTSActOpenInventory\\.cpp$")
add_library(DboTrigger STATIC ${_dbo_dbotrigger_src})
target_link_libraries(DboTrigger PUBLIC dbo_includes NtlTrigger zlib NtlGameTable NtlShared)

dbo_collect_sources(_dbo_scscaptcha_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/SCSCaptcha")
add_library(SCSCaptcha STATIC ${_dbo_scscaptcha_src})
target_link_libraries(SCSCaptcha PUBLIC dbo_includes)

dbo_collect_sources(_dbo_servercommon_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/servercommon")
add_library(servercommon STATIC ${_dbo_servercommon_src})
target_link_libraries(servercommon PUBLIC dbo_includes NtlSystem)

dbo_collect_sources(_dbo_database_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/Database")
add_library(Database STATIC ${_dbo_database_src})
if(WIN32)
  target_link_libraries(Database PUBLIC dbo_includes NtlThread dbo::boost_thread dbo::boost_system)
else()
  target_link_libraries(Database PUBLIC dbo_includes NtlThread)
  # Boost libraries must be linked - system before thread
  if(TARGET dbo::boost_system)
    target_link_libraries(Database PUBLIC dbo::boost_system)
  endif()
  if(TARGET dbo::boost_thread)
    target_link_libraries(Database PUBLIC dbo::boost_thread)
  endif()
endif()
dbo_configure_mysqlclient()
target_link_libraries(Database PUBLIC dbo::mysqlclient)

#
# Server executables
#

dbo_collect_sources(_dbo_authserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/AuthServer")
add_executable(AuthServer ${_dbo_authserver_src})
target_link_libraries(AuthServer PRIVATE dbo_includes Util Database NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
if(WIN32)
  target_link_libraries(AuthServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(AuthServer PRIVATE Threads::Threads)
  # Ensure Boost libraries are linked - system must come before thread
  # Use find_library result directly - this should work reliably
  if(BOOST_SYSTEM_LIB AND EXISTS "${BOOST_SYSTEM_LIB}")
    message(STATUS "Linking boost_system: ${BOOST_SYSTEM_LIB}")
    target_link_libraries(AuthServer PRIVATE "${BOOST_SYSTEM_LIB}")
  elseif(TARGET dbo::boost_system)
    message(STATUS "Linking boost_system via target")
    target_link_libraries(AuthServer PRIVATE dbo::boost_system)
  else()
    message(FATAL_ERROR "Boost system library not found! Install with: sudo apt-get install libboost-system-dev")
  endif()
  
  if(BOOST_THREAD_LIB AND EXISTS "${BOOST_THREAD_LIB}")
    message(STATUS "Linking boost_thread: ${BOOST_THREAD_LIB}")
    target_link_libraries(AuthServer PRIVATE "${BOOST_THREAD_LIB}")
  elseif(TARGET dbo::boost_thread)
    message(STATUS "Linking boost_thread via target")
    target_link_libraries(AuthServer PRIVATE dbo::boost_thread)
  else()
    message(FATAL_ERROR "Boost thread library not found! Install with: sudo apt-get install libboost-thread-dev")
  endif()
endif()

dbo_collect_sources(_dbo_charserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/CharServer")
add_executable(CharServer ${_dbo_charserver_src})
target_link_libraries(CharServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
if(WIN32)
  target_link_libraries(CharServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(CharServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_chatserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/ChatServer")
add_executable(ChatServer ${_dbo_chatserver_src})
target_link_libraries(ChatServer PRIVATE dbo_includes Util Database NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlXMLLoader)
if(WIN32)
  target_link_libraries(ChatServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(ChatServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_queryserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/QueryServer")
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

dbo_collect_sources(_dbo_masterserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/MasterServer")
add_executable(MasterServer ${_dbo_masterserver_src})
target_link_libraries(MasterServer PRIVATE dbo_includes Util NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon)
if(WIN32)
  target_link_libraries(MasterServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(MasterServer PRIVATE Threads::Threads)
endif()

dbo_collect_sources(_dbo_gameserver_src "${CMAKE_CURRENT_SOURCE_DIR}/Server/GameServer")
add_executable(GameServer ${_dbo_gameserver_src})
target_link_libraries(GameServer PRIVATE dbo_includes Lua NtlTrigger NtlXMLLoader Util zlib Database DboTrigger NtlGameTable NtlNetwork NtlSfx NtlShared NtlSystem NtlThread servercommon NtlNavi SCSCaptcha)
if(WIN32)
  target_link_libraries(GameServer PRIVATE ws2_32 winmm)
endif()
if(UNIX AND NOT APPLE)
  target_link_libraries(GameServer PRIVATE Threads::Threads)
endif()

# Windows resources (optional)
if(WIN32)
  foreach(_exe IN ITEMS AuthServer CharServer ChatServer QueryServer MasterServer GameServer)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Server/${_exe}/${_exe}.rc")
      target_sources(${_exe} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Server/${_exe}/${_exe}.rc")
    endif()
  endforeach()
endif()

