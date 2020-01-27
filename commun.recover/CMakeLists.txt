add_contract_with_abi(commun.recover abi/commun.recover.abi src/commun.recover.cpp)
target_include_directories(commun.recover.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/cyber.bios/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.recover/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   )
