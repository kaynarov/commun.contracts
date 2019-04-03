add_contract_with_abi(registrar registrar.abi registrar.cpp)
target_include_directories(registrar.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../golos.contracts/cyberway.contracts/cyber.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../bancor.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   )
