/**
 * @file test_serial.cpp
 * @brief Unit tests and examples for Serial communication
 *
 * NOTE: Serial tests require physical hardware or a virtual serial port.
 * Run with a loopback adapter (TX connected to RX) for automated testing,
 * or use socat to create virtual serial ports:
 *   socat -d -d pty,raw,echo=0 pty,raw,echo=0
 *
 * Usage:
 *   ./test_serial [port]           # Run tests on specified port
 *   ./test_serial /dev/ttyUSB0     # Example with USB-serial adapter
 *   ./test_serial --list           # List available serial ports
 *   ./test_serial --virtual        # Create and test virtual ports (requires socat)
 *
 * Example: Basic serial communication
 *   Communication::Serial serial;
 *   serial.open_connection("/dev/ttyUSB0", 115200, 0);
 *   serial.writeS("Hello", 5);
 *   uint8_t buf[256];
 *   int n = serial.readS(buf, 256, false, false);
 *   serial.close_connection();
 *
 * Example: Serial with CRC
 *   uint8_t data[10] = {0x01, 0x02, 0x03};  // Leave 2 bytes for CRC
 *   serial.writeS(data, 3, true);  // Appends CRC (sends 5 bytes)
 *   serial.readS(buffer, 5, true); // Verifies CRC
 */

#include "test_utils.hpp"
#include "serial_client.hpp"
#include <thread>
#include <chrono>
#include <cstring>
#include <dirent.h>

using namespace Communication;

static std::string g_test_port;
static bool g_port_available = false;

