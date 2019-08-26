add_contract_with_abi(reserve.emit reserve.emit.abi reserve.emit.cpp)
target_include_directories(reserve.emit.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/cyber.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   ${CMAKE_SOURCE_DIR}/include
   )

