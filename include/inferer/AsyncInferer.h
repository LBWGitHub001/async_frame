//
// Created by lbw on 25-2-15.
//

#ifndef ASYNCINFERER_H
#define ASYNCINFERER_H
#define DEFAULT_INFER VinoInfer
#include "inferer/preset/VinoInfer.h"
#ifdef TRT
#undef DEFAULT_INFER
#define DEFAULT_INFER TrtInfer
#include "inferer/preset/TrtInfer.h"
#endif
#define AUTO_INFER DEFAULT_INFER
#include <future>
#include <utility>
#include <condition_variable>
#include <vector>
#include <opencv2/opencv.hpp>
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
    void pushInput(const std::function<void(void**)>& get_input, long timestamp);
    void pushInput(const std::function<void(void**, std::vector<det::Binding>&)>& input_bindings, long timestamp);

private:
    void getNetStructure();
    //网络信息
    std::vector<det::Binding> input_bindings_;
    std::vector<det::Binding> output_bindings_;
    //推理器管理
    std::unique_ptr<InferBase> infer_;
    std::function<void(void*, std::vector<det::Binding>&, long timestamp)> callback_function_;
    //线程管理
    void release_future();
    bool start_release_;
    std::thread thread_release_function_;
    std::queue<Task> future_queue_;
    std::mutex future_queue_mtx_;
    int max_queue = MAX_QUEUE_LEN;
    long latest_timestamp_;
    std::mutex latest_timestamp_mtx_;

    //Test
    int max_queue_len_=0;
};


#endif //ASYNCINFERER_H
