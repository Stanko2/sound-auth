//
// Created by stanko on 2/17/26.
//
#include <aaudio/AAudio.h>
#include "modulation.h"
#include "RingBuffer.h"
#include "entry.h"

extern "C" void AAudioStreamBuilder_setInputPreset(AAudioStreamBuilder* builder, aaudio_input_preset_t inputPreset) __attribute__((weak));

AAudioStreamStruct * outputStream;
AAudioStreamStruct * inputStream;

aaudio_data_callback_result_t OutputDataCallback(
        AAudioStream* stream, void* userData, void* audioData, int32_t length
) {
    auto* t = (SoundTransfer*) userData;
    auto* waveform = (float*) audioData;
    memset(audioData, 0, length * sizeof(float));

    if (t == nullptr) {
        std::cout << "Not initialized" << std::endl;
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }

    int i = 0;
    while (!t->get_output_buffer()->empty() && i < length) {
        t->get_output_buffer()->pop(waveform[i]);
        i++;
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

aaudio_data_callback_result_t InputDataCallback(
        AAudioStream* stream, void* userData, void* audioData, int32_t length
) {
    auto* t = (SoundTransfer*) userData;
    auto* samples = (float*) audioData;
    if (t == nullptr || samples == nullptr) {
        std::cout << "Not initialized" << std::endl;
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    for(int i = 0; i < length; i++) {
        t->get_input_buffer()->add(samples[i]);
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void setStreamParams(AAudioStreamBuilder* &builder, const ProtocolConfig &p) {
//    AAudioStreamBuilder_setSampleRate(builder, p.sample_rate);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_UNPROCESSED);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
}

void OpenInputStream(ProtocolConfig &p, SoundTransfer* t) {
    AAudioStreamBuilder* builder;
    AAudio_createStreamBuilder(&builder);

    setStreamParams(builder, p);

    AAudioStreamBuilder_setDataCallback(builder, InputDataCallback, t);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    if (AAudioStreamBuilder_openStream(builder, &inputStream) != AAUDIO_OK) {
        AAudioStreamBuilder_delete(builder);
        return;
    }

    AAudioStreamBuilder_delete(builder);
}

void OpenOutputStream(ProtocolConfig &p, SoundTransfer* t) {
    AAudioStreamBuilder* builder;
    AAudio_createStreamBuilder(&builder);

    setStreamParams(builder, p);

    AAudioStreamBuilder_setDataCallback(builder, OutputDataCallback, t);
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    if (AAudioStreamBuilder_openStream(builder, &outputStream) != AAUDIO_OK) {
        std::cout << "Audio error" << std::endl;
        AAudioStreamBuilder_delete(builder);
        return;
    }

    AAudioStreamBuilder_delete(builder);

    int32_t actualRate = AAudioStream_getSampleRate(inputStream);
    std::cout << "Actual sample rate: " << actualRate << std::endl;
    aaudio_input_preset_t preset =
            AAudioStream_getInputPreset(inputStream);
    std::cout << "Actual preset " << preset << std::endl;
}

void StartStreams() {
    if (inputStream) {
        AAudioStream_requestStart(inputStream);
    }
    if (outputStream) {
        AAudioStream_requestStart(outputStream);
    }
}


void CloseStreams() {
    if (outputStream != nullptr) {
        AAudioStream_requestStop(outputStream);
        AAudioStream_close(outputStream);
        outputStream = nullptr;
    }
    if (inputStream != nullptr) {
        AAudioStream_requestStop(inputStream);
        AAudioStream_close(inputStream);
        inputStream = nullptr;
    }
}

