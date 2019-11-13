add_contract(rem.attr rem.attr ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.attr.cpp)

target_include_directories(rem.attr
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.attr
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")


configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/rem.attr.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/rem.attr.contracts.md @ONLY )

target_compile_options( rem.attr PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
