add_contract_with_abi(commun.gallery abi/commun.gallery.abi src/commun.gallery.cpp)
target_include_directories(commun.gallery.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_SOURCE_DIR}/include
   )
