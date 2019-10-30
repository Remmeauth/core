add_contract(rem.swap rem.swap ${CMAKE_CURRENT_SOURCE_DIR}/src/rem.swap.cpp)

target_include_directories(rem.swap
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.system/include)

target_include_directories(rem.swap
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../rem.token/include)

target_include_directories(rem.swap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(rem.swap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( rem.swap PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian )
