set(CMAKE_CXX_FLAGS " -abigen ${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(registrar.wasm ${CMAKE_CURRENT_SOURCE_DIR}/registrar.cpp)
target_include_directories(registrar.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/../cyberway.contracts/eosio.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../bancor.token/include
   ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(registrar.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

file(GLOB ABI_FILES "*.abi")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
