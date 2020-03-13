add_contract_with_checked_abi(commun.emit commun.emit commun.emit.abi src/commun.emit.cpp)
install_contract(commun.emit)
target_include_directories(commun.emit.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.emit
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_SOURCE_DIR}/include
   )
