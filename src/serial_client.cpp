#include "serial_client.hpp"

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

    logln("Connection in progress" + fstr("...", {BLINK_SLOW}));

    m_fd = open(path, flags);
    logln("Check connection: [fd:" + std::to_string(m_fd) + "] ");
    if(m_fd < 0)
        throw log_error("Could not open the serial port.");

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
    //tty.c_lflag &= ~IEXTEN; // Disable any special handling of received bytes
    //add ECHOK
    //tty.c_lflag &= ~ECHOK;
    //tty.c_lflag &= ~ECHOKE;
    //tty.c_lflag &= ~ECHOCTL;

    tty.c_oflag &= ~OPOST; // Prevent spe. interp. of out bytes (newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conv of newline to car. ret/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty.c_cc[VTIME] = 40; // Wait for up to 4s, ret when any data is received.
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

    //tcflush(m_fd, TCIFLUSH);
    // int n = TIOCM_DTR;
    // ioctl(m_fd, TIOCMBIS, &n);
    // usleep(2500);
    // ioctl(m_fd, TIOCMBIC, &n);
    // usleep(2500);

    // n= TIOCM_RTS;
    //    ioctl(m_fd, TIOCMBIS, &n);
    //    usleep(1000);
    //    ioctl(m_fd, TIOCMBIC, &n);

    // Save tty settings, also checking for error
    if(tcsetattr(m_fd, TCSANOW, &tty) != 0)
        throw m_id + "[ERROR] Could not set the serial port settings.";
    logln("Serial port settings saved (" + std::to_string(baud) + " baud).");
    m_is_connected = true;
    return m_fd;
}

int
Serial::readS(uint8_t *buffer, size_t size, bool has_crc, bool read_until)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    int n = read(m_fd, buffer, size);
    if(n != size && read_until)
        while(n != size) n += read(m_fd, buffer + n, size - n);

    if(has_crc)
        return check_CRC(buffer, size) ? n : -1;
    return n;
}

int
Serial::writeS(const void *buffer, size_t size, bool add_crc)
{
    std::lock_guard<std::mutex> lck(*m_mutex); //ensure only one thread using it
    int n = 0;
    if(add_crc)
        *(uint16_t *)((uint8_t *)buffer + size) =
            CRC((uint8_t *)buffer, size); // add crc
    return write(m_fd, buffer, size + 2 * add_crc);
}

} // namespace Communication
