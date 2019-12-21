add_contract(commun.ctrl commun.ctrl src/commun.ctrl.cpp)
target_include_directories(commun.ctrl.wasm
   PUBLIC
   ${TOKEN_INCLUDE_PATH}
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/cyber.bios/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..
   ${CMAKE_CURRENT_SOURCE_DIR}/../include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.emit/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include)
