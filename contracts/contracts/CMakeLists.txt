cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_subdirectory(remme.bios)
add_subdirectory(remme.msig)
add_subdirectory(remme.system)
add_subdirectory(remme.token)
add_subdirectory(remme.wrap)
