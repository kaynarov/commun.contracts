add_contract_with_abi(golos.publication golos.publication.abi golos.publication.cpp)
target_include_directories(golos.publication.wasm
   PUBLIC
   ${TOKEN_INCLUDE_PATH}
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.social/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..)
