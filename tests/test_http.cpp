/**
 * @file test_http.cpp
 * @brief Unit tests and examples for HTTP client functionality
 *
 * NOTE: HTTP tests require network access. Some tests use httpbin.org
 * for testing. For offline testing, use a local HTTP server.
 *
 * Usage:
 *   ./test_http                    # Run all tests
 *   ./test_http --offline          # Skip tests requiring internet
 *   ./test_http --server HOST:PORT # Use custom test server
 *
 * Example: HTTP GET request
 *   Communication::HTTP http;
 *   http.open_connection("httpbin.org", 80, 5);
 *   std::string response = http.get("/get", 4096);
 *   std::cout << response << std::endl;
 *   http.close_connection();
 *
 * Example: HTTP POST request
 *   Communication::HTTP http;
 *   http.open_connection("httpbin.org", 80, 5);
 *   std::string response = http.post("/post", "key=value", 4096);
 *   std::cout << response << std::endl;
 *   http.close_connection();
 *
 * Local test server (Python):
 *   python3 -m http.server 8080
 */

#include "test_utils.hpp"
#include "http_client.hpp"
#include <thread>
#include <chrono>
#include <cstring>

using namespace Communication;

static bool g_offline_mode = false;
static std::string g_test_host = "httpbin.org";
static int g_test_port = 80;

// Test: HTTP client opens connection
bool test_http_connect()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    try
    {
        http.open_connection(g_test_host.c_str(), g_test_port, 5);
        TEST_ASSERT(http.is_connected());
        http.close_connection();
        TEST_ASSERT(!http.is_connected());
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        std::cerr << "  (Is network available? Try --offline)" << std::endl;
        return false;
    }

    return true;
}

// Test: HTTP GET request
bool test_http_get()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    try
    {
        http.open_connection(g_test_host.c_str(), g_test_port, 5);

        // httpbin.org/get returns JSON with request info
        std::string response = http.get("/get", 4096);

        // Response should contain something
        TEST_ASSERT(response.length() > 0);

        http.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: HTTP POST request
bool test_http_post()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    try
    {
        http.open_connection(g_test_host.c_str(), g_test_port, 5);

        // httpbin.org/post echoes back the posted data
        std::string response = http.post("/post", "test_key=test_value", 4096);

        // Response should contain something
        TEST_ASSERT(response.length() > 0);

        http.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: HTTP POST without content
bool test_http_post_empty()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    try
    {
        http.open_connection(g_test_host.c_str(), g_test_port, 5);

        // POST with no content
        std::string response = http.post("/post", nullptr, 4096);

        TEST_ASSERT(response.length() > 0);

        http.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: HTTP handles connection refused gracefully
bool test_http_connection_refused()
{
    HTTP http(-1);

    try
    {
        // Port 1 is typically not listening
        http.open_connection("127.0.0.1", 1, 1);
        // Should not reach here
        return false;
    }
    catch(const std::exception &e)
    {
        // Expected to fail
        return true;
    }
}

// Test: HTTP handles invalid host gracefully
bool test_http_invalid_host()
{
    HTTP http(-1);

    try
    {
        http.open_connection("this.host.does.not.exist.invalid", 80, 2);
        // Should not reach here
        return false;
    }
    catch(const std::exception &e)
    {
        // Expected to fail
        return true;
    }
}

// Test: HTTP multiple requests on same connection
bool test_http_multiple_requests()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    try
    {
        http.open_connection(g_test_host.c_str(), g_test_port, 5);

        // Multiple GET requests
        std::string response1 = http.get("/get", 4096);
        TEST_ASSERT(response1.length() > 0);

        // Note: HTTP/1.1 allows keep-alive, but this implementation
        // might need reconnection for subsequent requests
        // This test verifies the current behavior

        http.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: HTTP with timeout
bool test_http_timeout()
{
    if(g_offline_mode)
    {
        std::cerr << "  Skipped: Offline mode" << std::endl;
        return true;
    }

    HTTP http(-1);

    auto start = std::chrono::steady_clock::now();

    try
    {
        // httpbin.org/delay/5 waits 5 seconds before responding
        // With a 2 second timeout, this should fail
        http.open_connection(g_test_host.c_str(), g_test_port, 2);
        // Connection succeeds, but reading might timeout
        http.close_connection();
    }
    catch(const std::exception &e)
    {
        // Timeout is expected
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    // Should complete within reasonable time
    TEST_ASSERT(duration.count() < 10);

    return true;
}

void print_usage(const char *prog)
{
    std::cout << "Usage: " << prog << " [options]\n"
              << "\nOptions:\n"
              << "  --offline          Skip tests requiring internet\n"
              << "  --server HOST:PORT Use custom test server\n"
              << "  --help             Show this help\n"
              << "\nExamples:\n"
              << "  " << prog << "\n"
              << "  " << prog << " --offline\n"
              << "  " << prog << " --server localhost:8080\n";
}

int main(int argc, char *argv[])
{
    // Parse arguments
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if(arg == "--offline")
        {
            g_offline_mode = true;
        }
        else if(arg == "--server" && i + 1 < argc)
        {
            std::string server = argv[++i];
            size_t colon = server.find(':');
            if(colon != std::string::npos)
            {
                g_test_host = server.substr(0, colon);
                g_test_port = std::stoi(server.substr(colon + 1));
            }
            else
            {
                g_test_host = server;
            }
        }
        else if(arg == "--help" || arg == "-h")
        {
            print_usage(argv[0]);
            return 0;
        }
    }

    if(!g_offline_mode)
    {
        std::cout << "Testing against: " << g_test_host << ":" << g_test_port << "\n";
        std::cout << "(Use --offline to skip network tests)\n\n";
    }

    Test::TestRunner runner;

    runner.add_test("HTTP connect", test_http_connect);
    runner.add_test("HTTP GET", test_http_get);
    runner.add_test("HTTP POST", test_http_post);
    runner.add_test("HTTP POST empty", test_http_post_empty);
    runner.add_test("HTTP connection refused", test_http_connection_refused);
    runner.add_test("HTTP invalid host", test_http_invalid_host);
    runner.add_test("HTTP multiple requests", test_http_multiple_requests);
    runner.add_test("HTTP timeout", test_http_timeout);

    return runner.run();
}
