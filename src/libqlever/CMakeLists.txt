add_library(qlever Qlever.cpp QleverWasmWrapper.cpp)
qlever_target_link_libraries(qlever parser engine util index absl::strings)

# WASM wrapper library for Emscripten compilation
add_library(qlever_wasm_wrapper INTERFACE)
target_include_directories(qlever_wasm_wrapper INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
