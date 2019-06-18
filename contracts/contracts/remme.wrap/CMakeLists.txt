add_contract(remme.wrap remme.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/remme.wrap.cpp)

target_include_directories(remme.wrap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(remme.wrap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( remme.wrap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
