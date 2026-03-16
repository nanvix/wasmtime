// Copyright(c) The Maintainers of Nanvix.
// Licensed under the MIT License.

//! Wasmtime platform API implementation for Nanvix OS.
//!
//! Provides the C functions required by wasmtime's custom platform layer:
//! - TLS (thread-local storage) get/set
//! - setjmp/longjmp wrappers for trap handling
//! - mmap/munmap/mprotect for virtual memory management

#include <errno.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

// ============================================================================
// Virtual Memory
// ============================================================================

#define WASMTIME_PROT_READ  (1 << 0)
#define WASMTIME_PROT_WRITE (1 << 1)
#define WASMTIME_PROT_EXEC  (1 << 2)

static int wasmtime_to_mmap_prot_flags(uint32_t prot_flags) {
    int flags = 0;
    if (prot_flags & WASMTIME_PROT_READ)
        flags |= PROT_READ;
    if (prot_flags & WASMTIME_PROT_WRITE)
        flags |= PROT_WRITE;
    if (prot_flags & WASMTIME_PROT_EXEC)
        flags |= PROT_EXEC;
    return flags;
}

int wasmtime_mmap_new(uintptr_t size, uint32_t prot_flags, uint8_t **ret) {
    void *rc = mmap(NULL, size, wasmtime_to_mmap_prot_flags(prot_flags),
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (rc == MAP_FAILED)
        return errno;
    *ret = rc;
    return 0;
}

int wasmtime_mmap_remap(uint8_t *addr, uintptr_t size, uint32_t prot_flags) {
    void *rc = mmap(addr, size, wasmtime_to_mmap_prot_flags(prot_flags),
                    MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (rc == MAP_FAILED)
        return errno;
    return 0;
}

int wasmtime_munmap(uint8_t *ptr, uintptr_t size) {
    int rc = munmap(ptr, size);
    if (rc != 0)
        return errno;
    return 0;
}

int wasmtime_mprotect(uint8_t *ptr, uintptr_t size, uint32_t prot_flags) {
    // Nanvix doesn't enforce NX bits — all user pages are effectively RWX.
    // The kernel's mprotect hangs on permission downgrades, so use a no-op.
    (void)ptr; (void)size; (void)prot_flags;
    return 0;
}

uintptr_t wasmtime_page_size(void) {
    // Nanvix uses 4KB pages. sysconf(_SC_PAGESIZE) is not supported.
    return 4096;
}

// Memory images are not supported; return NULL to indicate no image.
struct wasmtime_memory_image;

int wasmtime_memory_image_new(const uint8_t *ptr, uintptr_t len,
                              struct wasmtime_memory_image **ret) {
    *ret = NULL;
    return 0;
}

int wasmtime_memory_image_map_at(struct wasmtime_memory_image *image,
                                 uint8_t *addr, uintptr_t len) {
    abort();
}

void wasmtime_memory_image_free(struct wasmtime_memory_image *image) {
    abort();
}

// ============================================================================
// Trap Handling (setjmp/longjmp)
// ============================================================================

bool wasmtime_setjmp(const uint8_t **jmp_buf_out,
                     bool (*callback)(uint8_t *, uint8_t *),
                     uint8_t *payload, uint8_t *callee) {
    jmp_buf buf;
    if (setjmp(buf) != 0)
        return false;
    *jmp_buf_out = (uint8_t *)&buf;
    return callback(payload, callee);
}

void wasmtime_longjmp(const uint8_t *jmp_buf_ptr) {
    longjmp(*(jmp_buf *)jmp_buf_ptr, 1);
}

// ============================================================================
// Thread-Local Storage (single-threaded guest)
// ============================================================================

static uint8_t *WASMTIME_TLS = NULL;

uint8_t *wasmtime_tls_get(void) {
    return WASMTIME_TLS;
}

void wasmtime_tls_set(uint8_t *val) {
    WASMTIME_TLS = val;
}
