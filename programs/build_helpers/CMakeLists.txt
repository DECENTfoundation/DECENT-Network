
file( GLOB HEADERS "*.hpp" )

# We only actually need Boost, but link against FC for now so we don't duplicate it.

add_executable( cat-parts
                cat-parts.cpp
                ${HEADERS} )
target_link_libraries( cat-parts
                       PRIVATE fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

add_executable( member_enumerator
                member_enumerator.cpp
                ${HEADERS} )
target_link_libraries( member_enumerator
                       PRIVATE graphene_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
