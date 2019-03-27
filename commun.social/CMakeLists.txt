set(CMAKE_CXX_FLAGS " -abigen ${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(commun.social.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/commun.social.cpp)
target_include_directories(commun.social.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include/commun.social
   ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(commun.social.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/abi/commun.social.abi" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
