use cmake;
fn main() {
    let dst = cmake::Config::new("lib")
        .generator("Ninja")
        .build();
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=emulator");

    #[cfg(target_os = "macos")]
    println!("cargo:rustc-link-lib=dylib=c++");

    #[cfg(not(target_os = "macos"))]
    println!("cargo:rustc-link-lib=dylib=stdc++");
}