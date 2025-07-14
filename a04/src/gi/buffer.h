#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <omp.h>

/**
 * @brief A one to three dimensional templated buffer with overloaded access operators
 * @note Basically a std::vector with fancy access operators.
 *
 * @tparam T Template type of buffered object
 */
template <typename T> class Buffer {
public:
    // construct from buffer dimensions
    Buffer(size_t w, size_t h = 1, size_t d = 1) : w(w), h(h), d(d), mem(w * h * d) {}
    // copy constructor
    Buffer(const Buffer<T>& o) {
        *this = o; // delegate to copy assignment operator
    }
    virtual ~Buffer() {}

    // resize the buffer (alloc only if new size is greater and discards old data)
    void resize(size_t w, size_t h = 1, size_t d = 1) {
        this->w = w;
        this->h = h;
        this->d = d;
        mem.resize(w * h * d);
    }

    // 1D access operators using []
    T &operator[](size_t id) { return mem[id]; }
    const T &operator[](size_t id) const { return mem[id]; }

    // 3D access operators using ()
    T &operator()(size_t x, size_t y = 0, size_t z = 0) {
        return mem[z * w * h + y * w + x];
    }
    const T &operator()(size_t x, size_t y = 0, size_t z = 0) const {
        return mem[z * w * h + y * w + x];
    }

    // copy assignment operator
    Buffer<T> &operator=(const Buffer<T>& o) {
        w = o.w; h = o.h; d = o.d;
        mem.reserve(w * h * d);
        for (int z = 0; z < int(d); ++z)
            #pragma omp parallel for
            for (int y = 0; y < int(h); ++y)
                for (int x = 0; x < int(w); ++x)
                    mem[z * w * h + y * w + x] = o[z * w * h + y * w + x];
        return *this;
    }

    // set all entries in this buffer to val
    Buffer<T> &operator=(const T& val) {
        for (int z = 0; z < int(d); ++z)
            #pragma omp parallel for
            for (int y = 0; y < int(h); ++y)
                for (int x = 0; x < int(w); ++x)
                    mem[z * w * h + y * w + x] = val;
        return *this;
    }

    // "accessors"
    inline size_t width() const { return w; }
    inline size_t height() const { return h; }
    inline size_t depth() const { return d; }
    inline T* data() { return mem.data(); }
    inline const T* data() const { return mem.data(); }
    inline size_t nbytes() const { return w * h * d * sizeof(T); }

    // data
    size_t w, h, d;
    std::vector<T> mem;
};
