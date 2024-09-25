#include "serial_client.hpp"

#ifdef __linux__
#include <linux/input.h>
#elif _WIN32
#include <windows.h>
#include <commctrl.h>
#elif __APPLE__
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#endif

namespace Communication
{

using namespace ESC;

Serial::Serial(int verbose)
    : Client(verbose), ESC::CLI(verbose, "Serial-Client")
{
}

int
Serial::open_connection(const char *path, int baud, int flags)
{
    std::string o_mode = "";
    std::string mode;
    if((flags & O_RDWR) == O_RDWR)
        mode = "rw";
    else if((flags & O_RDONLY) == O_RDONLY)
        mode = "ro";
    else if((flags & O_WRONLY) == O_WRONLY)
        mode = "wo";
    else
        mode = "?";
    cli_id() += ((cli_id() == "") ? "" : " - ") +
                fstr_link(std::string(path) + ":" + mode);

    logln("Connection in progress" + fstr("...", {BLINK_SLOW}), true);

#ifdef __linux__
    m_fd = open(path, flags);
#elif _WIN32
    // Windows-specific code
    DWORD dwAccess = 0;
    if((flags & O_RDWR) == O_RDWR)
        dwAccess = GENERIC_READ | GENERIC_WRITE;
    else if((flags & O_RDONLY) == O_RDONLY)
        dwAccess = GENERIC_READ;
    else if((flags & O_WRONLY) == O_WRONLY)
        dwAccess = GENERIC_WRITE;
    else
        dwAccess = 0;
    DWORD dwShareMode = 0;
    if((flags & O_NOCTTY) == O_NOCTTY)
        dwShareMode = 0;
    else
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    DWORD dwCreationDisposition = 0;
    if((flags & O_CREAT) == O_CREAT)
        dwCreationDisposition = CREATE_ALWAYS;
    else
        dwCreationDisposition = OPEN_EXISTING;
    DWORD dwFlagsAndAttributes = 0;

    HANDLE fd = CreateFile(path, dwAccess, dwShareMode, NULL, dwCreationDisposition,
                           dwFlagsAndAttributes,
                           0);
    m_fd = (uint64_t)fd;
#elif __APPLE__
    m_fd = open(path, flags | O_NOCTTY | O_NDELAY);
#endif

    logln("Check connection: [fd:" + std::to_string(m_fd) + "] ");

#ifdef __linux__
    if(m_fd == -1)
    {
        logln("Scanning input for \"" + std::string(path) + "\"");
        std::string dev_path;
        std::string dev_name = path;
        for(int i = 0; i < 40; i++)
        {
            dev_path = "/dev/input/event" + std::to_string(i);
            m_fd = open(dev_path.c_str(), O_RDWR);
            if(m_fd > 0)
            {
                char test[256];
                ioctl(m_fd, EVIOCGNAME(sizeof(test)), test);
                if(dev_name == std::string(test))
                    break;
                close(m_fd);
                m_fd = -1;
            }
        }
        m_fd = open(dev_path.c_str(), O_RDWR);
    }
    if(m_fd < 0)
        throw log_error("Could not open the serial port.");
#elif _WIN32
    if((HANDLE)m_fd == INVALID_HANDLE_VALUE)
        throw log_error("Could not open the serial port.");
#elif __APPLE__
    if(m_fd == -1)
        throw log_error("Could not open the serial port.");
#endif

#ifdef __linux__
    // Linux-specific serial port settings
    struct termios tty;
    if(tcgetattr(m_fd, &tty) != 0)
        throw log_error("Could not get the serial port settings.");

    tty.c_cflag &= ~PARENB;  // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB;  // Clear stop field, only 1 stop bit (most common)
    tty.c_cflag &= ~CSIZE;   // Clear all bits that set the data size
    tty.c_cflag |= CS8;      // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow ctrl (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrlline(CLOCAL = 1)
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;   // Disable echo
    tty.c_lflag &= ~ECHOE;  // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                     ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    tty.c_cc[VTIME] = 40; // Wait for up to 4s, returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate
    int speed;
    switch(baud)
    {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    case 460800:
        speed = B460800;
        break;
    case 500000:
        speed = B500000;
        break;
    case 921600:
        speed = B921600;
        break;
    case 1000000:
        speed = B1000000;
        break;
    default:
        throw log_error("Unsupported baud rate");
    }

    cfsetispeed(&tty, speed);
    cfsetospeed(&tty, speed);

    if(tcsetattr(m_fd, TCSANOW, &tty) != 0)
        throw m_id + "[ERROR] Could not set the serial port settings.";
    logln("Serial port settings saved (" + std::to_string(baud) + " baud).");
#elif _WIN32
    // Windows-specific serial port settings
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if(!GetCommState((HANDLE)m_fd, &dcbSerialParams))
        throw log_error("Could not get the serial port settings.");

    dcbSerialParams.BaudRate = baud;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;
    dcbSerialParams.fBinary = TRUE;
    dcbSerialParams.fParity = FALSE;
    dcbSerialParams.fNull = FALSE;
    dcbSerialParams.fAbortOnError = FALSE;
    dcbSerialParams.fErrorChar = FALSE;
    dcbSerialParams.fTXContinueOnXoff = FALSE;

    if(!SetCommState((HANDLE)m_fd, &dcbSerialParams))
        throw log_error("Could not set the serial port settings.");
    logln("Serial port settings saved (" + std::to_string(baud) + " baud).");
#elif __APPLE__
    // macOS-specific serial port settings
    struct termios tty;
    if(tcgetattr(m_fd, &tty) != 0)
        throw log_error("Could not get the serial port settings.");

    cfmakeraw(&tty);

    // 一旦標準のボーレートを設定（B38400を使用）
    cfsetispeed(&tty, B38400);
    cfsetospeed(&tty, B38400);

    tty.c_cflag |= (CLOCAL | CREAD);    // Enable receiver, local mode
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;                 // 8 data bits
    tty.c_cflag &= ~PARENB;             // No parity
    tty.c_cflag &= ~CSTOPB;             // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;            // No hardware flow control

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // Raw input
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // No flow control
    tty.c_oflag &= ~OPOST;                          // Raw output

    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 40; // 4 seconds timeout

    if(tcsetattr(m_fd, TCSANOW, &tty) != 0)
        throw log_error("Could not set the serial port settings.");

    // カスタムボーレートを設定
    speed_t speed = baud;
    if(ioctl(m_fd, IOSSIOSPEED, &speed) == -1)
        throw log_error("Could not set custom baud rate.");

    logln("Serial port settings saved (" + std::to_string(baud) + " baud).");
#endif

    m_is_connected = true;
    return m_fd;
}

int
Serial::readS(uint8_t *buffer, size_t size, bool has_crc, bool read_until)
{
    std::lock_guard<std::mutex> lck(*m_mutex); // Ensure only one thread uses it
    if(m_is_connected)
    {
#if defined(__linux__) || defined(__APPLE__)
        int n = read(m_fd, buffer, size);
        if(n != size && read_until)
            while(n != size) n += read(m_fd, buffer + n, size - n);
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
Serial::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); // Ensure only one thread uses it
    if(m_is_connected)
    {
        if(add_crc)
            *(uint16_t *)((uint8_t *)buffer + size) =
                CRC((uint8_t *)buffer, size); // Add CRC
#if defined(__linux__) || defined(__APPLE__)
        return write(m_fd, buffer, size + 2 * add_crc);
#elif _WIN32
        DWORD n = 0;
        if(!WriteFile((HANDLE)m_fd, buffer, size + 2 * add_crc, &n, NULL))
            throw log_error("Could not write the serial port.");
        return n;
#endif
    }
    return -1;
}

} // namespace Communication
