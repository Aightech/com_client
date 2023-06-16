#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com_client.hpp"
#include <cerrno>
#include <clocale>
#include <cstring>

#include <sys/ioctl.h>

namespace Communication
{

using namespace ESC;



Client::Client(int verbose) : ESC::CLI(verbose, "Client")
{
    logln("Init communication client.", true);
    this->mk_crctable(); //init crc table
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

void
Client::from_socket(SOCKET s)
{
    m_fd = s;
}

int
Client::close_connection()
{
    logln("Closing connection ", true);
    int n = closesocket(m_fd);
    logln(fstr("OK", {BOLD, FG_GREEN}));
    return n;
}

bool
Client::check_CRC(uint8_t *buffer, int size)
{
    if(this->CRC(buffer, size - 2) != *(uint16_t *)(buffer + size - 2))
    {
        logln("CRC error", true);
        return false; //crc error
    }
    return true;
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

uint16_t
Client::CRC(uint8_t *buf, int n)
{
    uint16_t m_crc_accumulator = 0;
    for(int i = 0; i < n; i++)
        m_crc_accumulator = (m_crc_accumulator << 8) ^
                            s_crctable[(m_crc_accumulator >> 8) ^ buf[i]];
    return (m_crc_accumulator >> 8) | (m_crc_accumulator << 8);
}

uint16_t Client::s_crctable[256];
void
Client::mk_crctable(uint16_t genpoly)
{
    static bool once = [this, genpoly]()//init crc table only once
    {
        for(uint16_t data = 0; data < 256; data++)
        {
            uint16_t d = data;
            uint16_t accum = 0;
            d <<= 8;
            for(int i = 8; i > 0; i--)
            {
                if((d ^ accum) & 0x8000)
                    accum = (accum << 1) ^ genpoly;
                else
                    accum <<= 1;
                d <<= 1;
            }
            s_crctable[d] = accum;
        }
        return true;
    }();
}

} // namespace Communication
