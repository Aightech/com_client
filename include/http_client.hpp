#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include "com_client.hpp"

#include <cstring>

namespace Communication
{

/**
 * @brief HTTP client class
 * @details This class is used to send HTTP requests to a server
 */
class HTTP_client : virtual public ESC::CLI
{
    public:
    HTTP_client(int verbose = -1);
    ~HTTP_client();

    void
    open_connection(std::string ip, int port);

    /**
     * @brief send a get request to the server
        * @param page the page to request
        * @param n the number of bytes to read. If n is -1, read 2048 bytes max and don't read until 2048 bytes are read
     */
    std::string
    get(const char *page, int n = -1);

    /**
     * @brief send a post request to the server
        * @param page the page to request
        * @param content the content to send
        * @param n the number of bytes to read. If n is -1, read 2048 bytes max and don't read until 2048 bytes are read
     */
    std::string
    post(const char *page, const char *content = NULL, int n = -1);

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
