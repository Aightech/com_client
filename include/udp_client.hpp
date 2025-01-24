#ifndef __UDP_CLIENT_HPP__
#define __UDP_CLIENT_HPP__

#include "com_client.hpp"
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace Communication
{
class UDP : public Client
{
    public:
    UDP(int verbose = -1);
    ~UDP() {};

    /**
     * @brief open_connection Open the connection the serial or network or interface
     * @param address Path or IP address
     * @param port -1 to open a serial com, else the value of the port to listen/write
     * @param flags Some flags
     * @return
     */
    int
    open_connection(const char *path, int port, int timeout);

    /**
     * @brief readS read the com inmterface.
     * @param buffer Data store in buffer.
     * @param size Nb of bytes to read.
     * @param has_crc If true the two last bytes are checked as a CRC16.
     * @param read until loop until "size" bytes have been read.
     * @return number of bytes read.
     */
    int
    readS(uint8_t *buffer,
          size_t size,
          bool has_crc = false,
          bool read_until = true);

    /**
     * @brief writeS write the com interface.
     * @param buffer Data to write.
     * @param size Nb of bytes to write.
     * @param add_crc If true two more bytes are added to the buffer to store a CRC16. Be sure to have enough space in the buffer.
     * @return number of bytes written.
     */
    int
    writeS(const void *buffer, size_t size, bool add_crc = false);

    private:
    /* data */
    uint32_t m_size_addr;
};

/**
 * @brief UDP Server class
 *
 * This class implements a UDP server derived from the Server base class.
 * It manages receiving and sending datagrams and handles basic error logging.
 */
class UDPServer : public Server
{
    public:
    UDPServer(int port, int max_connections = 10, int verbose = -1)
        : Server(port, max_connections, verbose),
          ESC::CLI(verbose, "UDP-Server")
    {
        logln("UDP Server created on port " + std::to_string(port), true);
    }

    ~UDPServer()
    {
        logln("UDP Server destroyed", true);
        stop();
    }

    int
    send_data(const void *buffer, size_t size, void *addr) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return sendto(m_fd, (const char *)buffer, size, 0, (SOCKADDR *)addr,
                      sizeof(SOCKADDR));
    }

    void broadcast(const void *buffer, size_t size) override
    {
        //create a temporary socket to broadcast the data
        SOCKET broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if(broadcast_socket == INVALID_SOCKET)
        {
            throw log_error("Failed to create broadcast socket");
        }

        // Enable broadcast
        int broadcast_enable = 1;
        if(setsockopt(broadcast_socket, SOL_SOCKET, SO_BROADCAST,
                      (const char *)&broadcast_enable,
                      sizeof(broadcast_enable)) == SOCKET_ERROR)
        {
            closesocket(broadcast_socket);
            throw log_error("Failed to enable broadcast on socket");
        }

        // Set up the broadcast address
        SOCKADDR_IN broadcast_addr;
        memset(&broadcast_addr, 0, sizeof(broadcast_addr));
        broadcast_addr.sin_family = AF_INET;
        broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
        broadcast_addr.sin_port = htons(m_port);

        // Broadcast the data to all clients
        if(sendto(broadcast_socket, (const char *)buffer, size, 0,
                  (SOCKADDR *)&broadcast_addr, sizeof(broadcast_addr)) ==
           SOCKET_ERROR)
        {
            closesocket(broadcast_socket);
            throw log_error("Failed to broadcast data");
        }

        closesocket(broadcast_socket);
    };

    protected:
    void
    listen_for_connections() override
    {
        m_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(m_fd == INVALID_SOCKET)
        {
            throw log_error("Failed to create UDP socket");
        }

        // Set up the server address
        SOCKADDR_IN server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(m_port);

        // Bind the socket to the specified port
        if(bind(m_fd, (SOCKADDR *)&server_addr, sizeof(server_addr)) ==
           SOCKET_ERROR)
        {
            closesocket(m_fd);
            throw log_error("Failed to bind UDP socket to port " +
                            std::to_string(m_port));
        }

        // std::cout << "UDP Server is listening on port " << m_port << std::endl;
        logln("UDP Server is listening on port " + std::to_string(m_port),
              true);

        // Start receiving data in a separate thread
        std::thread(&UDPServer::receive_data, this).detach();
    }

    void
    handle_client(SOCKET client_socket) override
    {
        // No persistent connection in UDP, this is intentionally left blank.
    }

    private:
    void
    receive_data()
    {
        char buffer[1024];
        SOCKADDR_IN client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        while(m_is_running)
        {
            // Receive a datagram from a client
            int bytes_received =
                recvfrom(m_fd, buffer, sizeof(buffer) - 1, 0,
                         (SOCKADDR *)&client_addr, &client_addr_len);

            if(bytes_received > 0)
            {
                buffer[bytes_received] =
                    '\0'; // Null-terminate the received data
                logln("Received from client: " + std::string(buffer), true);
                logln("Client address: " +
                          std::string(inet_ntoa(client_addr.sin_addr)),
                      true);
                if(m_callback != nullptr)
                    m_callback(this, (uint8_t *)buffer, bytes_received,
                               (void *)&client_addr, m_callback_data);
                else
                {
                    // Echo the data back to the client
                    sendto(m_fd, buffer, bytes_received, 0,
                           (SOCKADDR *)&client_addr, client_addr_len);
                }
            }
            else if(bytes_received == SOCKET_ERROR)
            {
                std::cerr << "Error receiving data: " << strerror(errno)
                          << std::endl;
            }
        }
    }
};

} // namespace Communication

#endif // __UDP_CLIENT_HPP__