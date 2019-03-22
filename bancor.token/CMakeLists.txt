set(CMAKE_CXX_FLAGS " -abigen ${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(bancor.token.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/bancor.token.cpp)
target_include_directories(bancor.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/cyber.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(bancor.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/bancor.token.abi" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
