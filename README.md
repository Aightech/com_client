# com_client

A cross-platform C++ library providing a unified interface for serial, TCP, UDP, and HTTP communication protocols.

## Features

- **Unified API** - Common `Client` base class for all protocols with `open_connection()`, `readS()`, `writeS()`
- **Server support** - TCP and UDP servers with callback-based event handling
- **Cross-platform** - Windows, Linux, macOS
- **Thread-safe** - Mutex-protected operations for concurrent access
- **CRC16 checksum** - Built-in data integrity verification

## Supported Protocols

| Protocol | Client | Server |
|----------|--------|--------|
| Serial   | Yes    | -      |
| TCP      | Yes    | Yes    |
| UDP      | Yes    | Yes    |
| HTTP     | Yes (GET/POST) | - |

## Dependencies

- [strANSIseq](lib/strANSIseq/README.md) - ANSI terminal formatting (git submodule)

## Building

```bash
# Setup (initializes submodules)
./scripts/setup.sh

# Build library
cd build
cmake .. && make

# Build with demo app
cmake .. -DBUILD_EXAMPLES=ON && make
```

See [DEPENDENCIES.md](DEPENDENCIES.md) for submodule workflow details.

## Testing

```bash
# Build with tests
cmake .. -DBUILD_TESTS=ON && make

# Run all automatic tests
make run_tests

# Run individual tests
./tests/test_crc      # CRC checksum tests
./tests/test_tcp      # TCP client/server tests
./tests/test_udp      # UDP client/server tests
./tests/test_serial   # Serial tests (requires hardware or virtual port)
./tests/test_http     # HTTP tests (requires network)
```

Test files in `tests/` also serve as usage examples.

## Quick Example

```cpp
#include "tcp_client.hpp"

// TCP Client
Communication::TCP client;
client.open_connection("192.168.1.100", 8080, 5);  // 5s timeout
client.writeS("Hello", 5);
uint8_t buffer[256];
client.readS(buffer, 256);
client.close_connection();

// TCP Server with callback
Communication::TCPServer server(8080);
server.set_callback([](auto* srv, uint8_t* data, size_t len, void* addr, void* user) {
    // Handle received data
});
server.start();
```

## Demo Application

```bash
./com_client -h
```

See [src/main.cpp](src/main.cpp) for full usage examples.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
