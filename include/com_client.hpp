#ifndef COM_CLIENT_HPP
#define COM_CLIENT_HPP
#include <iostream>
#include <mutex>
#include <stdexcept>

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

/**
 * @brief The Client class
 *
 * This class is used to unify the different type of communication interface (serial, IP socket, ...) and to provide a common interface to the user.
 * It is also aming to be cross-platform.
 *
 * @author Alexis Devillard
 * @date 2022
 */
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
        std::cout << std::endl;
        usleep(1000000);
    };

    /**
     * @brief open_connection Open the connection the serial or network or interface
     * @param address Path or IP address
     * @param port -1 to open a serial com, else the value of the port to listen/write
     * @param flags Some flags
     * @return
     */
    int
    open_connection(const char *address,
                    int port = -1,
                    int flags = O_RDWR | O_NOCTTY);
    int
    close_connection();

    /**
     * @brief readS read the com inmterface.
     * @param buffer Data store in buffer.
     * @param size Nb of bytes to read.
     * @return number of bytes read.
     */
    int
    readS(uint8_t *buffer, size_t size);

    /**
     * @brief writeS write the com interface.
     * @param buffer Data to write.
     * @param size Nb of bytes to write.
     * @return number of bytes written.
     */
    int
    writeS(const void *buffer, size_t size);

    /**
     * @brief Check if the connection is open.
     * @return True if success, false otherwise.
     */
    bool
    is_connected()
    {
        return m_is_connected;
    };

    /**
     * @brief CRC Compute and return the CRC over the n first bytes of buf
     * @param buf path of the dir to look inside
     * @param n reference to the vector of string that will be filled with the directories that are in the path.
     */
    uint16_t
    CRC(uint8_t *buf, int n);

    private:
    /**
     * @brief crchware Generate the values for the CRC lookup table.
     * @param data The short data to generate the CRC.
     * @param genpoly The generator polynomial.
     * @param accum The accumulator value.
     * @return
     */
    uint16_t
    crchware(uint16_t data, uint16_t genpoly, uint16_t accum);

    /**
     * @brief mk_crctable Create a CRC lookup table to compute CRC16 fatser.
     * @param poly Represent the coeficients use for the polynome of the CRC
     */
    void
    mk_crctable(uint16_t poly = 0x1021);

    /**
     * @brief CRC_check Function use in CRC computation
     * @param data The data to compute.
     */
    void
    CRC_check(uint8_t data);

    /**
     * @brief setup_serial Set up the serial object.
     * @param address path of the serial port.
     * @param flags connection flags.
     * @return int return the file descriptor of the serial port.
     */
    int
    setup_serial(const char *address, int flags);

    /**
     * @brief setup_socket Set up the socket object.
     * @param address IP address of the server.
     * @param port Port of the server.
     * @return int return the file descriptor of the socket.
     */
    int
    setup_socket(const char *address, int port, int timeout = 2);

    /** Returns true on success, or false if there was an error */
    bool
    SetSocketBlockingEnabled(bool blocking);

    SOCKET m_fd;
    Mode m_comm_mode;
    bool m_is_connected = false;
    bool m_verbose;
    std::mutex *m_mutex;
    uint16_t m_crctable[256];
    uint16_t m_crc_accumulator;
};

} // namespace Communication

#endif //COM_CLIENT_HPP
