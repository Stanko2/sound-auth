
#include "audio/audio-control.h"
#include "audio/communication.h"
#include "pam/otp.cpp"
#include "config.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "pam/setup.cpp"
#include "sound-transfer-lib/tests/transferTest.h"

AudioControl* a;

void help() {
    std::cout << "sound-auth usage: sound-auth [COMMAND]" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  send [FILE] sends the [FILE] to all devices" << std::endl;
    std::cout << "  setup       generates keys and broadcasts them." << std::endl;
    std::cout << "  test        tries to run whole authentication process for current user" << std::endl;
}

int main(int argc, char** argv) {
    a = new AudioControl();
    AuthConfig config = AuthConfig::instance();
    Communication* c = new Communication(a, config.getAddress().data());

    std::vector<unsigned char> message;
    // std::vector<unsigned char> challenge = get_challenge();

    int ret = 0;
    if(argc > 1) {
        if (strncmp(argv[1], "send", 10) == 0) {
            std::ifstream f(argv[2], std::ios::in | std::ios::binary);
            if(!f) {
                std::cerr << "Could not find file: " << argv[1] << std::endl;
                ret = 1;
            }
            else {
                std::ostringstream oss;
                oss << f.rdbuf();
                std::string msg = oss.str();
                f.close();
                message = std::vector<uint8_t>(msg.begin(), msg.end());

                c->send_broadcast(message);
                ret = 0;
            }
        }
        else if (strncmp(argv[1], "setup", 10) == 0) {
            ret = runSetup(c);
        } else if (strncmp(argv[1], "list", 10) == 0) {
            a->listAllDevices();
        } else if (strncmp(argv[1], "test", 10) == 0) {
            bool r = runAuth(c);
            ret = !r;
        } else if (strncmp(argv[1], "test-tx", 10) == 0) {
          test_tx(c->get_transfer(), 8);
        } else if (strncmp(argv[1], "test-rx", 10) == 0) {
          test_rx(c->get_transfer(), 8);
        }

    } else {
        help();
        return 0;
    }
    a->end_loop();
    delete a;

    return ret;
}
