#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include "com_client.hpp"
#include "udp_client.hpp"

void cb(Communication::Server* srv, uint8_t* data, size_t len, void* addr, void* user_data)
{
    std::ofstream* filePtr = static_cast<std::ofstream*>(user_data);
    std::string mac_address(reinterpret_cast<const char*>(data), len);
    std::cout << "Received MAC: " << mac_address << std::endl;
    (*filePtr) << mac_address << "\n";
    filePtr->flush();
}

int main()
{
    using namespace Communication;

    // Create UDP server on port 4210
    UDPServer server(4210);
    
    // Open file for storing device MAC addresses
    std::ofstream file("esp32_devices.txt", std::ios::app);
    if(!file.is_open())
    {
        std::cerr << "Error opening file for writing." << std::endl;
        return -1;
    }

    // Register a callback to handle incoming data (MAC addresses)
    server.set_callback(cb, &file);

    // Start the UDP server (binds and listens in a separate thread)
    try
    {
        server.start();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Server start error: " << e.what() << std::endl;
        return -1;
    }

    // Prepare Wi-Fi credentials
    std::string ssid = "MyWiFiNetwork";
    std::string pass = "MySecretPass";
    std::string message = ssid + "," + pass;

    // Broadcast credentials
    try
    {
        server.broadcast(message.c_str(), message.size());
        std::cout << "Wi-Fi credentials sent to all devices." << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Broadcast failed: " << e.what() << std::endl;
        return -1;
    }

    // Keep the server running to receive MAC addresses indefinitely
    while(true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
