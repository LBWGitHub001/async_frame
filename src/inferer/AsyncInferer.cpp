//
// Created by lbw on 25-2-15.
//

#include <utility>

#include "inferer/AsyncInferer.h"

#include <thread>
#include <chrono>

AsyncInferer::AsyncInferer()
{
    start_release_ = true;
    latest_timestamp_ = -1e8;
    std::thread thread_release_function(&AsyncInferer::release_future, this);
    thread_release_function.detach();
}

AsyncInferer::~AsyncInferer()
{
    start_release_ = false;
    while (!future_queue_.empty())
    {
        future_queue_.front().future.get();
        future_queue_.pop();
    }
}

bool AsyncInferer::setInfer(std::unique_ptr<InferBase> infer)
{
    infer_ = std::move(infer);
    getNetStructure();
}

void AsyncInferer::pushInput(const std::function<void(void**)>& get_input, long timestamp)
{
    Task infer_task;
    infer_task.future = std::async(std::launch::async, [this,&get_input]()
    {
        void* input;
        // std::cout << "preProcess Start" << std::endl;

        get_input(&input);

        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
        // std::cout << "preProcess Done" << timestamp << std::endl;
        void* output;
        infer_->infer_async(input, &output);
        free(input);
        callback_function_(output, output_bindings_, timestamp);
        free(output);
        return true;
    });
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    infer_task.timestamp = timestamp;
    future_queue_.push(std::move(infer_task));
    // std::cout << "preProcess End" << std::endl;
}

void AsyncInferer::pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& get_input, long timestamp)
{
    Task infer_task;
    infer_task.future = std::async(std::launch::async, [this,&get_input,timestamp]()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
        std::cout << "A: " << timestamp << std::endl;
        void* input;
        get_input(&input, input_bindings_);

        void* output;
        infer_->infer_async(input, &output);
        free(input);
        callback_function_(output, output_bindings_, timestamp);
        free(output);
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (timestamp >= latest_timestamp_)
        {
            latest_timestamp_ = timestamp;
            start_release_ = true;
            auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::time_point_cast<std::chrono::microseconds>(now).time_since_epoch().count();
            std::cout << "B: " << timestamp << std::endl;
            return true;
        }
        return false;
    });
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
    infer_task.timestamp = timestamp;
    // future_queue_mtx_.lock();
    while (future_queue_.size() > 10);
    future_queue_.push(std::move(infer_task));
    // future_queue_mtx_.unlock();
    // std::thread th(&AsyncInferer::release_future, this);
    // th.detach();
    // std::cout << "preProcess End" << std::endl;
}

void AsyncInferer::registerCallback(
    std::function<void(void*, std::vector<det::Binding>& output_bindings, long)> callback)
{
    callback_function_ = std::move(callback);
    std::cout << "Registering callback function" << std::endl;
}

inline void AsyncInferer::getNetStructure()
{
    input_bindings_ = infer_->getInputBinding();
    output_bindings_ = infer_->getOutputBinding();
}

void AsyncInferer::release_future()
{
    bool is_success = true;
    while (start_release_)
    {
        if (!future_queue_.empty())
        {
            // future_queue_mtx_.lock();
            std::cout << "NOW: " << future_queue_.size() << std::endl;
            auto& task = future_queue_.front();
            std::string log;
            if (task.future.valid())
                log = task.future.get()?"SUCCESS: ":"FAIL: ";
            std::cout << log << task.timestamp << std::endl;

            future_queue_.pop();
            // latest_timestamp_mtx_.unlock();
            // future_queue_mtx_.unlock();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}
