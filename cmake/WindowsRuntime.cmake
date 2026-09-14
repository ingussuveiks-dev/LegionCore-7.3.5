if(NOT WIN32)
  return()
endif()

get_filename_component(_legion_mysql_libdir "${MYSQL_LIBRARY}" DIRECTORY)
find_file(LEGION_MYSQL_DLL NAMES libmysql.dll
  HINTS "${_legion_mysql_libdir}" "${MYSQL_ROOT_DIR}/bin")
find_file(LEGION_CRYPTO_DLL NAMES libcrypto-3-x64.dll libcrypto-1_1-x64.dll libeay32.dll
  HINTS "${OPENSSL_ROOT_DIR}" "${OPENSSL_INCLUDE_DIR}/..")
find_file(LEGION_SSL_DLL NAMES libssl-3-x64.dll libssl-1_1-x64.dll ssleay32.dll
  HINTS "${OPENSSL_ROOT_DIR}" "${OPENSSL_INCLUDE_DIR}/..")
find_file(LEGION_LEGACY_DLL NAMES legacy.dll
  HINTS "${OPENSSL_ROOT_DIR}/bin" "${OPENSSL_ROOT_DIR}/lib/ossl-modules")
foreach(_dll IN ITEMS LEGION_MYSQL_DLL LEGION_CRYPTO_DLL LEGION_SSL_DLL LEGION_LEGACY_DLL)
  if(${_dll})
    install(FILES "${${_dll}}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endforeach()
