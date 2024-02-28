#include "http_client.hpp"

namespace Communication
{

HTTP::HTTP(int verbose) : TCP(verbose), ESC::CLI(verbose, "HTTP_client")
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

std::string
HTTP::get(const char *page, int n)
{
    char buffer[n];
    sprintf(m_header, m_header_get, page, m_ip.c_str());
    writeS(m_header, strlen(m_header));
    bool read_until = true;
    if(n == -1)
    {
        n = 2048;
        read_until = false;
    }
    readS((uint8_t *)buffer, n, false,
          read_until); //if n <0 then don't "read until"

    char *d = strstr(buffer, "\r\n\r\n");
    int size = strtol(d, NULL, 16);
    d = strstr(d, "{");
    d[size] = '\0';
    std::string s(d);
    //logln("Received [" + std::to_string(size) + " bytes] : " + s, true);
    return d;
};

std::string
HTTP::post(const char *page, const char *content, int n)
{
    m_content_length = content ? strlen(content) : 0;
    if(m_content_length > 0)
        sprintf(m_header, m_header_post_with_data, page, m_ip.c_str(),
                m_content_length);
    else
        sprintf(m_header, m_header_post, page, m_ip.c_str());
    writeS(m_header, strlen(m_header));
    if(m_content_length > 0)
        writeS(content, m_content_length);

    //if no n is specified, read once 2048 bytes max
    bool read_until = true;
    if(n == -1)
    {
        n = 2048;
        read_until = false;
    }
    char buffer[n];
    n = readS((uint8_t *)buffer, n, false,
              read_until); //if n <0 then don't "read until"

    //get the code of the response
    char *c = strstr(buffer, "HTTP/1.1 ");
    if(c == NULL)
        throw log_error("Code HTTP/1.1 not found in header in : " +
                        std::string(buffer));
    int code = strtol(c + 9, NULL, 10);
    std::string code_str(c + 9, 3);

    if(code != 200)
    {
        logln(std::string("HEADER SENT:\n") + m_header, true);
        if(m_content_length > 0)
            logln(std::string("Content:\n") + content, true);
        logln("Received [" + std::to_string(n) + " bytes] :\n" +
                  std::string(buffer),
              true);
        throw log_error("HTTP code " + code_str);
    }
    //get the content length
    c = strstr(buffer, "\r\n\r\n");
    if(c == NULL)
        throw log_error("Content-Length not found in buffer");
    int size = strtol(c, NULL, 16);

    c = strstr(c, "{");
    c[size] = '\0';
    std::string s(c);
    logln(ESC::fstr("OK ", {ESC::FG_GREEN, ESC::BOLD}), true);
    return c;
};

} // namespace Communication
