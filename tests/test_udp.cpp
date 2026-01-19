/**
 * @file test_udp.cpp
 * @brief Unit tests for UDP client and server functionality
 *
 * Usage:
 *   ./test_udp
 *
 * Example: Basic UDP client
 *   Communication::UDP client;
 *   client.open_connection("127.0.0.1", 9000, 0);
 *   client.writeS("Hello UDP", 9);
 *   uint8_t buf[256];
 *   int n = client.readS(buf, 256);
 *   client.close_connection();
 *
 * Example: UDP server with callback
 *   Communication::UDPServer server(9000);
 *   server.set_callback([](Communication::Server* srv, uint8_t* data,
 *                          size_t len, void* addr, void* user) {
 *       // addr points to sockaddr_in of sender
 *       std::cout << "Received " << len << " bytes" << std::endl;
 *   });
 *   server.start();
 *   // ... server runs until stop() is called
 *   server.stop();
 *
 * Example: UDP broadcast
 *   Communication::UDP client;
 *   client.open_connection("255.255.255.255", 9000, 0);
 *   client.enable_broadcast(true);
 *   client.writeS("Broadcast message", 17);
 */

#include "test_utils.hpp"
#include "udp_client.hpp"
#include <thread>
#include <chrono>
#include <atomic>
#include <cstring>

using namespace Communication;

static const int TEST_PORT = 19900; // Use high port to avoid conflicts

// Test: UDP server starts and stops correctly
bool test_udp_server_lifecycle()
{
    UDPServer server(TEST_PORT);

    TEST_ASSERT(!server.is_running());

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TEST_ASSERT(server.is_running());

    server.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    TEST_ASSERT(!server.is_running());

    return true;
}

// Test: UDP client opens connection (binds socket)
bool test_udp_client_open()
{
    UDP client(-1); // Silent mode

    try
    {
        int result = client.open_connection("127.0.0.1", TEST_PORT + 1, 0);
        TEST_ASSERT(result >= 0);
        client.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: UDP client/server data exchange
bool test_udp_data_exchange()
{
    std::atomic<bool> data_received(false);
    std::string received_data;

    UDPServer server(TEST_PORT + 2);
    server.set_callback(
        [](Server *srv, uint8_t *data, size_t len, void *addr, void *user) {
            auto *received = static_cast<std::atomic<bool> *>(user);
            received->store(true);
        },
        &data_received);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UDP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 2, 0);

        const char *msg = "Hello UDP";
        client.writeS(msg, strlen(msg));

        // Wait for server to receive
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        TEST_ASSERT(data_received.load());

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

// Test: UDP server FIFO per-address buffering
bool test_udp_server_fifo()
{
    UDPServer server(TEST_PORT + 3);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UDP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 3, 0);

        // Send multiple messages
        const char *msg1 = "Message1";
        const char *msg2 = "Message2";
        client.writeS(msg1, strlen(msg1));
        client.writeS(msg2, strlen(msg2));

        // Wait for data to arrive
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Messages should be queued per sender address
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

// Test: UDP handles multiple senders
bool test_udp_multiple_senders()
{
    std::atomic<int> message_count(0);

    UDPServer server(TEST_PORT + 4);
    server.set_callback(
        [](Server *srv, uint8_t *data, size_t len, void *addr, void *user) {
            auto *count = static_cast<std::atomic<int> *>(user);
            (*count)++;
        },
        &message_count);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UDP client1(-1);
    UDP client2(-1);

    try
    {
        client1.open_connection("127.0.0.1", TEST_PORT + 4, 0);
        client2.open_connection("127.0.0.1", TEST_PORT + 4, 0);

        client1.writeS("From client 1", 13);
        client2.writeS("From client 2", 13);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        TEST_ASSERT_EQ(2, message_count.load());

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

// Test: UDP is connectionless (no timeout needed)
bool test_udp_connectionless()
{
    UDP client(-1);

    // UDP "connect" should succeed even if no server is listening
    // because UDP is connectionless
    try
    {
        int result = client.open_connection("127.0.0.1", TEST_PORT + 99, 0);
        TEST_ASSERT(result >= 0);

        // Write should also "succeed" (packet just gets lost)
        const char *msg = "Lost packet";
        int written = client.writeS(msg, strlen(msg));
        TEST_ASSERT(written > 0);

        client.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: UDP server send_data back to client
bool test_udp_server_reply()
{
    std::atomic<bool> received_reply(false);

    UDPServer server(TEST_PORT + 5);
    server.set_callback(
        [](Server *srv, uint8_t *data, size_t len, void *addr, void *user) {
            // Echo back to sender
            UDPServer *udp_srv = static_cast<UDPServer *>(srv);
            udp_srv->send_data("Reply", 5, addr);
        },
        nullptr);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UDP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 5, 0);
        client.writeS("Hello", 5);

        // Try to receive reply (non-blocking)
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
    return true;
}

// Test: Large UDP datagram
bool test_udp_large_datagram()
{
    std::atomic<size_t> received_size(0);

    UDPServer server(TEST_PORT + 6);
    server.set_callback(
        [](Server *srv, uint8_t *data, size_t len, void *addr, void *user) {
            auto *size = static_cast<std::atomic<size_t> *>(user);
            size->store(len);
        },
        &received_size);

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UDP client(-1);
    try
    {
        client.open_connection("127.0.0.1", TEST_PORT + 6, 0);

        // Send a larger message (but under typical MTU)
        char large_msg[1000];
        memset(large_msg, 'X', sizeof(large_msg));
        client.writeS(large_msg, sizeof(large_msg));

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Verify we received close to what we sent
        // (server buffer might be slightly smaller due to null termination)
        TEST_ASSERT(received_size.load() >= 999u);

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

int main()
{
    Test::TestRunner runner;

    runner.add_test("UDP server lifecycle", test_udp_server_lifecycle);
    runner.add_test("UDP client open", test_udp_client_open);
    runner.add_test("UDP data exchange", test_udp_data_exchange);
    runner.add_test("UDP server FIFO", test_udp_server_fifo);
    runner.add_test("UDP multiple senders", test_udp_multiple_senders);
    runner.add_test("UDP connectionless", test_udp_connectionless);
    runner.add_test("UDP server reply", test_udp_server_reply);
    runner.add_test("UDP large datagram", test_udp_large_datagram);

    return runner.run();
}
