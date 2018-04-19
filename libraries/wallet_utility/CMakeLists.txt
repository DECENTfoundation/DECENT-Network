
file( GLOB HEADERS "*.hpp" "include/decent/wallet_utility/*.hpp" )

add_library( wallet_utility
             wallet_utility.cpp
             ${HEADERS} )

target_link_libraries( wallet_utility
        PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_decent graphene_utilities graphene_wallet package_manager decent_encrypt
        pbc
        fc
        ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS}
)

if( WIN32 )
  target_link_libraries( wallet_utility PRIVATE ${GMP_LIBRARIES} )
  file(GLOB MySrcFiles *.cpp)
  #set_property(SOURCE ${MySrcFiles} PROPERTY COMPILE_FLAGS "/bigobj")
  set_property(SOURCE ${MySrcFiles} PROPERTY COMPILE_FLAGS "/bigobj")
else()
  target_link_libraries( wallet_utility PRIVATE gmp)
endif()

target_include_directories( wallet_utility
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

#install( TARGETS
#   package_manager
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
