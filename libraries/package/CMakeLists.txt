
file( GLOB HEADERS
        "include/graphene/package/*.hpp" )

add_library( package_manipulation
             package.cpp
           )

target_link_libraries( package_manipulation fc decent_encrypt )
target_include_directories( package_manipulation
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   package_manipulation

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
