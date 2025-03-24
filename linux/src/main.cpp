
#include "audio/audio-control.cpp"
#include "audio/communication.cpp"
#include "pam/otp.cpp"

#include <cstdio>
#include <csignal>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>
#include "pam/setup.cpp"

AudioControl* a;

void stop (int signal) {
    std::cout << "Ending" << std::endl;
    a->end_loop();
}

int main(int argc, char** argv) {
    a = new AudioControl();
    Communication* c = new Communication(a);
    std::signal(SIGINT, stop);

    std::vector<unsigned char> message;
    std::vector<unsigned char> challenge = get_challenge();


    if(argc > 1) {
        std::string msg;
        if (strncmp(argv[1], "send", 10) == 0) {
            FILE* f = fopen(argv[1], "r");
            while (1) {
                msg += fgetc(f);

                if (feof(f) || msg.size() >= 120)
                    break;
            }
            std::vector<uint8_t> message(msg.begin(), msg.end());
        }
        else if (strncmp(argv[1], "setup", 10) == 0) {
            if (generateSecretCode()) {
                std::cerr << "Error generating new secret code" << std::endl;
                return -1;
            }
            std::cout << "credentials for user " << getUsername() << " generated successfully" << std::endl;
            std::cout << "press [ENTER] when your phone is listening for the credentials" << std::endl;
            std::string x;
            std::cin.get();
            std::string setup = getSetupMessage();
            std::copy(setup.begin(), setup.end(), std::back_inserter(message));
            c->encode_message(message);
            std::vector<uint8_t> waveform = c->get_waveform();
            a->queue_audio(waveform);
            c->receive_callback = []() {
                std::cout << "Setup data was transferred into phone successfully" << std::endl;
            };

            a->start_loop();
            return 0;
        }

    } else {
        message = challenge;
        std::cout << message.size() << std::endl;
    }

    c->encode_message(message);
    std::vector<uint8_t> waveform = c->get_waveform();
    a->queue_audio(waveform);

    c->receive_callback = [c, challenge]() {
        std::vector<uint8_t> data;

        int len = c->get_data(data);
        std::cout << "Verify " << verify(data, challenge) << std::endl;
    };

    a->start_loop();

    return 0;
}
