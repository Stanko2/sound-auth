
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cassert>
#include <complex>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <fftw3.h>
#include <jni.h>
#include <string>
#include <android/log.h>
#include <sstream>
#include "transfer-lib/modulation.h"
#include "transfer-lib/waveforms.h"
#include "audio.cpp"


std::string jstring2string(JNIEnv *env, jstring jStr) {
    if (!jStr)
        return "";

    const jclass stringClass = env->GetObjectClass(jStr);
    const jmethodID getBytes = env->GetMethodID(stringClass, "getBytes", "(Ljava/lang/String;)[B");
    const jbyteArray stringJbytes = (jbyteArray) env->CallObjectMethod(jStr, getBytes, env->NewStringUTF("UTF-8"));

    size_t length = (size_t) env->GetArrayLength(stringJbytes);
    jbyte* pBytes = env->GetByteArrayElements(stringJbytes, NULL);

    std::string ret = std::string((char *)pBytes, length);
    env->ReleaseByteArrayElements(stringJbytes, pBytes, JNI_ABORT);

    env->DeleteLocalRef(stringJbytes);
    env->DeleteLocalRef(stringClass);
    return ret;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_PlayFrequencies(JNIEnv *env, jobject thiz,
                                                                 jstring data) {

    if (output_buffer == nullptr) {
        return;
    }
    std::string s = jstring2string(env, data);
    __android_log_print(ANDROID_LOG_DEBUG, "SOUND", "input: %s", s.c_str());
    auto* w = new Waveforms(0,0, new HannWindow(1024));
    std::vector<float> output = w->getWaveform(s, 1024, 48000);
    delete w;
    output_buffer->resize(output.size());

    for(float i : output) {
        output_buffer->add(i);
    }
}

std::atomic<bool> record_loop_running = false;
ProtocolConfig* p = nullptr;
SignalModulation* s = nullptr;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_CloseStreams(JNIEnv *env, jobject thiz) {
    record_loop_running = false;
    CloseStreams();
}

void RunRecordLoop(const ProtocolConfig *p, SignalModulation* s) {
    std::vector<float> frame(p->N);
    while(record_loop_running) {
        while(input_buffer->size() < p->N) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (!record_loop_running) break;
        for (int i = 0; i < p->N; i++) {
            input_buffer->pop(frame[i]);
        }
        s->enqueue_frame(frame);
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_OpenStreams(JNIEnv *env, jobject thiz) {
    ProtocolConfig* p = createProtocolConfig(1024);
    p->peak_threshold = 0.1f;
    std::vector<int> freqs = {p->f1, p->f1 + 10, p->f2, p->f2 + 10};
    OpenInputStream(*p);
    OpenOutputStream(*p);
    input_buffer = new Ringbuffer<float>(7*p->N);
    output_buffer = new Ringbuffer<float>(0);
    StartStreams();
    record_loop_running = true;
    s = new SignalModulation(*p);
    s->set_strategy(new TwoTonePerBitModulationStrategy(freqs));
    std::thread thread(RunRecordLoop, p, s);
    thread.detach();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_sendData(JNIEnv *env, jobject thiz,
                                                          jbyteArray data) {
    if (s == nullptr) return;
    jsize len = env->GetArrayLength(data);
    std::vector<uint8_t> msg(len);
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(msg.data()));
    std::cout << "Len: " << len << std::endl;

    std::vector<float> output = s->transmit_data(msg);
    output_buffer->resize(output.size());
    for(float i : output) {
        output_buffer->add(i);
    }
    std::cout << "Output size: " << output_buffer->size() << std::endl;
}