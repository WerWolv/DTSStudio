use cmake;
fn main() {
    let dst = cmake::Config::new("lib")
        .generator("Ninja")
        .build();
    println!("cargo:warning=cmake build {}", dst.display());
    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=emulator");
    println!("cargo:rustc-link-lib=dylib=stdc++");
}