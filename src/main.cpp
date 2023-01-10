#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include "com_client.hpp"
#include <lsl_cpp.h>

int
main(int argc, char **argv)
{
    if(argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " path/address [port]"
                  << std::endl;
        exit(0);
    }
    std::string path = argv[1];
    int port = atoi((argc < 2) ? "-1" : argv[2]);
    std::cout << "Com Interface:" << std::endl;
    Communication::Client device(true);

    try
    {
        if(port == -1)
        {
            device.open_connection(Communication::Client::SERIAL, path.c_str());
        }
        else
        {
            device.open_connection(Communication::Client::TCP, path.c_str(),
                                   port);
        }
    }
    catch(std::string msg)
    {
        std::cout << "ERROR: " << msg << "\n";
        return 0;
    }

    //    device.clean_start();

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
            int a = device.writeS(buff, 1);
            int tot = 32;
            int n = device.readS(buff, tot);
            while(n < tot) n += device.readS(buff + n, tot - n);
            for(uint8_t i = 0; i < 8; i++)
	      sample[i] = ((int32_t*)(buff))[i];

            outlet_sample.push_sample(sample);
        }
    }
    catch(std::exception &e)
    {
        std::cerr << "[ERROR] Got an exception: " << e.what() << std::endl;
    }
    uint8_t b[1] = {'n'};
    device.writeS(b, 1);

    return 0; // success
}
