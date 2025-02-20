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
    void registerCallback(
        std::function<void(void*, std::vector<det::Binding>&, long)> callback);
    [[deprecated("You need one more param")]]
    void pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& input_bindings, long timestamp);

private:
    void getNetStructure();
    //网络信息
    std::vector<det::Binding> input_bindings_;
    std::vector<det::Binding> output_bindings_;
    //推理器管理
    std::shared_ptr<InferBase> infer_;
    std::function<void(void*, std::vector<det::Binding>&, long timestamp)> callback_function_;
    //线程管理
    ThreadPool thread_pool_;
    //Test
    int max_queue_len_=0;
};


#endif //ASYNCINFERER_H
