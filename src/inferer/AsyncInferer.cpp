//
// Created by lbw on 25-2-15.
//

#include <utility>

#include "inferer/AsyncInferer.h"

#include <thread>
#include <chrono>

AsyncInferer::AsyncInferer()
{

}

AsyncInferer::~AsyncInferer()
{

}

bool AsyncInferer::setInfer(std::unique_ptr<InferBase> infer)
{
    infer_ = std::move(infer);
    getNetStructure();
}

void AsyncInferer::pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& get_input, long timestamp)
{
    thread_pool::TaskThread task;
    task.timestamp = timestamp;

    auto ff = [&](int pool_id,int thread_id)
    {
        void* input = nullptr;
        get_input(&input,input_bindings_);
        ThreadPool::try_to_malloc_static<TrtInfer>(pool_id,thread_id);
        return input;
    };

    thread_pool_.push(std::move(ff));
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
