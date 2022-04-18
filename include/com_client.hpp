#ifndef COM_CLIENT_HPP
#define COM_CLIENT_HPP
#include <iostream>

#ifdef WIN32 //////////// IF WINDOWS OS //////////
#include <winsock2.h>
#elif defined(linux) ///// IF LINUX OS //////////
#include <arpa/inet.h>
#include <netdb.h> // gethostbyname
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
// Linux headers
#include <errno.h>   // Error integer and strerror() function
#include <fcntl.h>   // Contains file controls like O_RDWR
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
#endif

    public:
    enum Mode
    {
        SERIAL_MODE,
        SOCKET_MODE
    };

    Client(Mode mode);
    ~Client();

    void
    setup()
    {
        // ARDUINO SIDE
        // Serial.write(0x40);
        // char c;
        // while (c != 0xbb)
        // {
        //   c = Serial.read();
        // }

        uint8_t buff[1] = {'.'};
        std::cout << "Connecting" << std::flush;
        while(buff[0] != 0x40) //empty the random char
        {
            if(this->readS(buff, 1) > 0)
                std::cout << buff[0] << std::flush;
            usleep(1000);
        }
        buff[0] = 0xbb;
        this->writeS(buff, 1);
        usleep(10000);
        std::cout << std::endl;
    };

    int
    open_connection(const char *address,
                    int port=5000,
                    int flags = O_RDWR | O_NOCTTY);
    int
    close_connection();

    int
    readS(uint8_t *buffer, size_t size);
    int
    writeS(const void *buffer, size_t size);

    private:
    int
    setup_serial(const char *address, int flags);
    int
    setup_socket(const char *address, int port);
    SOCKET m_fd;
    Mode m_comm_mode;
};

} // namespace Communication

#endif //COM_CLIENT_HPP
