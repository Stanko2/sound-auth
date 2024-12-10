
#include "audio-control.cpp"
#include "communication.cpp"

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
    a->start_loop();

    return 0;
}
