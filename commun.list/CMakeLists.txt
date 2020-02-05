add_contract(commun.list commun.list src/commun.list.cpp)
install_contract(commun.list)
target_include_directories(commun.list.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   )
