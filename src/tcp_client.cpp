#include "tcp_client.hpp"

namespace Communication
{

using namespace ESC;

TCP::TCP(int verbose) : Client(verbose), ESC::CLI(verbose, "TCP-Client") {}

int
TCP::open_connection(const char *address, int port, int timeout)
{
    m_ip = address;
    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo;
    TIMEVAL tv = {.tv_sec = timeout, .tv_usec = 0};
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
TCP::readS(uint8_t *buffer, size_t size, bool has_crc, bool read_until)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    int n = recv(m_fd, buffer, size, 0);
    if(n != size && read_until)
        while(n != size) n += recv(m_fd, buffer + n, size - n, 0);

    if(has_crc)
        return check_CRC(buffer, size) ? n : -1;
    return n;
}

int
TCP::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    return send(m_fd, buffer, size + 2 * add_crc, 0);
}

} // namespace Communication
