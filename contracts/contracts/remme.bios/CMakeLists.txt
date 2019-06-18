add_contract(remme.bios remme.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/remme.bios.cpp)

target_include_directories(remme.bios
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(remme.bios
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( remme.bios PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
