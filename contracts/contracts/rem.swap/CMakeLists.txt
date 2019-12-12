add_contract(rem.swap rem.swap
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.swap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system_info.cpp
)

target_include_directories(rem.swap
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.system/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.utils/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.token/include
)

set_target_properties(rem.swap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/rem.swap.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/rem.swap.contracts.md @ONLY )

target_compile_options( rem.swap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
