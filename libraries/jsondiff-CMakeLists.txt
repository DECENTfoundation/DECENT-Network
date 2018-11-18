file(GLOB HEADERS "jsondiff-cpp/include/jsondiff/*.h")

set(SOURCES 
jsondiff-cpp/jsondiff/diff_result.cpp  
jsondiff-cpp/jsondiff/helper.cpp  
jsondiff-cpp/jsondiff/json_value_types.cpp  
jsondiff-cpp/jsondiff/jsondiff.cpp
)

include_directories(jsondiff-cpp/include)

add_library( jsondiff ${SOURCES} ${HEADERS} )

target_link_libraries( jsondiff 
  PUBLIC fc)
target_include_directories( jsondiff 
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/jsondiff-cpp/include"
)

if (USE_PCH)
  set_target_properties(jsondiff PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(jsondiff)
endif(USE_PCH)

install( TARGETS
   jsondiff

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "jsondiff-cpp/include/jsondiff" )
