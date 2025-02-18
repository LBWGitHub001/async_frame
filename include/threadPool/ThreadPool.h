//
// Created by lbw on 25-2-17.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <utility>
#include <vector>

#include "inferer/preset/InferBase.h"

class ThreadPool
{

public:
    explicit ThreadPool();
    ~ThreadPool();

    void join();

    void push(std::function<void()>&& task);
    void free_push(std::function<void()>&& task);
    void force_push(std::function<void()>&& task);
    void get();

private:
    void resize();
    std::mutex resize_mtx_;

    std::vector<std::unique_ptr<std::thread>> threads_;
    std::vector<std::unique_ptr<InferBase>> infers_;

    std::mutex assign_mtx_;
    int threadNum_;
    int ptr_;
    std::atomic<int> num_busy_;

    void release(int thread_id);
};


#endif //THREADPOOL_H
