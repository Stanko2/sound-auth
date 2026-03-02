#ifdef __ANDROID__

#include <android/log.h>
#include <unistd.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>

static int pfd[2];
static pthread_t thr;
static const char* kLogTag = "NativeStdout";

static void* log_thread(void*) {
    ssize_t read_size;
    char buffer[256];
    char line[1024];
    size_t line_pos = 0;

    while ((read_size = read(pfd[0], buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < read_size; ++i) {
            if (buffer[i] == '\n' || line_pos >= sizeof(line) - 1) {
                line[line_pos] = '\0';
                __android_log_write(ANDROID_LOG_INFO, kLogTag, line);
                line_pos = 0;
            } else {
                line[line_pos++] = buffer[i];
            }
        }
    }

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

#endif
