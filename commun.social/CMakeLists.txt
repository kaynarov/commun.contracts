add_contract_with_abi(commun.social abi/commun.social.abi src/commun.social.cpp)
target_include_directories(commun.social.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.social
   ${CMAKE_SOURCE_DIR}/include
   )