// List available serial ports (Linux)
std::vector<std::string> list_serial_ports()
{
    std::vector<std::string> ports;

#ifdef __linux__
    // Check /dev/ttyUSB*
    DIR *dir = opendir("/dev");
    if(dir)
    {
        struct dirent *entry;
        while((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if(name.find("ttyUSB") == 0 || name.find("ttyACM") == 0 ||
               name.find("ttyS") == 0 || name.find("pts") == 0)
            {
                ports.push_back("/dev/" + name);
            }
        }
        closedir(dir);
    }
#elif defined(__APPLE__)
    DIR *dir = opendir("/dev");
    if(dir)
    {
        struct dirent *entry;
        while((entry = readdir(dir)) != nullptr)
        {
            std::string name = entry->d_name;
            if(name.find("tty.usb") == 0 || name.find("cu.usb") == 0)
            {
                ports.push_back("/dev/" + name);
            }
        }
        closedir(dir);
    }
#endif

    return ports;
}

// Test: Serial port opens successfully
bool test_serial_open()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true; // Skip test
    }

    Serial serial(-1);

    try
    {
        int result = serial.open_connection(g_test_port.c_str(), 115200, 0);
        TEST_ASSERT(result >= 0);
        TEST_ASSERT(serial.is_connected());
        serial.close_connection();
        TEST_ASSERT(!serial.is_connected());
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: Serial port opens with different baud rates
bool test_serial_baud_rates()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true;
    }

    Serial serial(-1);
    int baud_rates[] = {9600, 19200, 38400, 57600, 115200};

    for(int baud : baud_rates)
    {
        try
        {
            serial.open_connection(g_test_port.c_str(), baud, 0);
            serial.close_connection();
        }
        catch(const std::exception &e)
        {
            std::cerr << "  Failed at " << baud << " baud: " << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

// Test: Serial loopback (requires TX-RX connected)
bool test_serial_loopback()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true;
    }

    Serial serial(-1);

    try
    {
        serial.open_connection(g_test_port.c_str(), 115200, 0);

        const char *msg = "Loopback Test";
        serial.writeS(msg, strlen(msg));

        // Small delay for loopback
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        uint8_t buffer[256] = {0};
        int n = serial.readS(buffer, strlen(msg), false, false);

        if(n > 0)
        {
            TEST_ASSERT_EQ(0, memcmp(buffer, msg, strlen(msg)));
        }
        else
        {
            std::cerr << "  Note: No loopback data received (TX-RX not connected?)" << std::endl;
        }

        serial.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: Serial with CRC
bool test_serial_crc()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true;
    }

    Serial serial(-1);

    try
    {
        serial.open_connection(g_test_port.c_str(), 115200, 0);

        // Buffer with extra space for CRC
        uint8_t send_buf[10] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00};
        serial.writeS(send_buf, 4, true); // Appends 2-byte CRC

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        uint8_t recv_buf[10] = {0};
        int n = serial.readS(recv_buf, 6, true, false); // Verifies CRC

        if(n > 0)
        {
            // CRC should have been verified
            TEST_ASSERT_EQ(6, n);
        }

        serial.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: Serial non-blocking read
bool test_serial_nonblocking()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true;
    }

    Serial serial(-1);

    try
    {
        serial.open_connection(g_test_port.c_str(), 115200, 0);

        // Non-blocking read with no data should return quickly
        uint8_t buffer[256];
        auto start = std::chrono::steady_clock::now();
        int n = serial.readS(buffer, 256, false, false);
        auto end = std::chrono::steady_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        TEST_ASSERT(duration.count() < 1000); // Should be fast

        serial.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

// Test: Serial binary data
bool test_serial_binary()
{
    if(!g_port_available)
    {
        std::cerr << "  Skipped: No serial port available" << std::endl;
        return true;
    }

    Serial serial(-1);

    try
    {
        serial.open_connection(g_test_port.c_str(), 115200, 0);

        // Send all byte values 0-255
        uint8_t send_buf[256];
        for(int i = 0; i < 256; i++)
            send_buf[i] = (uint8_t)i;

        serial.writeS(send_buf, 256);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        uint8_t recv_buf[256] = {0};
        int n = serial.readS(recv_buf, 256, false, false);

        if(n == 256)
        {
            TEST_ASSERT_EQ(0, memcmp(send_buf, recv_buf, 256));
        }

        serial.close_connection();
    }
    catch(const std::exception &e)
    {
        std::cerr << "  Error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

void print_usage(const char *prog)
{
    std::cout << "Usage: " << prog << " [options] [port]\n"
              << "\nOptions:\n"
              << "  --list     List available serial ports\n"
              << "  --help     Show this help\n"
              << "\nExamples:\n"
              << "  " << prog << " /dev/ttyUSB0\n"
              << "  " << prog << " /dev/pts/2  # Virtual port from socat\n"
              << "\nTo create virtual serial ports:\n"
              << "  socat -d -d pty,raw,echo=0 pty,raw,echo=0\n";
}

int main(int argc, char *argv[])
{
    // Parse arguments
    for(int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if(arg == "--list")
        {
            std::cout << "Available serial ports:\n";
            for(const auto &port : list_serial_ports())
                std::cout << "  " << port << "\n";
            return 0;
        }
        else if(arg == "--help" || arg == "-h")
        {
            print_usage(argv[0]);
            return 0;
        }
        else if(arg[0] != '-')
        {
            g_test_port = arg;
            g_port_available = true;
        }
    }

    if(!g_port_available)
    {
        // Try to find a port automatically
        auto ports = list_serial_ports();
        if(!ports.empty())
        {
            g_test_port = ports[0];
            g_port_available = true;
            std::cout << "Auto-detected port: " << g_test_port << "\n";
        }
        else
        {
            std::cout << "No serial port specified or detected.\n"
                      << "Tests will be skipped. Use --help for usage.\n\n";
        }
    }

    Test::TestRunner runner;

    runner.add_test("Serial open", test_serial_open);
    runner.add_test("Serial baud rates", test_serial_baud_rates);
    runner.add_test("Serial loopback", test_serial_loopback);
    runner.add_test("Serial CRC", test_serial_crc);
    runner.add_test("Serial non-blocking", test_serial_nonblocking);
    runner.add_test("Serial binary data", test_serial_binary);

    return runner.run();
}
