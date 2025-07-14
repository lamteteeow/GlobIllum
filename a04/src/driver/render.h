#pragma once
#include <queue>
#include <mutex>

#include "context.h"

// ---------------------------------------------------------------------------------
// scheduling helper structures

struct Block {
    Block(size_t id, float convergence = 0) : id(id), conv(convergence) {}

    size_t id;
    float conv;
};
inline bool operator<(const Block& A, const Block& B) { return A.conv < B.conv; }

struct MutexPrioQueue {
    inline void push(size_t id, float conv = 0) {
        std::lock_guard<std::mutex> lock(mutex);
        queue.emplace(id, conv);
    }
    inline bool pop(size_t& id) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty()) return false;
        id = queue.top().id;
        queue.pop();
        return true;
    }

    std::mutex mutex;
    std::priority_queue<Block> queue;
};

// ---------------------------------------------------------------------------------
// actual main rendering call

void render(Context& ctx);
