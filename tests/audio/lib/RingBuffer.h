//
// Created by stanko on 2/17/26.
//

#ifndef SOUND_AUTH_RINGBUFFER_H
#define SOUND_AUTH_RINGBUFFER_H
#include <stdexcept>
#include <mutex>
#include <vector>

template <class T>
class Ringbuffer {
public:
    explicit Ringbuffer(unsigned int size = 1);
    ~Ringbuffer();
    void resize(unsigned int newsize);
    T get(int index);
    bool pop(T& val);
    bool add(const T & val);
    [[nodiscard]] ssize_t size() const;
    bool full();
    bool empty();
    std::vector<T> snapshot();
private:
    int n_elements;
    std::vector<T> memory;
    unsigned int start;
    int end;
    std::mutex lock;
};

template <class T>
Ringbuffer<T>::Ringbuffer(const unsigned int size) :
        n_elements(0),
        start(0),
        end(-1)
{
    resize(size);
}

template <class T>
Ringbuffer<T>::~Ringbuffer() = default;

template <class T>
void Ringbuffer<T>::resize(const unsigned int newsize)
{
    const std::lock_guard<std::mutex> g(lock);
    // memset(memory.data(), 0, size() * sizeof(T));
    memory.resize(newsize);
}

template <class T>
bool Ringbuffer<T>::pop(T& val) {
    const std::lock_guard<std::mutex> g(lock);
    if (empty()) {
        return false;
    }
    n_elements--;
    val =  memory[start++ % memory.capacity()];
    return true;
}


template <class T>
T Ringbuffer<T>::get(int index) {
    if (empty()) {
        return 0;
    }
    const std::lock_guard<std::mutex> g(lock);
    return memory[(start + index) % memory.capacity()];
}

template <class T>
bool Ringbuffer<T>::add(const T & value) {
    const std::lock_guard<std::mutex> g(lock);
    memory[++end % memory.capacity()] = value;
    if(full()) {
        ++start;
        return false;
    } else {
        n_elements++;
        return true;
    }
}

template <class T>
ssize_t Ringbuffer<T>::size() const{
    return n_elements;
}

template <class T>
bool Ringbuffer<T>::full() {
    return size() == memory.capacity();
}

template <class T>
bool Ringbuffer<T>::empty() {
    return size() == 0;
}

// Add this to your Ringbuffer class
template <class T>
std::vector<T> Ringbuffer<T>::snapshot() {
    const std::lock_guard<std::mutex> g(lock);
    std::vector<T> out;
    out.reserve(n_elements);
    for (int i = 0; i < n_elements; i++) {
        // Use the same logic as get() but under a single lock
        out.push_back(memory[(start + i) % memory.size()]);
    }
    return out;
}

#endif //SOUND_AUTH_RINGBUFFER_H
