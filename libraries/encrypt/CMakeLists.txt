file(GLOB HEADERS "include/decent/encrypt/*.hpp")

add_library( decent_encrypt
             encryptionutils.cpp
             ${HEADERS}
        )

# need to link graphene_debug_witness because plugins aren't sufficiently isolated #246
target_link_libraries( decent_encrypt fc )
target_include_directories( decent_encrypt
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

INSTALL( TARGETS
   decent_encrypt

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
