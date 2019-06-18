add_contract(remme.system remme.system ${CMAKE_CURRENT_SOURCE_DIR}/src/remme.system.cpp)

target_include_directories(remme.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../remme.token/include)

set_target_properties(remme.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_contract(rex.results rex.results ${CMAKE_CURRENT_SOURCE_DIR}/src/rex.results.cpp)

target_include_directories(rex.results
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rex.results
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.rex")

target_compile_options( remme.system PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
