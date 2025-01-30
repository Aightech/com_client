#ifndef COM_CLIENT_HPP
#define COM_CLIENT_HPP
#include <iostream>
#include <math.h>
#include <mutex>
#include <stdexcept>

#ifdef WIN32 //////////// IF WINDOWS OS //////////
#include <commctrl.h>
#include <windows.h>
#include <winsock2.h>
#elif defined(linux) || defined(__APPLE__) ///// IF LINUX OS //////////
#include <arpa/inet.h>
#include <netdb.h> // gethostbyname
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
// Linux headers
#include <errno.h> // Error integer and strerror() function
#include <fcntl.h> // Contains file controls like O_RDWR
#include <sys/time.h>
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#else
#error not defined for this platform
#endif

#include <strANSIseq.hpp>

//server FIFO var for each client
#include <unordered_set>

#define CRLF "\r\n"

namespace Communication
{

#if defined(linux) || defined(__APPLE__)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
typedef timeval TIMEVAL;
#endif

/**
 * @brief Communication client class
 *
 * This class is used to unify the different type of communication interface (serial, IP socket, ...) and to provide a common interface to the user.
 * It is also aming to be cross-platform.
 *
 * @author Alexis Devillard
 * @date 2022
 */
class Client : virtual public ESC::CLI
{
    public:
    Client(int verbose = -1);
    ~Client();

    /**
     * @brief open_connection Open the connection the serial or network or interface
     * @param address Path or IP address
     * @param opt -1 to open a serial com, else the value of the port to listen/write
     * @param flags Some flags
     * @return
     */
    virtual int
    open_connection(const char *address, int opt, int flags) = 0;

    void
    from_socket(SOCKET s);

    int
    close_connection();

    /**
     * @brief readS read the com inmterface.
     * @param buffer Data store in buffer.
     * @param size Nb of bytes to read.
     * @param has_crc If true the two last bytes are checked as a CRC16.
     * @param read until loop until "size" bytes have been read.
     * @return number of bytes read.
     */
    virtual int
    readS(uint8_t *buffer,
          size_t size,
          bool has_crc = false,
          bool read_until = true) = 0;

    /**
     * @brief writeS write the com interface.
     * @param buffer Data to write.
     * @param size Nb of bytes to write.
     * @param add_crc If true two more bytes are added to the buffer to store a CRC16. Be sure to have enough space in the buffer.
     * @return number of bytes written.
     */
    virtual int
    writeS(const void *buffer, size_t size, bool add_crc = false) = 0;

    /**
     * @brief Check if the connection is open.
     * @return True if success, false otherwise.
     */
    bool
    is_connected()
    {
        return m_is_connected;
    };

    bool
    check_CRC(uint8_t *buffer, int size);

    /**
     * @brief CRC Compute and return the CRC over the n first bytes of buf
     * @param buf path of the dir to look inside
     * @param n reference to the vector of string that will be filled with the directories that are in the path.
     */
    uint16_t
    CRC(uint8_t *buf, int n);

    void
    get_stat(char c = 'd', int pkgSize = 6)
    {
        // uint8_t buf[pkgSize];
        uint8_t* buf = new uint8_t[pkgSize];
        buf[0] = c;
        this->writeS(buf, pkgSize);
        float vals[4];
        this->readS((uint8_t *)vals, 16);

        std::cout << "mean: " << vals[0] << std::endl;
        std::cout << "std: " << sqrt(vals[1] - vals[0] * vals[0]) << std::endl;
        std::cout << "n: " << vals[2] << std::endl;
        std::cout << "max: " << vals[3] << std::endl;
        delete[] buf;
    }

    private:
    /**
     * @brief mk_crctable Create a CRC lookup table to compute CRC16 fatser.
     * @param poly Represent the coeficients use for the polynome of the CRC
     */
    void
    mk_crctable(uint16_t genpoly = 0x1021);
    static uint16_t s_crctable[256];

    protected:
    /** Returns true on success, or false if there was an error */
    bool
    SetSocketBlockingEnabled(bool blocking);

    SOCKET m_fd;
    bool m_is_connected = false;
    std::mutex *m_mutex;
    SOCKADDR_IN m_addr_to;
    std::string m_id;
};

class Server : virtual public ESC::CLI
{
    public:
    Server(int port, int max_connections = 10, int verbose = -1)
        : ESC::CLI(verbose, "Server"), m_port(port), m_max_connections(max_connections), m_is_running(false)
    {
    }

    virtual ~Server() { stop(); }

    /**
     * @brief Start the server and begin listening for connections.
     */
    virtual void
    start()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(m_is_running)
        {
            throw std::runtime_error("Server is already running");
        }
        m_is_running = true;
        listen_for_connections();
    }

    /**
     * @brief Stop the server and close all connections.
     */
    virtual void
    stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if(!m_is_running)
            return;

        m_is_running = false;
        logln("Server stopped", true);
        closesocket(m_fd);
        logln("Server socket closed", true);
        m_fd = INVALID_SOCKET;
    }

    /**
     * @brief Broadcast data to all connected clients.
     * @param buffer Data buffer to send.
     * @param size Size of the data buffer.
     */
    virtual void
    broadcast(const void *buffer, size_t size)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for(auto &client : m_clients) { send(client, buffer, size, 0); }
    }

    // virtual int
    // send_data(const void *buffer, size_t size, void *addr = nullptr){};

    // virtual int
    // send_data(const void *buffer, size_t size, SOCKET s){};

    /**
     * @brief Check if the server is running.
     * @return True if the server is running, false otherwise.
     */
    bool
    is_running() const
    {
        return m_is_running;
    }

    /**
     * @brief Set the callback function to be called when data is received.
     * @param callback Callback function.
     */
    void
    set_callback(void (*callback)(Server *server,
                                  uint8_t *buffer,
                                  size_t size,
                                  void *addr,
                                  void *data),
                 void *data = nullptr)
    {
        m_callback = callback;
        m_callback_data = data;
    }

    void
    set_callback_newClient(void (*callback)(Server *server,
                                            void *addr,
                                            SOCKET client_socket,
                                            void *data),
                           void *data = nullptr)
    {
        m_callback_newClient = callback;
        m_callback_data_newClient = data;
    }

    std::unordered_set<SOCKET> &
    get_clients()
    {
        return m_clients;
    }

    protected:
    /**
     * @brief Listen for incoming connections (to be implemented by derived classes).
     */
    virtual void
    listen_for_connections() = 0;

    /**
     * @brief Handle communication with a connected client (to be implemented by derived classes).
     * @param client_socket Client's socket descriptor.
     */
    virtual void
    handle_client(SOCKET client_socket) = 0;

    SOCKET m_fd = INVALID_SOCKET;
    int m_port;
    int m_max_connections;
    bool m_is_running;
    std::mutex m_mutex;
    std::unordered_set<SOCKET> m_clients;
    //callback(this)
    void (*m_callback)(Server *server,
                       uint8_t *buffer,
                       size_t size,
                       void *addr,
                       void *data) = nullptr;
    void (*m_callback_newClient)(Server *server,
                                 void *addr,
                                 SOCKET client_socket,
                                 void *data) = nullptr;
    void *m_callback_data;
    void *m_callback_data_newClient;
};

} // namespace Communication

#endif //COM_CLIENT_HPP
