# com_client

This C++ library provides a unified interface for different types of communication modes (serial, TCP, and UDP). It is designed to be cross-platform. The library allows you to open, close, read from, and write to a connection. It also implements CRC16 checksum functionality.

# Features
- Establish connections in various modes: Serial, TCP, or UDP.
- Cross-platform support (Windows, Linux, macOS).
- Methods for opening and closing connections.
- Methods for reading from and writing to the connection.
- CRC16 checksum calculation.
- Control over the blocking/non-blocking nature of the socket.

# Building source code

To build the project run:
```bash
cd com_client
mkdir build && cd build -DBUILD_EXAMPLES=ON
cmake .. && make
```

# Demonstration app

When the project have been built, you can run:
```bash
./com_client -h
```
to get the demonstration app usage.

# Example
Open the ![main.cpp](cpp:src/main.cpp) file to get an example how to use the lib.
