# Wasmtime Hello-Nanvix — build and run a WASM app on Nanvix OS
#
# Usage:
#   make precompile   — precompile WAT→Pulley32 bytecode (host)
#   make build        — build the Nanvix guest binary
#   make run          — run on Nanvix (standalone/microvm)
#   make all          — precompile + build + run
#   make clean        — remove build artifacts

WASMTIME_DIR    := $(shell cd ../.. && pwd)
NANVIX_DIR      := $(WASMTIME_DIR)/../nanvix
TARGET          := i686-unknown-nanvix
TOOLCHAIN       := nanvix-x86
ELF             := $(WASMTIME_DIR)/target/$(TARGET)/release/hello-nanvix.elf
PRECOMPILED     := hello.cwasm
PRECOMPILER     := $(WASMTIME_DIR)/target/release/precompile-wasm

# LLD linker — find dynamically under NANVIX_TOOLCHAIN (default: /opt/nanvix)
NANVIX_TOOLCHAIN ?= /opt/nanvix
LLD_PATH := $(shell find $(NANVIX_TOOLCHAIN) -name ld.lld -type f 2>/dev/null | head -1)

SHELL := /bin/bash
.PHONY: all precompile build postprocess run clean

all: precompile build run

# Step 1: Build the host precompiler and generate Pulley32 bytecode
precompile: $(PRECOMPILED)

$(PRECOMPILED): $(PRECOMPILER)
	$(PRECOMPILER) $(PRECOMPILED)

$(PRECOMPILER): ../precompile-wasm/src/main.rs ../precompile-wasm/Cargo.toml
	cd $(WASMTIME_DIR) && cargo +$(TOOLCHAIN) build --release -p precompile-wasm

# Step 2: Build the Nanvix guest binary
build: $(ELF)

$(ELF): src/main.rs Cargo.toml build.rs wasmtime-platform.c user.ld $(PRECOMPILED)
	cd $(WASMTIME_DIR) && CARGO_TARGET_I686_UNKNOWN_NANVIX_LINKER=$(LLD_PATH) \
		cargo +$(TOOLCHAIN) build --release --target $(TARGET) -p hello-nanvix
	$(MAKE) postprocess

# Step 3: Post-process ELF to fix overlapping LOAD segments
postprocess:
	objcopy --set-section-flags .bss=alloc,load,contents "$(ELF)" 2>/dev/null || true
	python3 -c "\
import struct; \
f = open('$(ELF)', 'r+b'); \
f.seek(28); e_phoff = struct.unpack('<I', f.read(4))[0]; \
f.seek(42); e_phentsize = struct.unpack('<H', f.read(2))[0]; \
e_phnum = struct.unpack('<H', f.read(2))[0]; \
phdrs = []; \
[f.seek(e_phoff + i * e_phentsize) or phdrs.append(list(struct.unpack('<IIIIIIII', f.read(32)))) for i in range(e_phnum)]; \
rw = [(i, p) for i, p in enumerate(phdrs) if p[0] == 1 and (p[6] & 2)]; \
exec( \
'if len(rw) >= 2:\n' \
'    i0, p0 = rw[0]; i1, p1 = rw[1]\n' \
'    if (p0[2] + p0[5] - 1) // 0x1000 >= p1[2] // 0x1000:\n' \
'        p0[4] = p1[1] + p1[4] - p0[1]; p0[5] = p1[2] + p1[5] - p0[2]\n' \
'        f.seek(e_phoff + i0 * e_phentsize); f.write(struct.pack(\"<IIIIIIII\", *p0))\n' \
'        p1[0] = 0; f.seek(e_phoff + i1 * e_phentsize); f.write(struct.pack(\"<IIIIIIII\", *p1))\n' \
); \
f.close()"

# Step 4: Run on Nanvix
# Note: timeout is needed because wasmtime's cleanup code blocks the normal
# process exit path. The WASM execution itself completes correctly.
# Console output is written to a temp file because nanvixd's -console-file
# writes binary data that needs post-processing with 'strings'.
run: $(ELF)
	@rm -f /tmp/.nanvix-console
	@cd $(NANVIX_DIR) && setsid --wait env RUST_LOG=warn timeout 15 \
		./bin/nanvixd.elf -console-file /tmp/.nanvix-console -- $(ELF) \
		</dev/null >/dev/null 2>&1; true
	@strings /tmp/.nanvix-console 2>/dev/null \
		| grep -E '^\[wasmtime\]|^Hello from|^hello\(\)|^WASM|^Result' \
		| sort -u
	@rm -f /tmp/.nanvix-console

clean:
	rm -f $(PRECOMPILED)
	cd $(WASMTIME_DIR) && cargo clean -p hello-nanvix -p precompile-wasm 2>/dev/null || true
