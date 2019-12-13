add_contract(rem.auth rem.auth
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.auth.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.attr/src/rem.attr.cpp
)

target_include_directories(rem.auth
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.oracle/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.swap/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.token/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.attr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.swap/src
)

set_target_properties(rem.auth
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.auth PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
