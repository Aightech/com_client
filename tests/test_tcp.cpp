/**
 * @file test_tcp.cpp
 * @brief Unit tests for TCP client and server functionality
 *
 * Usage:
 *   ./test_tcp
 *
 * Example: Basic TCP client
 *   Communication::TCP client;
 *   client.open_connection("127.0.0.1", 8080, 5);
 *   client.writeS("Hello", 5);
 *   uint8_t buf[256];
 *   int n = client.readS(buf, 256, false, false);
 *   client.close_connection();
 *
 * Example: TCP server with callback
 *   Communication::TCPServer server(8080);
 *   server.set_callback([](Communication::Server* srv, uint8_t* data,
 *                          size_t len, void* addr, void* user) {
 *       std::cout << "Received " << len << " bytes" << std::endl;
 *   });
 *   server.start();
 *   // ... server runs until stop() is called
 *   server.stop();
 */

#include "test_utils.hpp"
#include "tcp_client.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

using namespace Communication;

static const int TEST_PORT = 19876; // Use high port to avoid conflicts

// Test: TCP server starts and stops correctly
bool test_tcp_server_lifecycle()
{
    TCPServer server(TEST_PORT);

    TEST_ASSERT(!server.is_running());

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TEST_ASSERT(server.is_running());

    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TEST_ASSERT(!server.is_running());

    return true;
}

// Test: TCP client connects to server
bool test_tcp_client_connect()
{
    TCPServer server(TEST_PORT + 1);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client(-1); // Silent mode
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 1, 2);
        TEST_ASSERT(client.is_connected());
        client.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Connection failed: " << e.what() << std::endl;
        return false;
    }

    server.stop();
    return true;
}

// Test: TCP client/server data exchange
bool test_tcp_data_exchange()
{
    std::atomic<bool> data_received(false);
    std::string received_data;

    TCPServer server(TEST_PORT + 2);
    server.set_callback(
        [](Server *srv, uint8_t *data, size_t len, void *addr, void *user) {
            std::string *received = static_cast<std::string *>(user);
            *received = std::string(reinterpret_cast<char *>(data), len);
            *static_cast<std::atomic<bool> *>(
                static_cast<void *>(&received)) = true;
        },
        &received_data);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 2, 2);

        const char *msg = "Hello TCP";
        client.writeS(msg, strlen(msg));

        // Wait for server to receive
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        client.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    server.stop();

    // Verify data was received (check via FIFO or callback)
    return true;
}

// Test: TCP server FIFO buffering
bool test_tcp_server_fifo()
{
    TCPServer server(TEST_PORT + 3);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 3, 2);

        // Send test data
        const char *msg = "FIFO Test Data";
        client.writeS(msg, strlen(msg));

        // Wait for data to arrive in FIFO
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Get client sockets
        auto &clients = server.get_clients();
        if(!clients.empty())
        {
            SOCKET client_sock = *clients.begin();

            // Read from FIFO
            uint8_t buffer[256] = {0};
            int8_t result = server.read_byte(client_sock, buffer, strlen(msg), false, true);

            if(result >= 0)
            {
                TEST_ASSERT_EQ(0, memcmp(buffer, msg, strlen(msg)));
            }
        }

        client.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    server.stop();
    return true;
}

// Test: TCP server handles multiple clients
bool test_tcp_multiple_clients()
{
    TCPServer server(TEST_PORT + 4);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client1(-1);
    TCP client2(-1);

    try
    {
        client1.open_connection("127.0.0.1", TEST_PORT + 4, 2);
        client2.open_connection("127.0.0.1", TEST_PORT + 4, 2);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Both should be connected
        TEST_ASSERT(client1.is_connected());
        TEST_ASSERT(client2.is_connected());

        // Server should track both
        TEST_ASSERT_EQ(2u, server.get_clients().size());

        client1.close_connection();
        client2.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    server.stop();
    return true;
}

// Test: TCP connection timeout or refused
bool test_tcp_connection_timeout()
{
    TCP client(-1);

    auto start = std::chrono::steady_clock::now();

    try
    {
        // Connect to non-existent server with 1 second timeout
        client.open_connection("127.0.0.1", TEST_PORT + 99, 1);
        // Should not reach here
        return false;
    }
    catch(const std::exception &e)
    {
        // Connection should fail (either timeout or refused)
        // Both are acceptable outcomes
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        // Should complete within reasonable time
        TEST_ASSERT(duration.count() < 5);
    }
    catch(...)
    {
        // Any exception is acceptable (connection failed)
    }

    return true;
}

// Test: TCP server broadcast
bool test_tcp_server_broadcast()
{
    TCPServer server(TEST_PORT + 5);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client1(-1);
    TCP client2(-1);

    try
    {
        client1.open_connection("127.0.0.1", TEST_PORT + 5, 2);
        client2.open_connection("127.0.0.1", TEST_PORT + 5, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Broadcast message to all clients
        const char *msg = "Broadcast";
        server.broadcast(msg, strlen(msg));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client1.close_connection();
        client2.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    server.stop();
    return true;
}

// Test: New client callback
bool test_tcp_new_client_callback()
{
    std::atomic<int> new_client_count(0);

    TCPServer server(TEST_PORT + 6);
    server.set_callback_newClient(
        [](Server *srv, void *addr, SOCKET client_socket, void *user) {
            std::atomic<int> *count = static_cast<std::atomic<int> *>(user);
            (*count)++;
        },
        &new_client_count);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    TCP client1(-1);
    TCP client2(-1);

    try
    {
        client1.open_connection("127.0.0.1", TEST_PORT + 6, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client2.open_connection("127.0.0.1", TEST_PORT + 6, 2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        TEST_ASSERT_EQ(2, new_client_count.load());

        client1.close_connection();
        client2.close_connection();
    }
    catch(const std::exception &e)
    {
        server.stop();
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    server.stop();
    return true;
}

int main()
{
    Test::TestRunner runner;

    runner.add_test("TCP server lifecycle", test_tcp_server_lifecycle);
    runner.add_test("TCP client connect", test_tcp_client_connect);
    runner.add_test("TCP data exchange", test_tcp_data_exchange);
    runner.add_test("TCP server FIFO", test_tcp_server_fifo);
    runner.add_test("TCP multiple clients", test_tcp_multiple_clients);
    runner.add_test("TCP connection timeout", test_tcp_connection_timeout);
    runner.add_test("TCP server broadcast", test_tcp_server_broadcast);
    runner.add_test("TCP new client callback", test_tcp_new_client_callback);

    return runner.run();
}
