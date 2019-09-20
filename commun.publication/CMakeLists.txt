add_contract_with_abi(commun.publication commun.publication.abi commun.publication.cpp)
target_include_directories(commun.publication.wasm
   PUBLIC
   ${TOKEN_INCLUDE_PATH}
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.social/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.gallery/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.ctrl/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..)
