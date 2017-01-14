

add_library( package_manager
             package.cpp
           )

target_link_libraries( package_manager fc decent_encrypt )
target_include_directories( package_manager
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   package_manager

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
