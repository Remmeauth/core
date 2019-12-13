add_contract(rem.oracle rem.oracle ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.oracle.cpp)

target_include_directories(rem.oracle
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.system/include)

set_target_properties(rem.oracle
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.oracle PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
