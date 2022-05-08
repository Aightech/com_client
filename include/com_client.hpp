#ifndef COM_CLIENT_HPP
#define COM_CLIENT_HPP
#include <iostream>
#include <stdexcept>
#include <mutex>

#ifdef WIN32 //////////// IF WINDOWS OS //////////
#include <winsock2.h>
#elif defined(linux) ///// IF LINUX OS //////////
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

#define CRLF "\r\n"

namespace Communication
{

class Client
{
#if defined(linux)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
    typedef int SOCKET;
    typedef struct sockaddr_in SOCKADDR_IN;
    typedef struct sockaddr SOCKADDR;
    typedef struct in_addr IN_ADDR;
    typedef timeval TIMEVAL;
#endif

    public:
    enum Mode
    {
        SERIAL_MODE,
        SOCKET_MODE
    };

    Client(bool verbose = false);
    ~Client();

    void
    clean_start()
    {
        // ARDUINO SIDE
        // Serial.write(0xaa);
        // char c;
        // while (c != 0xbb)
        //   c = Serial.read();

        uint8_t buff[1] = {'.'};
        std::cout << "Connecting" << std::flush;
        while(buff[0] != 0xaa) //empty the random char
        {
            if(this->readS(buff, 1) > 0)
                std::cout << "." << std::flush;
            usleep(1000);
        }
        buff[0] = 0xbb;
        this->writeS(buff, 1);
	std::cout  << std::endl;
        usleep(1000000);
    };

    int
    open_connection(const char *address,
                    int port = -1,
                    int flags = O_RDWR | O_NOCTTY);
    int
    close_connection();

    int
    readS(uint8_t *buffer, size_t size);
    int
    writeS(const void *buffer, size_t size);

    bool
    is_connected()
    {
        return m_is_connected;
    };

    private:
    int
    setup_serial(const char *address, int flags);
    int
    setup_socket(const char *address, int port, int timeout=2);

    /** Returns true on success, or false if there was an error */
    bool
    SetSocketBlockingEnabled(bool blocking);

    SOCKET m_fd;
    Mode m_comm_mode;
    bool m_is_connected = false;
    bool m_verbose;
  std::mutex* m_mutex;
};

} // namespace Communication

#endif //COM_CLIENT_HPP
