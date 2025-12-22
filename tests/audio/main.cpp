#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
#include "waveforms.cpp"

SDL_AudioSpec captureSpec;
SDL_AudioSpec playbackSpec;
SDL_AudioDeviceID captureDevice;
SDL_AudioDeviceID playbackDevice;
double sampleRate = 48000;

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


void analyzeFrequencies(int fftSamples) {
    SDL_PauseAudioDevice(captureDevice, 0);

    bool running = true;
    while(running) {
        while(SDL_GetQueuedAudioSize(captureDevice) < fftSamples) {
            SDL_Delay(10);
        }

        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) running = false;
        }

        std::vector<float> samples(fftSamples);
        int bytes = SDL_DequeueAudio(captureDevice, samples.data(), fftSamples * sizeof(float));

        getFrequencies(samples, (int)sampleRate);
    }
}

void playTones(std::string frequencyData) {
    SDL_PauseAudioDevice(playbackDevice, 0);
    bool running = true;
    while(running) {
        std::vector<float> waveForm = getWaveform(frequencyData, 8192, (int)sampleRate);
        normalize(waveForm);
        SDL_QueueAudio(playbackDevice, waveForm.data(), waveForm.size() * sizeof(float));
        std::cout << "Enqueued " << SDL_GetQueuedAudioSize(playbackDevice) << " samples\n";


        while(SDL_GetQueuedAudioSize(playbackDevice) > 0) {
            SDL_Delay(1);
            SDL_Event e;
            while(SDL_PollEvent(&e)) {
                if(e.type == SDL_QUIT) running = false;
            }
        }

    }

}

int main(int argc, const char* argv[]) {
    if (argc == 1) {
        std::cout << "No action provided" << std::endl;
    }
    setenv("SDL_AUDIODRIVER", "pipewire", 1);

    SDL_Init(SDL_INIT_AUDIO);
    init_capture();
    init_playback();

    std::string cmd(argv[1]);
    if (cmd == "detect") {
        analyzeFrequencies(atoi(argv[2]));
    }
    else if (cmd == "play") {
        playTones(argv[2]);
    }



    // for (int i = 0; i < 20; ++i)
    //     std::cout << samples[i] << " ";
    // std::cout << std::endl;
    // std::cout << "Dequeued " << bytes << " bytes\n";

    // std::cout << "delta-f: " << sampleRate / samplesNeeded << std::endl;

}
