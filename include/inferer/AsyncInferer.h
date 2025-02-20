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

class AsyncInferer
{

public:
    explicit AsyncInferer();
    ~AsyncInferer();

    bool setInfer(std::unique_ptr<InferBase> infer);
    void registerPostprocess(std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> callback);
    void registerCallback(std::function<void(void*)> callback);

    void pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& input_bindings);

    void set_getResult_timer(int time);
private:
    void getNetStructure();
    //网络信息
    std::vector<det::Binding> input_bindings_;
    std::vector<det::Binding> output_bindings_;
    //推理器管理
    std::shared_ptr<InferBase> infer_;
    std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> post_function_;
    //线程管理
    ThreadPool thread_pool_;
    //结果管理
    int result_delay_ = 100;
    bool result_start_ = true;
    std::function<void(void*)> callback_;
    void result_loop();
};


inline void AsyncInferer::set_getResult_timer(int time)
{
    result_delay_ = time;
}

inline void AsyncInferer::registerCallback(std::function<void(void*)> callback)
{
    callback_ = std::move(callback);
}

inline void AsyncInferer::result_loop()
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
#endif //ASYNCINFERER_H
