//
// Created by stanko on 10/28/24.
//

#include <jni.h>
#include "ggwave/ggwave.h"
#include <string>

GGWave* ggWave;
char* msgBuffer;
int msg_buffer_size;
float sample_rate;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_soundauth_ListenService_initGGWave(JNIEnv *env, jobject thiz, jfloat rate, jint buffer_size) {
    sample_rate = rate;
//    msgBuffer = new char [buffer_size];
//    msg_buffer_size = buffer_size;
//    ggWave = std::make_shared<GGWave>();
//    return ggWave != nullptr;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_soundauth_ListenService_encode(JNIEnv * env, jobject thiz, jbyteArray message, jint messageSize) {
    memset(msgBuffer, 0, msg_buffer_size);
//    ggWave->init((int) messageSize, (const char*) message, GGWAVE_PROTOCOL_AUDIBLE_NORMAL, 50);
    ggWave = new GGWave(GGWave::Parameters  {
        -1,
        GGWave::kDefaultSampleRate,
        GGWave::kDefaultSampleRate,
        GGWave::kDefaultSampleRate,
        GGWave::kDefaultSamplesPerFrame,
        GGWave::kDefaultSoundMarkerThreshold,
        GGWAVE_SAMPLE_FORMAT_I8,
        GGWAVE_SAMPLE_FORMAT_I8,
        GGWAVE_OPERATING_MODE_TX
    });
    printf("message size: %d", messageSize);
    ggWave->init(messageSize, reinterpret_cast<const char *>(message), GGWAVE_PROTOCOL_AUDIBLE_NORMAL, 30);
    unsigned int length = ggWave->encode();
    if (length > 0){
        jbyteArray ret = env->NewByteArray((jsize)length);
        env->SetByteArrayRegion(ret, 0, (jsize)length, (jbyte*)ggWave->txWaveform());
        return ret;
    }
//    int length = ggwave_encode(ggWave, message, messageSize, GGWAVE_PROTOCOL_ULTRASOUND_FASTEST, 100, msgBuffer, 0);
//    return ret;
    std::string s = "No data encoded - encode() returned " + std::to_string(length);
    env->ThrowNew(env->FindClass("com/example/soundauth/SoundProcessException"), s.c_str());
    return nullptr;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_soundauth_ListenService_decode(JNIEnv * env, jobject thiz, jbyteArray audio, jint audioSize) {
    memset(msgBuffer, 0, msg_buffer_size);
    bool valid = ggWave->decode(audio, audioSize);
    if (!valid) {
        // throw error
        return nullptr;
    }

    jbyteArray ret = env->NewByteArray(ggWave->rxDataLength());
    env->SetByteArrayRegion(ret, 0, ggWave->rxDataLength(), (jbyte*) ggWave->rxData().data());
    return ret;
//    ggWave->init()
//    ggWave->encode()
//    int length = ggwave_decode(ggWave, audio, audioSize, msgBuffer);
//    if (length > 0){
//        jbyteArray ret = env->NewByteArray(length);
//        env->SetByteArrayRegion(ret, 0, length, (jbyte*) msgBuffer);
//        return ret;
//    }
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_soundauth_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ListenService_initGGwave(JNIEnv *env, jobject thiz) {

}