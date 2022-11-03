#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com_client.hpp"
#include <cerrno>
#include <clocale>
#include <cstring>

namespace Communication
{

using namespace ESC;

Client::Client(int verbose) : ESC::CLI(verbose, "Client")
{
    logln("Init communication client.", true);
    mk_crctable();
    m_mutex = new std::mutex();
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
    delete m_mutex;
}

int
Client::open_connection(Mode mode, const char *address, int port, int flags)
{
    m_comm_mode = mode;

    if(m_comm_mode == SERIAL)
        this->setup_serial(address, flags);
    else if(m_comm_mode == TCP)
        this->setup_TCP_socket(address, port,
                               ((O_RDWR | O_NOCTTY) == flags) ? -1 : flags);
    else if(m_comm_mode == UDP)
        this->setup_UDP_socket(address, port,
                               ((O_RDWR | O_NOCTTY) == flags) ? -1 : flags);

    usleep(100000);

    return m_fd;
}

  void
Client::from_socket(SOCKET s)
{
    m_comm_mode = TCP;
    m_fd = s;
}

int
Client::close_connection()
{
  logln("Closing connection ",true);
    int n = closesocket(m_fd);
    logln(fstr(" OK", {BOLD, FG_GREEN}));
    return n;
}

int
Client::setup_TCP_socket(const char *address, int port, int timeout)
{
    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo;
    TIMEVAL tv = {.tv_sec = timeout, .tv_usec = 0};
    int res;
    cli_id() += ((cli_id() == "") ? "" : " - ") +
                fstr_link(std::string(address) + ":" + std::to_string(port));

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_fd == INVALID_SOCKET)
        throw log_error("socket() invalid");

    hostinfo = gethostbyname(address);
    if(hostinfo == NULL)
        throw log_error(std::string("Unknown host") + address);

    sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    logln("Connection in progress" + fstr("...", {BLINK_SLOW}) +
          " (timeout=" + std::to_string(timeout) + "s)", true);

    if(timeout != -1)
        this->SetSocketBlockingEnabled(false); //set socket non-blocking
    res = connect(m_fd, (SOCKADDR *)&sin, sizeof(SOCKADDR)); //try to connect
    if(timeout != -1)
        this->SetSocketBlockingEnabled(true); //set socket blocking

    if(res < 0 && errno == EINPROGRESS) //if not connected instantaneously
    {
        fd_set wait_set;         //create fd set
        FD_ZERO(&wait_set);      //clear fd set
        FD_SET(m_fd, &wait_set); //add m_fd to the set
        res = select(m_fd + 1,  // return if one of the set could be wrt/rd/expt
                     NULL,      //reading set of fd to watch
                     &wait_set, //writing set of fd to watch
                     NULL,      //exepting set of fd to watch
                     &tv);      //timeout before stop watching
        if(res < 1)
            logln("Could not connect to " + fstr(m_id, {BOLD, FG_RED}));
        if(res == -1)
            throw log_error(" Error with select()");
        else if(res == 0)
            throw log_error(" Connection timed out");
        res = 0;
    }
    if(res == 0)
    {
        int opt; // check for errors in socket layer
        socklen_t len = sizeof(opt);
        if(getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &opt, &len) < 0)
            throw log_error(" Error retrieving socket options");
        if(opt) // there was an error
            throw log_error(std::strerror(opt));
        logln(fstr("connected", {BOLD, FG_GREEN}));
        m_is_connected = true;
    }
    else
      {
	throw log_error("Connection error");
      }

    return 1;
}

int
Client::setup_UDP_socket(const char *address, int port, int timeout)
{
    struct hostent *hostinfo;
    cli_id() += ((cli_id() == "") ? "" : " - ") +
                fstr_link(std::string(address) + ":" + std::to_string(port));

    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(m_fd == INVALID_SOCKET)
        throw log_error("socket() invalid");

    hostinfo = gethostbyname(address);
    if(hostinfo == NULL)
        throw log_error(std::string("Unknown host") + address);

    m_addr_to.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    m_addr_to.sin_port = htons(port);
    m_addr_to.sin_family = AF_INET;

    m_size_addr = sizeof(m_addr_to);

    logln("UDP socket is setup. ", true);

    return 1;
}

