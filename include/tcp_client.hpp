#ifndef __TCP_CLIENT_HPP__
#define __TCP_CLIENT_HPP__

#include "com_client.hpp"
#include <cstring>
#include <deque>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <algorithm> // for std::copy

namespace Communication
{
class TCP : public Client
{
    public:
    TCP(int verbose = -1);
    ~TCP() {};

    /**
     * @brief open_connection Open the connection the serial or network or interface
     * @param address Path or IP address
     * @param port -1 to open a serial com, else the value of the port to listen/write
     * @param flags Some flags
     * @return
     */
    int
    open_connection(const char *address, int port, int timeout);

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

    protected:
    std::string m_ip;
    /* data */
};

class TCPServer : public Server
{
    public:
    TCPServer(int port, int max_connections = 10, int verbose = -1)
        : Server(port, max_connections, verbose),
          ESC::CLI(verbose, "TCP-Server")
    {
        logln("TCP Server created on port " + std::to_string(port), true);
    }

    ~TCPServer() { stop(); }

    int
    send_data(const void *buffer, size_t size, void *addr) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        SOCKADDR_IN *client_addr = (SOCKADDR_IN *)addr;
        for(auto &client : m_clients)
        {
            if(client_addr == nullptr || client == client_addr->sin_addr.s_addr)
            {
                send(client, buffer, size, 0);
            }
        }
        return 0;
    }
    int8_t
    read_byte(SOCKET i, uint8_t *buffer, size_t size)
    {
        if(m_clients.find(i) == m_clients.end())
            return -1;
        size = std::min(size, m_fifos[i].size());
        std::copy(m_fifos[i].begin(), m_fifos[i].begin() + size, buffer);
        m_fifos[i].erase(m_fifos[i].begin(), m_fifos[i].begin() + size);
        return size;
    }

    int
    is_available(SOCKET i)
    {
        if(m_clients.find(i) == m_clients.end())
            return -1;
        return m_fifos[i].size();
    }

    protected:
    void
    listen_for_connections() override
    {
        m_fd = socket(AF_INET, SOCK_STREAM, 0);
        if(m_fd == INVALID_SOCKET)
        {
            throw log_error("socket() invalid");
        }

        // Set up the server address
        SOCKADDR_IN sin = {0};
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(m_port);

        if(bind(m_fd, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
        {
            closesocket(m_fd);
            throw log_error("Failed to bind to port " + std::to_string(m_port));
        }

        if(listen(m_fd, m_max_connections) == SOCKET_ERROR)
        {
            closesocket(m_fd);
            throw log_error("Failed to listen on port " +
                            std::to_string(m_port));
        }

        // std::cout << "TCP Server listening on port " << m_port << std::endl;
        logln("TCP Server is listening on port " + std::to_string(m_port),
              true);

        // Accept incoming connections
        std::thread(&TCPServer::accept_connections, this).detach();
    }

    void
    handle_client(SOCKET client_socket) override
    {
        char buffer[1024];
        while(m_is_running)
        {
            int bytes_received =
                recv(client_socket, buffer, sizeof(buffer), 0);
            if(bytes_received > 0)
            {
                m_fifos[client_socket].insert(m_fifos[client_socket].end(),
                                              buffer, buffer + bytes_received);
                if(m_callback)
                {
                    m_callback(this, (uint8_t *)buffer, bytes_received,
                               (void *)&client_socket, m_callback_data);
                }
            }
            else if(bytes_received == 0)
            {
                std::cout << "Client disconnected." << std::endl;
                break;
            }
            else
            {
                std::cerr << "Error receiving data." << std::endl;
                break;
            }
        }

        closesocket(client_socket);
        //remove the SOCKET from the vector
        
    }

    
    

    private:
    std::unordered_map<SOCKET, std::deque<uint8_t>> m_fifos;
    void
    accept_connections()
    {
        while(m_is_running)
        {
            SOCKADDR_IN client_addr;
            socklen_t client_addr_len = sizeof(client_addr);

            SOCKET client_socket =
                accept(m_fd, (SOCKADDR *)&client_addr, &client_addr_len);
            if(client_socket == INVALID_SOCKET)
            {
                if(m_is_running)
                {
                    std::cerr << "Error accepting client connection."
                              << std::endl;
                }
                continue;
            }

            std::cout << "Client connected from "
                      << inet_ntoa(client_addr.sin_addr) << ":"
                      << ntohs(client_addr.sin_port) << std::endl;

            m_clients.insert(client_socket);
            m_fifos[client_socket] = std::deque<uint8_t>();

            // Handle the client in a separate thread
            std::thread(&TCPServer::handle_client, this, client_socket)
                .detach();
        }
    }
};

} // namespace Communication

#endif // __TCP_CLIENT_HPP__