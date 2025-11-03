use std::path::Path;
use cmake;
fn main() {
    let library_path = Path::new("../../../lib");
    let dst = cmake::Config::new(library_path)
        .generator("Ninja")
        .build();
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=impl");

    #[cfg(target_os = "macos")]
    println!("cargo:rustc-link-lib=dylib=c++");

    #[cfg(not(target_os = "macos"))]
    println!("cargo:rustc-link-lib=dylib=stdc++");

    println!("cargo:rerun-if-changed={}", library_path.display());
}