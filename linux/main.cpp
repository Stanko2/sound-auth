#include "ggwave/ggwave.h"

#include "audio-control.cpp"

#include <cstdio>
#include <thread>

int main(int argc, char** argv) {
    AudioControl* a = new AudioControl(argc, argv);
    
    return 0;
}
