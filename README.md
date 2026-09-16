# utils

Internal C utility library (vec, str, fs, alloc, ...) I now keep in git so my projects can pull it as a submodule. Feel free to do the same.

## Build & test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Optional:

- `-DBUILD_SHARED=ON`
- `-DCUSTOM_FS=ON` (your own `fs_platform_*`; tests off by default)
- `-DUTILS_DEFAULT_ALLOCATOR=tlsf` (TLSF instead of libc malloc)
- `-DUTILS_ALLOC_TLSF=OFF` (drop the TLSF module)

## Use as a git submodule

```bash
git submodule add git@github.com:mathijs-follon/c-utils.git third_party/utils
git submodule update --init --recursive
```

In your `CMakeLists.txt`:

```cmake
add_subdirectory(third_party/utils)
target_link_libraries(your_app PRIVATE utils_static)  # or utils_shared
```

Headers live under `include/` (`vec.h`, `str.h`, `fs.h`, ...).

## Allocators

All library allocations go through `utils_malloc` / `utils_realloc` / `utils_free` (`alloc.h`).

Backends are modules (`alloc_module.h`):

- **system**: libc `malloc` (default)
- **tlsf**: Two-Level Segregated Fit (optional; good for deterministic / embedded use)

Pick the default at configure time with `-DUTILS_DEFAULT_ALLOCATOR=system|tlsf`, or switch at runtime with `utils_alloc_module_use("tlsf")`.
Feel free to open a pull request to add more allocator modules.

For a fixed pool (e.g. STM32, no libc heap growth):

```c
alignas(8) static u8 pool[32 * 1024];
UtilsTlsf heap;
utils_tlsf_init_static(&heap, pool, sizeof(pool));
UtilsAllocator allocator = utils_tlsf_allocator(&heap);
utils_allocator_set(&allocator);
```
