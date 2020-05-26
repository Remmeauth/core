# will be implictly used for any compilation unit if not overrided by SYSTEM_INCLUDE_FOLDERS parameter
# these directories go as -isystem <dir> to avoid warnings from code of third-party libraries

#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eosio.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/eosio.bios/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eosio.msig/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/eosio.msig/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eosio.system/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/eosio.system/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eosio.token/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/eosio.token/)
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/eosio.wrap/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/eosio.wrap/)

#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/old_versions/v1.6.0-rc3/eosio.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/old_versions/v1.6.0-rc3/eosio.bios/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.bios/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.system/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.system/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.token/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.token/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.wrap/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.wrap/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.msig/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.msig/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.auth/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.auth/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.attr/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.attr/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.swap/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.swap/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.utils/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.utils/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/rem.oracle/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/rem.oracle/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/old_versions/v1.7.0-develop-preactivate_feature/eosio.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/old_versions/v1.7.0-develop-preactivate_feature/eosio.bios/)
