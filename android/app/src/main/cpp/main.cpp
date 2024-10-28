//
// Created by stanko on 10/28/24.
//

#include <jni.h>
#include "ggwave/ggwave.h"
#include <string>

GGWave* ggWave;

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_example_soundauth_ListenService_initGGWave(JNIEnv *env, jobject thiz, jint sample_rate, jint buffer_size) {
    ggWave = new GGWave();
    return ggWave != nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_soundauth_ListenService_encode(JNIEnv * env, jbyteArray message) {
    ggWave->encode();
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