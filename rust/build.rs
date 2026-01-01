fn main() {
    // Link against QLever C++ library
    // When libqlever is built as a C-compatible library, uncomment below:

    // println!("cargo:rustc-link-search=native=../build/lib");
    // println!("cargo:rustc-link-lib=dylib=qlever");
    // println!("cargo:rustc-link-search=native=/usr/lib");
    // println!("cargo:rustc-link-search=native=/usr/local/lib");

    // For now, this is a placeholder for when libqlever C FFI is available
    println!("cargo:warning=libqlever C FFI not yet linked. Update build.rs when C++ wrapper is built.");
}
