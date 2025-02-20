//
// Created by lbw on 25-2-15.
//

#include <utility>

#include "inferer/AsyncInferer.h"

#include <thread>
#include <chrono>

AsyncInferer::AsyncInferer()
{
    result_start_ = true;
    std::thread th(&AsyncInferer::result_loop, this);
    th.detach();

}

AsyncInferer::~AsyncInferer()
{
    thread_pool_.join();
    result_start_ = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(result_delay_*5));
}

bool AsyncInferer::setInfer(std::unique_ptr<InferBase> infer)
{
    try
    {
        infer_ = std::move(infer);
    getNetStructure();
    }
    catch (...)
    {
        return false;
    }
    return true;
}

void AsyncInferer::pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& get_input)
{
    auto ff = [&](int pool_id, int thread_id)
    {
        void* input = nullptr;
        get_input(&input, input_bindings_);
        ThreadPool::try_to_malloc_static(pool_id, thread_id, infer_->get_size());
        void* ptr;
        get_staticMem_ptr(pool_id, thread_id, &ptr);
        InferBase* infer = nullptr;
        auto name = infer_->get_name();
        if (name == "VinoInfer")
        {
            infer = static_cast<VinoInfer*>(ptr);
        }
        else if (name == "TrtInfer")
        {
            infer = static_cast<TrtInfer*>(ptr);
        }
        else
        {
            throw "Unknown Infer";
        }
        infer->copy_from_data(input);
        infer->infer();
        auto output_vec = infer->getResult();
        post_function_(output_vec, output_bindings_);
        return nullptr;
    };

    thread_pool_.push(std::move(ff));
}

void AsyncInferer::registerPostprocess(
    std::function<void*(std::vector<void*>&, std::vector<det::Binding>& output_bindings)> callback)
{
    post_function_ = std::move(callback);
    std::cout << "Registering callback function" << std::endl;
}

inline void AsyncInferer::getNetStructure()
{
    input_bindings_ = infer_->getInputBinding();
    output_bindings_ = infer_->getOutputBinding();
}
