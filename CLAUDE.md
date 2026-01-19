# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Modular Library Ecosystem

This project is part of a larger ecosystem of reusable C++ library blocks (networking, debugging, etc.). All projects share the same CMakeLists.txt pattern and use git submodules for dependencies.

### Recursive Build Mechanism

The CMake system automatically discovers and builds subprojects:

1. **`cmake/PullingLibs.cmake`** scans `lib/` directory for subdirectories
2. Each subdirectory with a CMakeLists.txt is added via `add_subdirectory()`
3. Subprojects run the same mechanism recursively (unlimited nesting)
4. Library names accumulate in `EXTRA_LIBS` and are linked to the parent

**Key files:**
- `CMakeConfig.cmake` - Project name and version (`EXEC_NAME`, `PROJECT_VERSION`)
- `cmake/PullingLibs.cmake` - Recursive subproject loader
- `cmake/CMakeFunctions.cmake` - Helper macros (`subdirlist`, `subproject_version`)

**Adding a dependency:** Add a git submodule to `lib/` - the build system handles the rest.

**Example hierarchy (CleverHand-interface):**
```
CleverHand-interface/
└── lib/
    ├── com_client/         ← this project
    │   └── lib/strANSIseq/
    └── strANSIseq/
```

## Build Commands

```bash
# First time setup (initializes submodules)
./scripts/setup.sh

# Build the library
cd build
cmake .. && make

# Build with examples (demo CLI app)
cmake .. -DBUILD_EXAMPLES=ON && make

# Run demo app
./com_client -h
```

**Note:** CMake will fail with a clear error if submodules aren't initialized. See `DEPENDENCIES.md` for submodule workflow.

```bash
# Build with tests
cmake .. -DBUILD_TESTS=ON && make
./tests/test_crc && ./tests/test_tcp && ./tests/test_udp
```

The build produces both shared (`com_client.9.3`) and static (`com_client`) library targets.

## Architecture

This is a cross-platform C++ communication library providing a unified interface for serial, TCP, UDP, and HTTP protocols.

### Core Abstractions (namespace `Communication`)

**Base Classes** in `include/com_client.hpp`:
- `Client` - Abstract base for all communication clients. Pure virtual methods: `open_connection()`, `readS()`, `writeS()`. Provides CRC16 checksum support and mutex-protected operations.
- `Server` - Abstract base for servers with `start()`, `stop()`, callback system (`set_callback()`, `set_callback_newClient()`), and client tracking.

**Implementations**:
- `Serial` (`serial_client.hpp/cpp`) - Serial port communication
- `TCP` + `TCPServer` (`tcp_client.hpp/cpp`) - Thread-per-client model with FIFO buffers, Nagle control, blocking/non-blocking reads
- `UDP` + `UDPServer` (`udp_client.hpp/cpp`) - Datagram communication with per-address FIFO queues and broadcast support
- `HTTP` (`http_client.hpp/cpp`) - GET/POST requests extending TCP

### Dependencies

- **strANSIseq** (git submodule in `lib/`) - ANSI terminal formatting; all classes inherit from `ESC::CLI`

### Platform Support

Windows (Winsock2), Linux (POSIX sockets), macOS (POSIX + IOKit for serial).

## Code Style

- Allman braces, 4-space indentation
- snake_case for methods, PascalCase for classes, m_ prefix for members
- Strict compilation: `-Wall -Wextra -Wpedantic -Werror`
