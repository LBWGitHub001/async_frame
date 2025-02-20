//
// Created by lbw on 25-2-15.
//

#ifndef ASYNCINFERER_H
#define ASYNCINFERER_H
#include "inferer/preset/VinoInfer.h"
#ifdef TRT
#include "inferer/preset/TrtInfer.h"
#endif

#include <future>
#include <utility>
#include <condition_variable>
#include <vector>
#include <opencv2/opencv.hpp>
#include <threadPool/threadPool.h>

#include "inferer/preset/ppp.h"

#define MAX_QUEUE_LEN 5

typedef struct
{
    std::future<bool> future;
    long timestamp;
} Task;

template<class _Infer>
class AsyncInferer
{
public:
    explicit AsyncInferer()
    {
        result_start_ = true;
        std::thread th(&AsyncInferer::result_loop, this);
        th.detach();

    }
    ~AsyncInferer()
    {
        thread_pool_.join();
        result_start_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(result_delay_*5));
    }

    bool setInfer(std::unique_ptr<InferBase> infer)
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
    void registerPostprocess(std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> callback)
    {
        post_function_ = std::move(callback);
        std::cout << "Registering callback function" << std::endl;
    }
    void registerCallback(std::function<void(void*)> callback)
    {
        callback_ = std::move(callback);
    }

    void pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& get_input)
    {
        auto ff = [this,get_input](int pool_id, int thread_id)
        {
            void* input = nullptr;
            get_input(&input, input_bindings_);
            if (ThreadPool::try_to_malloc_static(pool_id, thread_id, infer_->get_size()))
            {
                void* ptr;
                get_staticMem_ptr(pool_id, thread_id, &ptr);
                auto name = infer_->get_name();
                if (name == "VinoInfer")
                {
                    auto* infer = static_cast<VinoInfer*>(ptr);
                    // infer->VinoInfer(infer_->getModelPath(),false,dynamic_cast<VinoInfer*>(infer_.get())->getDevice());
                    infer->setModel(infer_->getModelPath());
                    infer->setDevice(dynamic_cast<VinoInfer*>(infer_.get())->getDevice());
                    infer->init();
                }
                else if (name == "TrtInfer")
                {
                    auto* infer = static_cast<TrtInfer*>(ptr);
                    infer->setModel(infer_->getModelPath());
                    infer->init();
                }
                else
                {
                    throw "Unknown Infer";
                }
            }
            void* ptr;
            get_staticMem_ptr(pool_id, thread_id, &ptr);
            std::vector<void*> output_vec;
            auto name = infer_->get_name();
            if (name == "VinoInfer")
            {
                auto* infer = static_cast<VinoInfer*>(ptr);
                infer->copy_from_data(input);
                infer->infer();
            }
            else if (name == "TrtInfer")
            {
                auto* infer = static_cast<TrtInfer*>(ptr);
                infer->copy_from_data(input);
                infer->infer();
            }
            else
            {
                throw "Unknown Infer";
            }
            // auto output_vec = infer->getResult();
            post_function_(output_vec, output_bindings_);
            return nullptr;
        };

        thread_pool_.push(std::move(ff));
    }
    void set_getResult_timer(int time)
    {
        result_delay_ = time;
    }
private:
    void getNetStructure()
    {
        input_bindings_ = infer_->getInputBinding();
        output_bindings_ = infer_->getOutputBinding();
    }

    //网络信息
    std::vector<det::Binding> input_bindings_;
    std::vector<det::Binding> output_bindings_;
    std::string model_path_;
    std::string device_path;
    //推理器管理
    std::shared_ptr<InferBase> infer_;
    std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> post_function_;
    //线程管理
    ThreadPool thread_pool_;
    //结果管理
    int result_delay_ = 100;
    bool result_start_ = true;
    std::function<void(void*)> callback_;
    void result_loop()
    {
        while (result_start_)
        {
            void* output;
            if (thread_pool_.fast_get(&output))
            {
                callback_(output);
                free(output);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(result_delay_));
            }
        }
    }
};

#endif //ASYNCINFERER_H
