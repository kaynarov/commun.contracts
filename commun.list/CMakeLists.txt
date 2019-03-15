set(CMAKE_CXX_FLAGS " -abigen ${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(commun.list.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/commun.list.cpp)
target_include_directories(commun.list.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.list
   ${CMAKE_CURRENT_SOURCE_DIR}/../bancor.token/include/bancor.token
   ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(commun.list.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/commun.list.abi" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
