#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com_client.hpp"

namespace Communication
{

Client::Client(Mode mode) : m_comm_mode(mode)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if(err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
}

Client::~Client(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

int
Client::open_connection(const char *address, int port, int flags)
{
    if(m_comm_mode == FILE_MODE)
      {

	this->setup_serial(address, flags);
      }
    else if(m_comm_mode == SOCKET_MODE)
    {
      setup_socket(address, port);
    }
    usleep(100000);

    return m_fd;
}

int
Client::close_connection()
{
    return closesocket(m_fd);
}

int
Client::setup_socket(const char *address, int port)
{
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo;

    if(m_fd == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    hostinfo = gethostbyname(address);
    if(hostinfo == NULL)
    {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    if(connect(m_fd, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()");
        exit(errno);
    }
}

int
Client::setup_serial(const char *path, int flags)
{
    m_fd = open(path, flags);
    std::cout << "> Check connection: " << std::flush;
    if(m_fd < 0)
    {
        std::cerr << "[ERROR] Could not open the serial port." << std::endl;
        exit(-1);
    }
    std::cout << "OK\n" << std::flush;

    struct termios tty;
    if(tcgetattr(m_fd, &tty) != 0)
    {
        std::cerr << "[ERROR] Could not get the serial port settings."
                  << std::endl;
        exit(-1);
    }

    tty.c_cflag &= ~PARENB;  // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB;  // Clear stop field, only 1 stop bit (most common)
    tty.c_cflag &= ~CSIZE;   // Clear all bits that set the data size
    tty.c_cflag |= CS8;      // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow ctrl (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrlline(CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                     ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent spe. interp. of out bytes (newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conv of newline to car. ret/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty.c_cc[VTIME] = 0; // Wait for up to 1s, ret when any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if(tcsetattr(m_fd, TCSANOW, &tty) != 0)
    {
        std::cerr << "[ERROR] Could not set the serial port settings."
                  << std::endl;
        exit(-1);
    }
}

int
Client::readS(uint8_t *buffer, size_t n)
{
    int nn = 0;

    if((nn = recv(m_fd, buffer, n, 0)) < 0)
    {
        perror("recv()");
        exit(errno);
    }
    return nn;
}

int
Client::writeS(const void *buffer, size_t size)
{
    int n = 0;
    if((n = send(m_fd, buffer, size, 0)) < 0)
    {
        perror("send()");
        exit(errno);
    }
    return n;
}

} // namespace Communication
