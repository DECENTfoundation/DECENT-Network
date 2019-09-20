list( APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../contrib/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision(GIT_REFSPEC GRAPHENE_GIT_REVISION_SHA)
get_git_unix_timestamp(GRAPHENE_GIT_REVISION_UNIX_TIMESTAMP)
git_describe(GRAPHENE_GIT_REVISION_DESCRIPTION --tags)
if(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)
    set(GRAPHENE_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)

file(GLOB HEADERS "include/decent/*.hpp" "include/graphene/utilities/*.hpp")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)

add_library( graphene_utilities
             decent_config.cpp
             key_conversion.cpp
             keys_generator.cpp
             string_escape.cpp
             dirhelper.cpp
             json.cpp
             time.cpp
             words.cpp
             ipfs_check.cpp
             about.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp
             ${HEADERS}
           )

target_link_libraries( graphene_utilities PUBLIC fc PRIVATE ipfs-api )
target_include_directories( graphene_utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

#install( TARGETS
#   graphene_utilities
#
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
#)
