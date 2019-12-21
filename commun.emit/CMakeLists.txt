add_contract(commun.emit commun.emit src/commun.emit.cpp)
target_include_directories(commun.emit.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.emit
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.list/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../commun.point/include
   ${CMAKE_SOURCE_DIR}/include
   )
