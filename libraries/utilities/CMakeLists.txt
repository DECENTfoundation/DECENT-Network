list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/fc/GitVersionGen" )
include( GetGitRevisionDescription )
git_describe(GRAPHENE_GIT_REVISION_DESCRIPTION --tags)
if(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)
    message( FATAL_ERROR "Could not get Git revision" )
endif()

file(GLOB HEADERS "include/decent/*.hpp" "include/graphene/utilities/*.hpp")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)

add_library( graphene_utilities
             decent_config.cpp
             key_conversion.cpp
             keys_generator.cpp
             string_escape.cpp
             dirhelper.cpp
             time.cpp
             words.cpp
             about.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_utilities PUBLIC fc )
target_include_directories( graphene_utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   graphene_utilities
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
