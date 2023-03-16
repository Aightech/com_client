#include "http_client.hpp"

namespace Communication
{

HTTP_client::HTTP_client(int verbose)
    : ESC::CLI(verbose, "HTTP_client"), m_client(verbose - 1)
{
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
    strcpy(m_header_get, "GET %s HTTP/1.1\n"
                         "Host: %s\n"
                         "User-Agent: aightech\n"
                         "Accept: */*\n\n");
};
HTTP_client::~HTTP_client() { m_client.close_connection(); };

void
HTTP_client::open_connection(std::string ip, int port)
{
    m_ip = ip;
    m_client.open_connection(Communication::Client::Mode::TCP, "192.168.0.100",
                             port);
};

std::string
HTTP_client::get(const char *page, int n)
{
    char buffer[n];
    sprintf(m_header, m_header_get, page, m_ip.c_str());
    m_client.writeS(m_header, strlen(m_header));
    bool read_until=true;
    if(n==-1)
      {
	n=2048;
	read_until=false;
      }
    m_client.readS((uint8_t *)buffer, n, false, read_until); //if n <0 then don't "read until"

    char *d = strstr(buffer, "\r\n\r\n");
    int size = strtol(d, NULL, 16);
    d = strstr(d, "{");
    d[size] = '\0';
    std::string s(d);
    //logln("Received [" + std::to_string(size) + " bytes] : " + s, true);
    return d;
};

std::string
HTTP_client::post(const char *page, const char *content, int n)
{
    m_content_length = content ? strlen(content) : 0;
    if(m_content_length > 0)
        sprintf(m_header, m_header_post_with_data, page, m_ip.c_str(),
                m_content_length);
    else
        sprintf(m_header, m_header_post, page, m_ip.c_str());
    m_client.writeS(m_header, strlen(m_header));
    if(m_content_length > 0)
        m_client.writeS(content, m_content_length);

    //logln(std::string("HEADER ") + m_header);
    // if(m_content_length > 0)
    //   logln(std::string("POST ") + content);
    //  logln("@" + std::string(page));
    bool read_until=true;
    if(n==-1)
      {
	n=2048;
	read_until=false;
      }
    char buffer[n];
    m_client.readS((uint8_t *)buffer, n, false, read_until);//if n <0 then don't "read until"
    //printf("Received %d bytes: %s\n", n, buffer);

    char *d = strstr(buffer, "\r\n\r\n");
    int size = strtol(d, NULL, 16);
    d = strstr(d, "{");
    d[size] = '\0';
    std::string s(d);
    //logln("Received [" + std::to_string(size) + " bytes] : " + s, true);
    return d;
};

} // namespace Communication
