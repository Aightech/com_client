#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include "http_client.hpp"
#include "serial_client.hpp"
#include "tcp_client.hpp"
#include "udp_client.hpp"
#include <lsl_cpp.h>

int
main(int argc, char **argv)
{
    if(argc < 4)
    {
        std::cout << ESC::fstr("Usage:\t", {ESC::BOLD}) << argv[0] << " mode path/ip port/baud"
                  << std::endl << std::endl
                << ESC::fstr("Example:\n\t", {ESC::BOLD}) << argv[0] << " serial /dev/ttyUSB0 115200" << std::endl
                << "\t" << argv[0] << " tcp 192.168.0.1 8080"
                    << std::endl << std::endl
                    << ESC::fstr("Description:", {ESC::BOLD}) << std::endl
                  << "\t- mode: serial, tcp, udp, http" << std::endl
                    << "\t- path/ip: path to serial port or ip address" << std::endl
                    << "\t- port/baud: port number or baud rate" << std::endl;
        exit(0);
    }
    //Mode can be "serial", "tcp", "udp", "http"
    std::string mode = argv[1];
    std::string path = argv[2];
    int opt = atoi(argv[3]);
    int flags = (mode == "serial") ? O_RDWR | O_NOCTTY : 1;
    std::cout << "Com Interface:" << std::endl;
    Communication::Client *device;
    int verbose=1;
    try
    {
        if(mode == "serial")
            device = new Communication::Serial(verbose);
        else if(mode == "tcp")
            device = new Communication::TCP(verbose);
        else if(mode == "udp")
            device = new Communication::UDP(verbose);
        else if(mode == "http")
            device = new Communication::HTTP(verbose);
        else
        {
            std::cout << "Mode not recognized" << std::endl;
            exit(0);
        }

        device->open_connection(path.c_str(), opt, flags);
    }
    catch(std::string msg)
    {
        std::cout << "ERROR: " << msg << "\n";
        return 0;
    }


    try
    {
        int nb_ch = 8;
        lsl::stream_info info_sample("forces", "sample", nb_ch, 0,
                                     lsl::cf_int32);
        lsl::stream_outlet outlet_sample(info_sample);
        std::vector<int32_t> sample(nb_ch);
        std::cout << "[INFOS] Now sending data... " << std::endl;
        for(int t = 0;; t++)
        {
            uint8_t buff[24] = {'1'};
            int a = device->writeS(buff, 1);
            std::cout << a << std::flush;
            int tot = 32;

            std::cout << " ... ";
            int n = device->readS(buff, tot, false);

            std::cout << n << std::endl;
            for(uint8_t i = 0; i < 8; i++)
            {
                sample[i] = ((int32_t *)(buff))[i];
                std::cout << sample[i] << " ";
            }
            std::cout << std::endl;
            outlet_sample.push_sample(sample);
        }
    }
    catch(std::exception &e)
    {
        std::cerr << "[ERROR] Got an exception: " << e.what() << std::endl;
    }
    uint8_t b[1] = {'n'};
    device->writeS(b, 1);

    return 0; // success
}
