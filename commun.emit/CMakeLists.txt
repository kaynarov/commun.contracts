add_contract_with_abi(commun.emit abi/commun.emit.abi src/commun.emit.cpp)
target_include_directories(commun.emit.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.emit
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_SOURCE_DIR}/include
   )
