find_package( PBC REQUIRED )
find_package( CryptoPP REQUIRED )

file( GLOB HEADERS
    "include/decent/encrypt/*.hpp"
    "${PBC_INCLUDE_DIR}/pbc/*.h" )

add_executable( test_encrypt test_encryption_utils.cpp ${HEADERS} )
add_library( decent_encrypt
             encryptionutils.cpp
             custodyutils.cpp
             ${HEADERS} )

target_link_libraries( decent_encrypt
    PUBLIC fc pbc cryptopp )

target_link_libraries( test_encrypt pbc graphene_chain decent_encrypt gmp )
target_include_directories( decent_encrypt
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )
target_include_directories( test_encrypt
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain/include" )

install( TARGETS
   decent_encrypt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
