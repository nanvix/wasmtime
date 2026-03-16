/// Precompiles a WAT module to Pulley32 bytecode for Nanvix.
///
/// Usage: precompile-wasm [output.cwasm]
/// Generates a precompiled module that can be deserialized on Nanvix
/// without needing Cranelift compilation at runtime.

fn main() -> anyhow::Result<()> {
    let output = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "hello.cwasm".to_string());

    let wat = r#"
        (module
            (func (export "hello") (result i32)
                i32.const 42
            )
        )
    "#;

    let mut config = wasmtime::Config::new();
    config.target("pulley32")?;
    // Use small memory sizes safe for 32-bit usize
    config.memory_reservation(1 << 20); // 1 MiB
    config.memory_guard_size(4096); // 4 KiB
    config.memory_reservation_for_growth(65536); // 64 KiB
    config.guard_before_linear_memory(false);
    let engine = wasmtime::Engine::new(&config)?;

    eprintln!("Precompiling WAT module for pulley32...");
    let wasm = wat::parse_str(wat)?;
    let precompiled = engine.precompile_module(&wasm)?;

    std::fs::write(&output, &precompiled)?;
    eprintln!("Wrote {} bytes to {}", precompiled.len(), output);
    Ok(())
}
