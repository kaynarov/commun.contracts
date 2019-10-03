add_contract_with_abi(commun.list abi/commun.list.abi src/commun.list.cpp)
target_include_directories(commun.list.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.list
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   )
