#ifndef __Serial_CLIENT_HPP__
#define __Serial_CLIENT_HPP__

#include "com_client.hpp"
#include <iostream>
#include <fcntl.h>
#include <cstdint>

#ifdef _WIN32
#define O_NOCTTY 0
#elif __APPLE__
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#endif

namespace Communication
{
class Serial : public Client
{
    public:
    Serial(int verbose = -1);
    ~Serial() = default;

    /**
     * @brief Open the serial connection
     * @param path Path to the serial device
     * @param baud Baud rate (e.g., 9600, 19200, 38400, 500000, etc.)
     * @param flags Open flags (default is O_RDWR | O_NOCTTY)
     * @return File descriptor of the opened serial port
     */
    int open_connection(const char *path, int baud = 9600, int flags = O_RDWR | O_NOCTTY);

    /**
     * @brief Read from the serial interface
     * @param buffer Buffer to store the read data
     * @param size Number of bytes to read
     * @param has_crc If true, validates the last two bytes as CRC16
     * @param read_until If true, continues reading until 'size' bytes are read
     * @return Number of bytes read
     */
    int readS(uint8_t *buffer, size_t size, bool has_crc = false, bool read_until = true);

    /**
     * @brief Write to the serial interface
     * @param buffer Data to write
     * @param size Number of bytes to write
     * @param add_crc If true, adds a CRC16 checksum to the data (ensure buffer has enough space)
     * @return Number of bytes written
     */
    int writeS(const void *buffer, size_t size, bool add_crc = false);

    private:
    // 追加のメンバ変数があればここに宣言します
};

} // namespace Communication

#endif // __Serial_CLIENT_HPP__
