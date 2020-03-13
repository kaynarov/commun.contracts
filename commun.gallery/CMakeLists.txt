add_contract_with_checked_abi(commun.gallery commun.gallery commun.gallery.abi src/commun.gallery.cpp)
install_contract(commun.gallery)
target_include_directories(commun.gallery.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_SOURCE_DIR}/include
   )
