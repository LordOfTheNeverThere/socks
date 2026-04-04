A C++ wrapper library on the original C sockets API, with both UDP, TCP and Raw Socket features, as well as a class with some Tool routines.

## 🗺️️ Prerequisites

Before building the project, ensure you have the following installed:

* **CMake** (v3.28 or higher)
* **C++ Compiler** (supporting C++17)

## 🪢 Linking to your project
On your CMakeLists.txt at the root of the project
```cmake
include(FetchContent)
# Fetch socks library
FetchContent_Declare(
        socks
        GIT_REPOSITORY https://github.com/LordOfTheNeverThere/socks.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(socks)

(...)

# Link against the fetched socks library
target_link_libraries(yourTarget PUBLIC socks)

## Add this to your target_include_directories:
$<BUILD_INTERFACE:${socks_BINARY_DIR}/generated>
```

On your CMakeLists.txt on the tests folder add:

```cmake
target_compile_definitions(networkMapperTests PRIVATE
        SOCKS_RESOURCES="${socks_BINARY_DIR}/resources/"
)
```
