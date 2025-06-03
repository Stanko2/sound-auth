
#include "audio/audio-control.cpp"
#include "audio/communication.cpp"
#include "pam/otp.cpp"
#include "config.h"

#include <cstdint>
#include <cstdio>
#include <csignal>
#include <cstring>
#include <iostream>
#include <vector>
#include "pam/setup.cpp"

AudioControl* a;

void stop (int signal) {
    std::cout << "Ending" << std::endl;
    a->end_loop();
}

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
    std::signal(SIGINT, stop);


    std::vector<unsigned char> message;
    // std::vector<unsigned char> challenge = get_challenge();

    int ret = 0;
    if(argc > 1) {
        std::string msg;
        if (strncmp(argv[1], "send", 10) == 0) {
            FILE* f = fopen(argv[1], "r");
            while (1) {
                msg += fgetc(f);

                if (feof(f) || msg.size() >= 120)
                    break;
            }
            message = std::vector<unsigned char>(msg.begin(), msg.end());
            std::cout << message.size() << std::endl;

            c->send_broadcast(message);
        }
        else if (strncmp(argv[1], "setup", 10) == 0) {
            ret = runSetup(c);
        } else if (strncmp(argv[1], "list", 10) == 0) {
            a->listAllDevices();
        } else if (strncmp(argv[1], "test", 10) == 0) {
            bool r = runAuth(c);
            ret = !r;
        }

    } else {
        help();
        return 0;
    }
    a->end_loop();
    delete a;

    // c->receive_callback = [c, challenge]() {
    //     std::vector<uint8_t> data;

    //     int len = c->get_data(data);
    //     std::cout << "Verify " << verify(data, challenge) << std::endl;
    // };


    return ret;
}
