#include "tcp_client.hpp"

namespace Communication
{

using namespace ESC;

TCP::TCP(int verbose) : ESC::CLI(verbose, "TCP-Client"), Client(verbose) {}

int
TCP::open_connection(const char *address, int port, int timeout)
{
    m_ip = address;
    SOCKADDR_IN sin = {0,0,0,0};
#ifdef __linux__
    struct hostent *hostinfo;
    TIMEVAL tv = {timeout,0};
    int res;
    set_cli_id(cli_id() + ((cli_id() == "") ? "" : " - ") +
               fstr_link(std::string(address) + ":" + std::to_string(port)));

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
              " (timeout=" + std::to_string(timeout) + "s)",
          true);

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
        logln(fstr("connected", {BOLD, FG_GREEN}),true);
        m_is_connected = true;
    }
    else
    {
        throw log_error("Connection error");
    }
    m_is_connected = true;
#elif _WIN32
    // WSADATA WSAData;
    // int err = WSAStartup(MAKEWORD(2, 2), &WSAData);
    // if(err < 0)
    //     throw log_error("WSAStartup failed !");
    // set_cli_id(cli_id() + ((cli_id() == "") ? "" : " - ") +
    //            fstr_link(std::string(address) + ":" + std::to_string(port)));

    // m_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if(m_fd == INVALID_SOCKET)
    //     throw log_error("socket() invalid");

    // sin.sin_addr.s_addr = inet_addr(address);
    // sin.sin_port = htons(port);
    // sin.sin_family = AF_INET;

    // logln("Connection in progress" + fstr("...", {BLINK_SLOW}) +
    //           " (timeout=" + std::to_string(timeout) + "s)",
    //       true);

    // if(timeout != -1)
    //     this->SetSocketBlockingEnabled(false); //set socket non-blocking
    // int res = connect(m_fd, (SOCKADDR *)&sin, sizeof(SOCKADDR)); //try to connect

    // if(timeout != -1)
    //     this->SetSocketBlockingEnabled(true); //set socket blocking
#endif

    return 1;
}

int
TCP::readS(uint8_t *buffer, size_t size, bool has_crc, bool read_until)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    if(m_is_connected)
    {
#if defined(__linux__) || defined(__APPLE__)
        ssize_t n = recv(m_fd, buffer, size, 0);
        if((size_t)n != size && read_until)
            while((size_t)n != size) n += recv(m_fd, buffer + n, size - n, 0);
#elif _WIN32
        DWORD n = 0;
        if(!ReadFile((HANDLE)m_fd, buffer, size, &n, NULL))
            throw log_error("Could not read the serial port.");
        if(n != size && read_until)
            while(n != size)
            {
                DWORD n2 = 0;
                if(!ReadFile((HANDLE)m_fd, buffer + n, size - n, &n2, NULL))
                    throw log_error("Could not read the serial port.");
                n += n2;
            }
#endif
        if(has_crc)
            return check_CRC(buffer, size) ? n : -1;
        return n;
    }
    return -1;
}

int
TCP::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    if(!m_is_connected)
        return -1;

    if(add_crc)
        *(uint16_t *)((uint8_t *)buffer + size) = CRC((uint8_t *)buffer, size);

#if defined(__linux__) || defined(__APPLE__)
    return send(m_fd, buffer, size + 2 * add_crc, 0);
#elif _WIN32
    DWORD n = 0;
    if(WriteFile((HANDLE)m_fd, buffer, size + 2 * add_crc, &n, NULL))
        return n;
    return -1;
#endif
}

} // namespace Communication
