// Copyright(c) The Maintainers of Nanvix.
// Licensed under the MIT License.

fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();
    if target_os != "nanvix" {
        return;
    }

    println!("cargo:rerun-if-changed=wasmtime-platform.c");
    println!("cargo:rerun-if-changed=user.ld");

    let nanvix_toolchain = std::env::var("NANVIX_TOOLCHAIN")
        .unwrap_or_else(|_| "/opt/nanvix".to_string());

    // Compile the wasmtime platform C API implementation.
    cc::Build::new()
        .file("wasmtime-platform.c")
        .warnings(true)
        .compile("wasmtime-platform");

    // Custom linker script that merges .ctors/.got.plt into .data to avoid
    // overlapping LOAD segments on the same page (Nanvix kernel rejects these).
    let linker_script = std::env::var("NANVIX_LINKER_SCRIPT").unwrap_or_else(|_| {
        format!(
            "{}/user.ld",
            std::env::var("CARGO_MANIFEST_DIR").unwrap_or_default()
        )
    });
    println!("cargo:rustc-link-arg=-T{linker_script}");

    // Nanvix system C library (provides memcpy, memset, strlen, abort, etc.)
    println!(
        "cargo:rustc-link-arg=-L{}/i686-nanvix/lib/",
        nanvix_toolchain
    );
    println!("cargo:rustc-link-arg=-lc");
    println!("cargo:rustc-link-arg=-lm");

    // Compiler-RT builtins (provides __divdi3, __udivdi3, etc.)
    println!(
        "cargo:rustc-link-arg=-L{}/lib/clang/21/lib/i386-unknown-nanvix/",
        nanvix_toolchain
    );
    println!("cargo:rustc-link-arg=-lclang_rt.builtins");
}
