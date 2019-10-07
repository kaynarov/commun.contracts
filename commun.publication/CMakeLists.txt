add_contract_with_abi(commun.publication abi/commun.publication.abi src/commun.publication.cpp)
target_include_directories(commun.publication.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${TOKEN_INCLUDE_PATH}
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.gallery/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..)
