#include <android/log.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <jni.h>

static int pfd[2];
static pthread_t thr;
static const char* kLogTag = "NativeStdout";

static JavaVM* g_vm = nullptr;
static jobject g_logger_obj = nullptr;
static jmethodID g_log_method = nullptr;

// Synchronization primitives
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_init_cond = PTHREAD_COND_INITIALIZER;
static bool g_initialized = false;

extern "C" JNIEXPORT void JNICALL
Java_com_example_soundauth_ui_Logger_initNativeLogging(JNIEnv* env, jobject thiz) {
    pthread_mutex_lock(&g_init_mutex);

    env->GetJavaVM(&g_vm);
    g_logger_obj = env->NewGlobalRef(thiz);
    jclass clazz = env->GetObjectClass(g_logger_obj);
    g_log_method = env->GetMethodID(clazz, "log", "(Ljava/lang/String;)V");

    g_initialized = true;

    // Signal the thread that it's safe to proceed
    pthread_cond_signal(&g_init_cond);
    pthread_mutex_unlock(&g_init_mutex);
}

static void* log_thread(void*) {
    // --- STEP 1: Wait for initialization ---
    pthread_mutex_lock(&g_init_mutex);
    while (!g_initialized) {
        pthread_cond_wait(&g_init_cond, &g_init_mutex);
    }
    pthread_mutex_unlock(&g_init_mutex);

    // Now g_vm is guaranteed to be valid
    JNIEnv* env;
    if (g_vm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
        return nullptr;
    }

    ssize_t read_size;
    char buffer[256];
    char line[1024];
    size_t line_pos = 0;

    while ((read_size = read(pfd[0], buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < read_size; ++i) {
            if (buffer[i] == '\n' || line_pos >= sizeof(line) - 1) {
                line[line_pos] = '\0';

                for (int j = 0; j < line_pos; j++) {
                    auto c = (unsigned char) line[j];

                    if (!isprint(c) && c != '\t' && c != '\r') {
                        line[j] = '?';
                    }
                }

                jstring jmsg = env->NewStringUTF(line);
                env->CallVoidMethod(g_logger_obj, g_log_method, jmsg);
                env->DeleteLocalRef(jmsg);

                __android_log_write(ANDROID_LOG_INFO, kLogTag, line);
                line_pos = 0;
            } else {
                line[line_pos++] = buffer[i];
            }
        }
    }

    g_vm->DetachCurrentThread();
    return nullptr;
}

void redirectStdoutToLogcat() {
    pipe(pfd);
    dup2(pfd[1], STDOUT_FILENO);
    dup2(pfd[1], STDERR_FILENO);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    pthread_create(&thr, nullptr, log_thread, nullptr);
}