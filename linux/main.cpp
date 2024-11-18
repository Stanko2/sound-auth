
#include "audio-control.cpp"
#include "communication.cpp"

#include <cstdio>
#include <thread>

int main(int argc, char** argv) {
    AudioControl* a = new AudioControl(new Communication());
    
    return 0;
}
