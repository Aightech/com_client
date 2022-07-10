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
    int port = atoi((argc < 2) ?"-1" : argv[2]);
    std::cout << "Com Interface:" << std::endl;
    Communication::Client device(true);

    try
    {
        if(port==-1)
            {
                device.open_connection(Communication::Client::SERIAL, path.c_str());
            }
            else
            {
                device.open_connection(Communication::Client::TCP, path.c_str(), port);
            }
    }
    catch(std::string msg)
    {
        std::cout << "ERROR: " << msg << "\n";
	return 0;
    }
    
    device.clean_start();

    try
    {
        int nb_ch = 8 * 8;
        lsl::stream_info info_sample("pressure", "sample", nb_ch, 0,
                                     lsl::cf_int32);
        lsl::stream_outlet outlet_sample(info_sample);
        std::vector<int32_t> sample(nb_ch);
        std::cout << "[INFOS] Now sending data... " << std::endl;
        for(int t = 0;; t++)
        {
            //val8 = clvHd.readReg<uint8_t>(15, 0x30);

            if(true) //val8 & 0b00100100)
            {
                //std::cout << " hoy " << std::endl;
                uint8_t buff[128] = {'\n'};
                device.writeS(buff, 1);
                int n = 0;
                while(n < 128) { n += device.readS(buff + n, 128 - n); }
                for(int i = 0; i < 8; i++)
                {
                    for(int j = 0; j < 8; j++)
                    {

                        std::cout << "  " << ((int16_t *)buff)[i * 8 + j];
                        sample[i * 8 + (8 - j)] = ((int16_t *)buff)[i * 8 + j];
                    }
                    std::cout << std::endl;
                }
                std::cout << std::endl;
                std::cout << std::endl;
                for(int i = 0; i < 8 * 8; i++)
                { sample[i] = ((int16_t *)buff)[i]; }

                //std::cout << std::hex << val16 << std::endl;
                outlet_sample.push_sample(sample);
            }
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
