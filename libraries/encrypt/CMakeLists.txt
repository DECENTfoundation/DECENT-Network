file( GLOB HEADERS "include/decent/encrypt/*.hpp" )

add_library( decent_encrypt
             encryptionutils.cpp
             custodyutils.cpp
             ${HEADERS}
           )

target_link_libraries( decent_encrypt PUBLIC ${CRYPTOPP_NAME} fc PRIVATE gmp pbc )
target_include_directories( decent_encrypt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   decent_encrypt
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
