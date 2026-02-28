set(_DBO_THIRD_PARTY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(dbo_configure_mysqlclient)
  # Provides imported target dbo::mysqlclient
  if(TARGET dbo::mysqlclient)
    return()
  endif()

  set(DBO_MYSQL_INCLUDE_DIR "" CACHE PATH "MySQL include directory")
  set(DBO_MYSQL_LIBRARY "" CACHE FILEPATH "MySQL client library")

  if(WIN32)
    if(NOT DBO_MYSQL_INCLUDE_DIR)
      set(DBO_MYSQL_INCLUDE_DIR "${_DBO_THIRD_PARTY_CMAKE_DIR}/../extern/mysql57")
    endif()
    if(NOT DBO_MYSQL_LIBRARY)
      set(DBO_MYSQL_LIBRARY "${_DBO_THIRD_PARTY_CMAKE_DIR}/../extern/mysql57/lib/libmysql.lib")
    endif()
  else()
    # Linux: default to system MySQL (override with -D if needed)
    if(NOT DBO_MYSQL_INCLUDE_DIR)
      set(DBO_MYSQL_INCLUDE_DIR "/usr/include/mysql")
    endif()
    if(NOT DBO_MYSQL_LIBRARY)
      set(DBO_MYSQL_LIBRARY "/usr/lib/x86_64-linux-gnu/libmysqlclient.so")
    endif()
  endif()

  # Allow either direct folder containing mysql.h or a folder that contains an 'include' subfolder.
  set(_dbo_mysql_header "")
  if(DBO_MYSQL_INCLUDE_DIR)
    if(EXISTS "${DBO_MYSQL_INCLUDE_DIR}/mysql.h")
      set(_dbo_mysql_header "${DBO_MYSQL_INCLUDE_DIR}/mysql.h")
    elseif(EXISTS "${DBO_MYSQL_INCLUDE_DIR}/include/mysql.h")
      set(DBO_MYSQL_INCLUDE_DIR "${DBO_MYSQL_INCLUDE_DIR}/include")
      set(_dbo_mysql_header "${DBO_MYSQL_INCLUDE_DIR}/mysql.h")
    endif()
  endif()

  if(NOT _dbo_mysql_header)
    message(FATAL_ERROR "MySQL headers not found. Set DBO_MYSQL_INCLUDE_DIR to a folder containing mysql.h (or containing include/mysql.h).")
  endif()
  if(NOT DBO_MYSQL_LIBRARY OR NOT EXISTS "${DBO_MYSQL_LIBRARY}")
    message(FATAL_ERROR "MySQL client library not found. Set DBO_MYSQL_LIBRARY (e.g. libmysqlclient.so or libmysql.lib).")
  endif()

  add_library(dbo_mysqlclient UNKNOWN IMPORTED)
  set_target_properties(dbo_mysqlclient PROPERTIES
    IMPORTED_LOCATION "${DBO_MYSQL_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${DBO_MYSQL_INCLUDE_DIR}"
  )

  add_library(dbo::mysqlclient ALIAS dbo_mysqlclient)
endfunction()

