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
#include "threadPool/common.h"
#include "inferer/preset/InferBase.h"


class ThreadPool
{
public:
    explicit ThreadPool();
    ~ThreadPool();

    void join();

    void push(std::function<void*()>&& task);
    void free_push(std::function<void*()>&& task);
    void force_push(std::function<void*()>&& task);

    template <typename Type>
    bool get(void* output)
    {
        thread_pool::Result result;
        if (results_.empty())
            return false;
        result = results_.front();
        results_.pop();
        //memcpy(output,result.result_ptr,sizeof(Type));
        auto a = static_cast<Type*>(output);
        *a = *static_cast<Type*>(result.result_ptr);
        delete static_cast<Type*>(result.result_ptr);
        return true;
    }

    bool fast_get(void* output);
    bool image_get(cv::Mat& image);

    template <class checkedClass>
    bool check_inlaw()
    {
        return CheckType<checkedClass>::value == std::true_type::value;
    }

private:
    //运行参数
    int MIN_PUSH_DELAY_ms = 100; //输入端的两个输入之间的最短时间（最大频率）

    void resize();
    std::mutex resize_mtx_;

    std::vector<std::unique_ptr<thread_pool::TaskThread>> task_threads_;
    std::vector<std::unique_ptr<InferBase>> infers_;

    std::mutex assign_mtx_;
    int threadNum_;
    int ptr_;
    std::atomic<int> num_busy_{};

    std::queue<thread_pool::Result> results_;
    std::queue<int> id_seq_;

    void release(int thread_id);
    std::mutex release_mtx_;
};


#endif //THREADPOOL_H
