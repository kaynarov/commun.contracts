add_contract_with_abi(bancor.token abi/bancor.token.abi src/bancor.token.cpp)
target_include_directories(bancor.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/cyber.token/include
   ${CMAKE_SOURCE_DIR}/include
   )
