//
// Created by stanko on 10/28/24.
//

#include <jni.h>
#include "ggwave/ggwave.h"
#include <string>
#include <android/log.h>

ggwave_Instance ggWave;

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_soundauth_MessageSender_encode(JNIEnv * env, jobject thiz, jbyteArray message) {
    int message_size = env->GetArrayLength(message);
    char buff[message_size];
    env->GetByteArrayRegion(message, 0, message_size, (jbyte*)buff);
    int waveform_length = ggwave_encode(ggWave, buff, message_size, GGWAVE_PROTOCOL_ULTRASOUND_FASTEST, 50, NULL, 1);

    if (waveform_length > 0){
        char waveform[waveform_length];
        ggwave_encode(ggWave, buff, message_size, GGWAVE_PROTOCOL_ULTRASOUND_FASTEST, 50, waveform, 0);

        jbyteArray ret = env->NewByteArray((jsize)waveform_length);
        env->SetByteArrayRegion(ret, 0, (jsize)waveform_length, (jbyte*)waveform);
        return ret;
    }

    std::string s = "No data encoded - encode() returned " + std::to_string(waveform_length);
    env->ThrowNew(env->FindClass("com/example/soundauth/SoundProcessException"), s.c_str());
    return nullptr;
}

extern "C"
JNIEXPORT jbyteArray JNICALL
Java_com_example_soundauth_MessageReceiver_decode(JNIEnv * env, jobject thiz, jshortArray audioData) {
    jsize dataSize = env->GetArrayLength(audioData);
    jboolean isCopy = false;
    jshort* data = env->GetShortArrayElements(audioData, &isCopy);

    char output[1000];
    int length = ggwave_decode(ggWave, (char*) data, 2*dataSize, output);

    // no data detected
    if (length == 0) {
        return nullptr;
    }

    if (length == -1) {
        env->ThrowNew(env->FindClass("com/example/soundauth/SoundProcessException"), "Error while decoding message");
    }

    __android_log_print(ANDROID_LOG_DEBUG, "GGWAVE", "received message %s", output);
    jbyteArray ret = env->NewByteArray(length);
    env->SetByteArrayRegion(ret, 0, length, reinterpret_cast<const jbyte *>(output));

    return ret;
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
JNIEXPORT jboolean JNICALL
Java_com_example_soundauth_ListenService_initGGwave(JNIEnv *env, jobject thiz, jfloat sample_rate,jint buffer_size) {
    ggwave_Parameters parameters = ggwave_getDefaultParameters();
    parameters.sampleFormatInp = GGWAVE_SAMPLE_FORMAT_I16;
    parameters.sampleFormatOut = GGWAVE_SAMPLE_FORMAT_I16;
    parameters.sampleRateInp = sample_rate;
    ggWave = ggwave_init(parameters);

    __android_log_print(ANDROID_LOG_DEBUG, "GGWAVE", "Successfully initialized");
    return true;
}