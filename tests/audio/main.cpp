#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <ostream>
#include <vector>
#include "lib/waveforms.h"
#include "lib/modulation.h"
// #include "lib/wavExport.cpp"

SDL_AudioSpec captureSpec;
SDL_AudioSpec playbackSpec;
SDL_AudioDeviceID captureDevice;
SDL_AudioDeviceID playbackDevice;
double sampleRate = 48000;
ProtocolConfig p;

void init_capture() {
    SDL_zero(captureSpec);

    captureSpec.freq = sampleRate;
    captureSpec.format = AUDIO_F32;
    captureSpec.channels = 1;
    captureSpec.samples = 1024;
    captureSpec.callback = NULL;

    SDL_AudioSpec obtained;
    SDL_zero(obtained);

    captureDevice =
        SDL_OpenAudioDevice(NULL, SDL_TRUE, &captureSpec, &obtained, 0);


    if (!captureDevice) {
        std::cerr << "Failed to open audio capture device" << std::endl;
    }
}

void init_playback() {
    SDL_zero(playbackSpec);

    playbackSpec.freq = sampleRate;
    playbackSpec.format = AUDIO_F32;
    playbackSpec.channels = 1;
    playbackSpec.samples = 1024;
    playbackSpec.callback = NULL;

    SDL_AudioSpec obtained;
    SDL_zero(obtained);

    playbackDevice =
        SDL_OpenAudioDevice(NULL, SDL_FALSE, &captureSpec, &obtained, 0);


    if (!playbackDevice) {
        std::cerr << "Failed to open audio playback device" << std::endl;
    }
}


void analyzeFrequencies() {
    SDL_PauseAudioDevice(captureDevice, 0);

    bool running = true;
    // std::ofstream outputFile("data.wav", std::ios::binary);
    SignalModulation* s = new SignalModulation(p);
    // int total = 0;
    while(running) {
        while(SDL_GetQueuedAudioSize(captureDevice) < p.N * sizeof(float)) {
            SDL_Delay(10);
        }

        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }

        std::vector<float> samples(p.N);
        int bytes = SDL_DequeueAudio(captureDevice, samples.data(), p.N * sizeof(float));

        // if (bytes > 0) {
        //     outputFile.write(reinterpret_cast<const char*>(samples.data()), bytes);
        //     total += bytes;
        // }


        // get_spectrum(samples, (int)sampleRate);
        s->enqueue_frame(samples);
    }

    delete s;
    // finalizeWav(outputFile, total, 1, p.sample_rate, 32);
}

void playTones(std::string frequencyData) {
    SDL_PauseAudioDevice(playbackDevice, 0);
    bool running = true;

    Waveforms* w = new Waveforms(0,0);

    std::vector<float> waveForm = w->getWaveform(frequencyData, 2048, (int)sampleRate);
    // normalize(waveForm);
    SDL_QueueAudio(playbackDevice, waveForm.data(), waveForm.size() * sizeof(float));
    std::cout << "Enqueued " << SDL_GetQueuedAudioSize(playbackDevice) / 4 << " samples\n";


    while(SDL_GetQueuedAudioSize(playbackDevice) > 0) {
        SDL_Delay(1);
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }
    }
    // }
}

void sendData(std::string data) {
    SDL_PauseAudioDevice(playbackDevice, 0);
    bool running = true;

    SignalModulation* s = new SignalModulation(p);

    std::vector<uint8_t> vec(data.begin(), data.end());
    waveform w = s->transmit_data(vec);
    SDL_QueueAudio(playbackDevice, w.data(), w.size() * sizeof(float));

    while(SDL_GetQueuedAudioSize(playbackDevice) > 0) {
        SDL_Delay(1);
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }
    }

}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        std::cout << "No action provided" << std::endl;
    }
    setenv("SDL_AUDIODRIVER", "pipewire", 1);

    p.N = 1024;
    p.sample_rate = 48000;
    p.peak_threshold = 50;
    p.f1 = 15000 * p.N / p.sample_rate;
    p.f2 = 17000 * p.N / p.sample_rate;

    // std::cout << "f1: " << p.f1 << "f2: " << p.f2 << std::endl;

    SDL_Init(SDL_INIT_AUDIO);
    init_capture();
    init_playback();

    std::string cmd(argv[1]);
    if (cmd == "detect") {
        analyzeFrequencies();
    }
    else if (cmd == "play") {
        playTones(argv[2]);
    } else if (cmd == "send") {
        sendData(argv[2]);
    }



    // for (int i = 0; i < 20; ++i)
    //     std::cout << samples[i] << " ";
    // std::cout << std::endl;
    // std::cout << "Dequeued " << bytes << " bytes\n";

    // std::cout << "delta-f: " << sampleRate / samplesNeeded << std::endl;

}
