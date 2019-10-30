file(GLOB HEADERS "include/eosio/rem_oracle_plugin/*.hpp")
add_library( rem_oracle_plugin
             http_client.cpp
             rem_oracle_plugin.cpp
             ${HEADERS} )

target_link_libraries( rem_oracle_plugin appbase fc http_plugin chain_plugin eosio_testing )
target_include_directories( rem_oracle_plugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                            rem_oracle_plugin PUBLIC ${CMAKE_SOURCE_DIR}/libraries/testing/include
                            rem_oracle_plugin PUBLIC ${CMAKE_BINARY_DIR}/unittests/include )
