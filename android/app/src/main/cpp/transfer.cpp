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
#include "tests/transferTest.h"

SoundTransfer* sound_transfer;

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

    if (sound_transfer == nullptr) {
        return;
    }
    std::string s = jstring2string(env, data);
    __android_log_print(ANDROID_LOG_DEBUG, "SOUND", "input: %s", s.c_str());
    auto* w = new Waveforms(0,0, new HannWindow(1024));
    std::vector<float> output = w->getWaveform(s, 1024, 48000);
    delete w;
//    output_buffer->resize(output.size());

    for(float i : output) {
        sound_transfer->get_output_buffer()->add(i);
    }
}



extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_CloseStreams(JNIEnv *env, jobject thiz) {
    CloseStreams();
    sound_transfer->stop();
    sound_transfer = nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_OpenStreams(JNIEnv *env, jobject thiz) {
    ProtocolConfig* p = createProtocolConfig(1024, 48000, 6000, 8000);
    p->lowest_strength = -110; //-125;
    p->strength_threshold = -90; //-90;

    ModulationStrategy* strategy = new TwoTonePerBitModulationStrategy(p->f1, 4, 5);
    sound_transfer = new SoundTransfer(strategy, p);
    OpenInputStream(*p, sound_transfer);
    OpenOutputStream(*p, sound_transfer);
    sound_transfer->run();

    StartStreams();
}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_sendData(JNIEnv *env, jobject thiz,
                                                          jbyteArray data) {
    if (sound_transfer == nullptr) return;

    jsize len = env->GetArrayLength(data);
    std::vector<uint8_t> msg(len);
    env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(msg.data()));
    sound_transfer->send(msg);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_testTx(JNIEnv *env, jobject thiz) {
    if (sound_transfer == nullptr) return;
    test_tx(sound_transfer, 8, 32);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_SoundTestingScreen_testRx(JNIEnv *env, jobject thiz) {
    if (sound_transfer == nullptr) return;
    test_rx(sound_transfer, 8, 32);
}