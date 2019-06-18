add_contract(remme.token remme.token ${CMAKE_CURRENT_SOURCE_DIR}/src/remme.token.cpp)

target_include_directories(remme.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(remme.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( remme.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