int
Client::setup_serial(const char *path, int flags)
{
    cli_id() += ((cli_id() == "") ? "" : " - ") +
                fstr_link(std::string(path) + ":" + std::to_string(flags));

    logln("Connection in progress" + fstr("...", {BLINK_SLOW}));

    m_fd = open(path, flags);
    logln(" Check connection: [fd:" + std::to_string(m_fd) + "] ");
    if(m_fd < 0)
        throw log_error("Could not open the serial port.");

    struct termios tty;
    if(tcgetattr(m_fd, &tty) != 0)
        throw log_error("Could not get the serial port settings.");

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

    tty.c_cc[VTIME] = 40; // Wait for up to 4s, ret when any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    cfsetispeed(&tty, B500000);
    cfsetospeed(&tty, B500000);

    // Save tty settings, also checking for error
    if(tcsetattr(m_fd, TCSANOW, &tty) != 0)
        throw m_id + "[ERROR] Could not set the serial port settings.";
    std::cout << "> Serial port settings saved.\n" << std::flush;
    return m_fd;
}

int
Client::readS(uint8_t *buffer, size_t size, bool has_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    int n = 0;
    if(m_comm_mode == TCP)
        n = recv(m_fd, buffer, size, 0);
    else if(m_comm_mode == UDP)
        n = recvfrom(m_fd, buffer, size, MSG_WAITALL, (SOCKADDR *)&m_addr_to,
                     &m_size_addr);
    else if(m_comm_mode == SERIAL)
        n = read(m_fd, buffer, size);
    //std::cout << ">  " << n  << " " << MSG_WAITALL<< std::endl;
    if(has_crc &&
       this->CRC(buffer, size - 2) != *(uint16_t *)(buffer + size - 2))
        return -1; //crc error
    return n;
}

int
Client::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    int n = 0;
    *(uint16_t *)((uint8_t *)buffer + size) =
        CRC((uint8_t *)buffer, size); // add crc
    if(m_comm_mode == TCP)
        n = send(m_fd, buffer, size + 2 * add_crc, 0);
    if(m_comm_mode == UDP)
        n = sendto(m_fd, buffer, size + 2 * add_crc, 0, (SOCKADDR *)&m_addr_to,
                   m_size_addr);
    else if(m_comm_mode == SERIAL)
        n = write(m_fd, buffer, size + 2 * add_crc);
    return n;
}

bool
Client::SetSocketBlockingEnabled(bool blocking)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    if(m_fd < 0)
        return false;

#ifdef _WIN32
    unsigned long mode = blocking ? 0 : 1;
    return (ioctlsocket(m_fd, FIONBIO, &mode) == 0) ? true : false;
#else
    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags == -1)
        return false;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(m_fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

void
Client::mk_crctable(uint16_t poly)
{
    for(int i = 0; i < 256; i++) m_crctable[i] = crchware(i, poly, 0);
}

uint16_t
Client::crchware(uint16_t data, uint16_t genpoly, uint16_t accum)
{
    static int i;
    data <<= 8;
    for(i = 8; i > 0; i--)
    {
        if((data ^ accum) & 0x8000)
            accum = (accum << 1) ^ genpoly;
        else
            accum <<= 1;
        data <<= 1;
    }
    return accum;
}

void
Client::CRC_check(uint8_t data)
{
    m_crc_accumulator =
        (m_crc_accumulator << 8) ^ m_crctable[(m_crc_accumulator >> 8) ^ data];
};

uint16_t
Client::CRC(uint8_t *buf, int n)
{
    m_crc_accumulator = 0;
    for(int i = 0; i < n; i++) CRC_check(buf[i]);
    return (m_crc_accumulator >> 8) | (m_crc_accumulator << 8);
}

} // namespace Communication
