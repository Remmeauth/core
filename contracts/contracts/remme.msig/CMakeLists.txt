add_contract(remme.msig remme.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/remme.msig.cpp)

target_include_directories(remme.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(remme.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( remme.msig PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
