#include "http_client.hpp"

namespace Communication
{

HTTP::HTTP(int verbose) : ESC::CLI(verbose, "HTTP_client"), TCP(verbose)
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
    char *buffer = new char[n];
    snprintf(m_header, 2048, m_header_get, page, m_ip.c_str());
    writeS(m_header, strlen(m_header));
    bool read_until = true;
    if(n == -1)
    {
        n = 2048;
        read_until = false;
    }
    int bytes_read = readS((uint8_t *)buffer, n, false, read_until);
    if(bytes_read > 0 && bytes_read < n)
        buffer[bytes_read] = '\0';
    else if(bytes_read >= n)
        buffer[n - 1] = '\0';

    // Find body after headers
    char *body = strstr(buffer, "\r\n\r\n");
    if(body == NULL)
    {
        delete[] buffer;
        throw log_error("Malformed HTTP response: no header delimiter");
    }
    body += 4; // skip \r\n\r\n

    std::string result(body);
    delete[] buffer;
    return result;
}

std::string
HTTP::post(const char *page, const char *content, int n)
{
    m_content_length = content ? strlen(content) : 0;
    if(m_content_length > 0)
        snprintf(m_header, 2048, m_header_post_with_data, page, m_ip.c_str(),
                m_content_length);
    else
        snprintf(m_header, 2048, m_header_post, page, m_ip.c_str());
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
    char *buffer = new char[n];
    int bytes_read = readS((uint8_t *)buffer, n, false, read_until);
    if(bytes_read > 0 && bytes_read < n)
        buffer[bytes_read] = '\0';
    else if(bytes_read >= n)
        buffer[n - 1] = '\0';

    //get the code of the response
    char *c = strstr(buffer, "HTTP/1.1 ");
    if(c == NULL)
    {
        delete[] buffer;
        throw log_error("HTTP/1.1 not found in response");
    }
    int code = strtol(c + 9, NULL, 10);
    std::string code_str(c + 9, 3);

    if(code != 200)
    {
        logln(std::string("HEADER SENT:\n") + m_header, true);
        if(m_content_length > 0)
            logln(std::string("Content:\n") + content, true);
        logln("Received [" + std::to_string(bytes_read) + " bytes] :\n" +
                  std::string(buffer),
              true);
        delete[] buffer;
        throw log_error("HTTP code " + code_str);
    }

    // Find body after headers
    char *body = strstr(buffer, "\r\n\r\n");
    if(body == NULL)
    {
        delete[] buffer;
        throw log_error("Malformed HTTP response: no header delimiter");
    }
    body += 4; // skip \r\n\r\n

    std::string result(body);
    logln(ESC::fstr("OK ", {ESC::FG_GREEN, ESC::BOLD}), true);
    delete[] buffer;
    return result;
}

} // namespace Communication
