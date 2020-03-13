add_contract_with_checked_abi(commun.social commun.social commun.social.abi src/commun.social.cpp)
install_contract(commun.social)
target_include_directories(commun.social.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.social
   ${CMAKE_SOURCE_DIR}/include
   )
