
#include "audio/audio-control.cpp"
#include "audio/communication.cpp"

#include <cstdio>
#include <thread>
#include <csignal>

AudioControl* a;

void stop (int signal) {
    std::cout << "Ending" << std::endl;
    a->end_loop();
}

int main(int argc, char** argv) {
    a = new AudioControl();
    Communication* c = new Communication(a);
    std::signal(SIGINT, stop);

    if(argc > 1) {
        std::string msg;
        FILE* f = fopen(argv[1], "r");
        while (1) {
            msg += fgetc(f);

            if (feof(f) || msg.size() >= 120)
                break;
        }
        std::vector<uint8_t> message(msg.begin(), msg.end());
        c->encode_message(message);
        std::vector<uint8_t> waveform = c->get_waveform();
        a->queue_audio(waveform);
    }

    a->start_loop();

    return 0;
}
