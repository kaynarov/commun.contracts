set(CMAKE_CXX_FLAGS " -abigen ${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(commun.token.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/commun.token.cpp)
target_include_directories(commun.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.token
   ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(commun.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/commun.token.abi" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

