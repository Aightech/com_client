#ifndef __HTTP_CLIENT_HPP__
#define __HTTP_CLIENT_HPP__

#include "com_client.hpp"

#include <cstring>

namespace Communication
{

class HTTP_client : virtual public ESC::CLI
{

    public:
    HTTP_client(std::string ip, int port, int verbose = -1)
        : ESC::CLI(verbose, "HTTP_client"), m_client(verbose -1)
    {
        m_ip = ip;
        strcpy(m_header_post, "POST %s HTTP/1.1\n"
                              "Host: %s\n"
                              "User-Agent: aightech\n"
                              "Accept: */*\n\n");
        strcpy(m_header_post_with_data,
               "POST %s HTTP/1.1\n"
               "Host: %s\n"
               "User-Agent: aightech\n"
               "Accept: */*\n"
               "Content-Length: %d\n"
               "Content-Type: application/x-www-form-urlencoded\n\n");
        m_client.open_connection(Communication::Client::Mode::TCP,
                                 "192.168.0.100", port);
    };
    ~HTTP_client() { m_client.close_connection(); };

    std::string
    post(const char *page, const char *content = NULL)
    {
        m_content_length = content ? strlen(content) : 0;
        if(m_content_length > 0)
            sprintf(m_header, m_header_post_with_data, page, m_ip.c_str(),
                    m_content_length);
        else
            sprintf(m_header, m_header_post, page, m_ip.c_str());
        //std::cout << "new message: " << std::endl;
        //std::cout << m_header << (m_content_length ? content : "") << std::endl;
        m_client.writeS(m_header, strlen(m_header));
        if(m_content_length > 0)
            m_client.writeS(content, m_content_length);

	// if(m_content_length > 0)
	//   logln("POST " + m_content);
	// logln("@" + std::string(page));
        m_client.readS((uint8_t *)m_reply_buff, 2048);
        //printf("Received %d bytes: %s\n", n, m_reply_buff);

        char *d = strstr(m_reply_buff, "\r\n\r\n");
        int size = strtol(d, NULL, 16);
        d = strstr(d, "{");
        d[size] = '\0';
        std::string s(d);
        //logln("Received [" + std::to_string(size) + " bytes] : " + s, true);
        return d;
    };

    protected:
    Communication::Client m_client;
    std::string m_ip;
    char m_header_post[1024];
    char m_header_post_with_data[1024];
    char m_header[2024];
    char m_reply_buff[2048];
    std::string m_content;
    int m_content_length;
};

} // namespace Communication

#endif // __HTTP_CLIENT_HPP__
