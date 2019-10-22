file(GLOB HEADERS "include/graphene/wallet/*.hpp")
file(GLOB INLINES "*.inl")

find_package( Perl )
find_package( Doxygen )

if( PERL_FOUND AND DOXYGEN_FOUND )
  configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMAND ${DOXYGEN_EXECUTABLE}
                      DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${CMAKE_CURRENT_SOURCE_DIR}/include/graphene/wallet/wallet.hpp )
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${PERL_EXECUTABLE} -I${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp.new
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_api_documentation.pl ${CMAKE_CURRENT_BINARY_DIR}/doxygen/perlmod/DoxyDocs.pm )
else()
  # no perl and doxygen, generate the best docs we can at runtime from reflection
  add_custom_command( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api_documentation_standin.cpp )
endif()

add_library( graphene_wallet
             wallet.cpp
             wallet_utility.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/api_documentation.cpp
             ${INLINES}
             ${HEADERS}
           )

target_link_libraries( graphene_wallet PUBLIC graphene_app PRIVATE package_manager ipfs-api )
target_include_directories( graphene_wallet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( wallet.cpp wallet_utility.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

#install( TARGETS
#   graphene_wallet
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
