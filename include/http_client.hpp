#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include "com_client.hpp"

#include <cstring>

namespace Communication
{

class HTTP_client : virtual public ESC::CLI
{

    public:
    HTTP_client(int verbose = -1);
    ~HTTP_client();

    void
    open_connection(std::string ip, int port);

    std::string
    get(const char *page, int n=2048);

    std::string
    post(const char *page, const char *content = NULL, int n=2048);

    private:
    Communication::Client m_client;
    std::string m_ip;
    char m_header_post[1024];
    char m_header_post_with_data[1024];
    char m_header_get[1024];
    char m_header[2024];
    int m_content_length;
};

} // namespace Communication

#endif // __HTTP_CLIENT_HPP__
