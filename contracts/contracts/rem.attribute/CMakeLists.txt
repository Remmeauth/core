add_contract(rem.attribute rem.attribute ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.attribute.cpp)

target_include_directories(rem.attribute
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.attribute
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.attribute PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
