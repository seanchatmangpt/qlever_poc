use std::path::Path;

fn main() {
    // Configure linking to QLever C wrapper
    //
    // This build script looks for the compiled C wrapper library and links it.
    // The C wrapper (src/qlever_c.cpp) must be compiled into a library first.

    let manifest_dir = env!("CARGO_MANIFEST_DIR");
    let parent_dir = Path::new(manifest_dir).parent().unwrap();
    let lib_search_path = parent_dir.join("build").join("lib");

    // Check if libraries have been compiled
    let qlever_c_static = lib_search_path.join("libqlever_c.a");
    let qlever_c_shared = lib_search_path.join("libqlever_c.so");

    if qlever_c_static.exists() {
        // Use static linking
        println!("cargo:rustc-link-search=native={}", lib_search_path.display());
        println!("cargo:rustc-link-lib=static=qlever_c");

        // Link main qlever library only if it exists
        let qlever_static = lib_search_path.join("libqlever.a");
        let qlever_shared = lib_search_path.join("libqlever.so");
        if qlever_static.exists() {
            println!("cargo:rustc-link-lib=static=qlever");
        } else if qlever_shared.exists() {
            println!("cargo:rustc-link-lib=dylib=qlever");
        }

        println!("cargo:rustc-link-lib=stdc++");  // C++ standard library
        println!("cargo:warning=Linked static libqlever_c");
    } else if qlever_c_shared.exists() {
        // Use dynamic linking
        println!("cargo:rustc-link-search=native={}", lib_search_path.display());
        println!("cargo:rustc-link-lib=dylib=qlever_c");

        // Link main qlever library only if it exists
        let qlever_static = lib_search_path.join("libqlever.a");
        let qlever_shared = lib_search_path.join("libqlever.so");
        if qlever_static.exists() {
            println!("cargo:rustc-link-lib=static=qlever");
        } else if qlever_shared.exists() {
            println!("cargo:rustc-link-lib=dylib=qlever");
        }

        println!("cargo:rustc-link-lib=stdc++");  // C++ standard library
        println!("cargo:warning=Linked dynamic libqlever_c");
    } else {
        // Libraries not yet compiled
        println!("cargo:warning=QLever C FFI not yet linked");
        println!("cargo:warning=To enable: compile src/qlever_c.cpp into a library");
        println!("cargo:warning=Expected paths:");
        println!("cargo:warning=  Static:  {}", qlever_c_static.display());
        println!("cargo:warning=  Shared:  {}", qlever_c_shared.display());
    }

    // Rerun if build.rs changes
    println!("cargo:rerun-if-changed=build.rs");
}
