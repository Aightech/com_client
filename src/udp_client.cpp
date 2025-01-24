#include "udp_client.hpp"

namespace Communication
{

using namespace ESC;

UDP::UDP(int verbose) : Client(verbose), ESC::CLI(verbose, "UDP-Client") {}

int
UDP::open_connection(const char *address, int port, int timeout)
{
#ifdef __linux__
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
#endif
    return 1;
}

int
UDP::readS(uint8_t *buffer, size_t size, bool has_crc, bool read_until)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
#ifdef __linux__
    int n = recvfrom(m_fd, buffer, size, MSG_WAITALL, (SOCKADDR *)&m_addr_to,
                     &m_size_addr);
    if(n != size && read_until)
        while(n != size)
            n += recvfrom(m_fd, buffer + n, size - n, MSG_WAITALL,
                          (SOCKADDR *)&m_addr_to, &m_size_addr);

    if(has_crc)
        return check_CRC(buffer, size) ? n : -1;
    return n;
#endif
    return 1;
}

int
UDP::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
#ifdef __linux__
    return sendto(m_fd, buffer, size + 2 * add_crc, 0, (SOCKADDR *)&m_addr_to,
                  m_size_addr);
#endif
    return -1;
}


} // namespace Communication
