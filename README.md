# utils

Internal C utility library (vec, str, fs, alloc, ...) I now keep in git so my projects can pull it as a submodule. Feel free to do the same.

## Build & test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Optional:

- `-DUTILS_BUILD_SHARED=ON`
- `-DCUSTOM_FS=ON` (your own `fs_platform_*`; tests off by default)
- `-DUTILS_DEFAULT_ALLOCATOR=tlsf` (TLSF instead of libc malloc)
- `-DUTILS_ALLOC_TLSF=OFF` (drop the TLSF module)

## Add to a CMake project

Link against `utils::utils` (static by default).

### Submodule

```bash
git submodule add git@github.com:mathijs-follon/c-utils.git third_party/utils
git submodule update --init --recursive
```

```cmake
add_subdirectory(third_party/utils)
target_link_libraries(your_app PRIVATE utils::utils)
```

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(utils
    GIT_REPOSITORY git@github.com:mathijs-follon/c-utils.git
    GIT_TAG main
)
FetchContent_MakeAvailable(utils)
target_link_libraries(your_app PRIVATE utils::utils)
```

### Installed package

```bash
cmake --install build --prefix /path/to/prefix
```

```cmake
find_package(utils 0.1 REQUIRED)
target_link_libraries(your_app PRIVATE utils::utils)
```

Also available: `utils::static`, `utils::shared` (if built).

When utils is pulled in as a subdirectory, its tests are off by default so they do not pollute your project.

Headers: `include/` (`vec.h`, `str.h`, `fs.h`, `alloc.h`, ...).

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
