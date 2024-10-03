#ifndef __Serial_CLIENT_HPP__
#define __Serial_CLIENT_HPP__

#include "com_client.hpp"
#include <iostream>
#include <fcntl.h>

#ifdef _WIN32
#define O_NOCTTY 0
#endif

namespace Communication
{
class Serial : public Client
{
    public:
    Serial(int verbose = -1);
    ~Serial(){};

    /**
     * @brief open_connection Open the connection the serial or network or interface
     * @param address Path or IP address
     * @param port -1 to open a serial com, else the value of the port to listen/write
     * @param flags Some flags
     * @return
     */
    int
    open_connection(const char *path,
                    int baud = 9600,
                    int flags = O_RDWR | O_NOCTTY);

    /**
     * @brief readS read the com inmterface.
     * @param buffer Data store in buffer.
     * @param size Nb of bytes to read.
     * @param has_crc If true the two last bytes are checked as a CRC16.
     * @param read until loop until "size" bytes have been read.
     * @return number of bytes read.
     */
    int
    readS(uint8_t *buffer,
          size_t size,
          bool has_crc = false,
          bool read_until = true);

    /**
     * @brief writeS write the com interface.
     * @param buffer Data to write.
     * @param size Nb of bytes to write.
     * @param add_crc If true two more bytes are added to the buffer to store a CRC16. Be sure to have enough space in the buffer.
     * @return number of bytes written.
     */
    int
    writeS(const void *buffer, size_t size, bool add_crc = false);

    private:
    /* data */
};

} // namespace Communication

#endif // __Serial_CLIENT_HPP__