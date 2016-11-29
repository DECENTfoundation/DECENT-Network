file(GLOB HEADERS "include/decent/encrypt/*.hpp")
add_executable( test_encrypt test_encryption_utils.cpp ${HEADERS} )
add_library( decent_encrypt
             encryptionutils.cpp
             ${HEADERS}
        )

# need to link graphene_debug_witness because plugins aren't sufficiently isolated #246
target_link_libraries( decent_encrypt fc )
target_link_libraries( test_encrypt fc decent_encrypt ${CRYPTOPP_LIBRARIES} )
target_include_directories( decent_encrypt
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( test_encrypt
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
INSTALL( TARGETS
   decent_encrypt
        test_encrypt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
