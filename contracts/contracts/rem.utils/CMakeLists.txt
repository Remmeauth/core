add_contract(rem.utils rem.utils
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/validate_address.cpp
)

target_include_directories(rem.utils
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.utils
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.utils PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
