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
#include "threadPool/memBlock.h"
#include "inferer/preset/InferBase.h"


class ThreadPool
{
public:
    explicit ThreadPool();
    ~ThreadPool();

    void join();
    bool clearStaticMem();
    void setClear(std::function<void(void*)> func);

    void push(std::function<void*(int pool_id,int thread_id)>&& task);
    void free_push(std::function<void*(int pool_id,int thread_id)>&& task);
    void force_push(std::function<void*(int pool_id,int thread_id)>&& task);

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

    friend bool get_staticMem_ptr(int pool_id, int thread_id,void** ptr);

    template <typename Memory>
    static bool try_to_malloc_static(int pool_id,int thread_id)
    {
        auto& staticMem = pools_ptr_[pool_id]->static_memory_vector_[thread_id];
        if (staticMem != nullptr)
            return false;
        staticMem = malloc(sizeof(Memory));
        return true;
    }

    template <typename Memory>
    static bool try_to_free_static(int pool_id,int thread_id)
    {
        auto& staticMem = pools_ptr_[pool_id]->static_memory_vector_[thread_id];
        if (staticMem == nullptr)
            return false;
        free(staticMem);
        staticMem = nullptr;
        return true;
    }

private:
    //运行参数
    int MIN_PUSH_DELAY_ms = 100; //输入端的两个输入之间的最短时间（最大频率）
    //辅助量
    void resize();
    std::mutex resize_mtx_;

    //线程管理
    std::vector<std::unique_ptr<thread_pool::TaskThread>> task_threads_;
    std::vector<void*> static_memory_vector_;

    std::mutex assign_mtx_;
    int threadNum_;
    int ptr_;
    std::atomic<int> num_busy_{};

    std::queue<thread_pool::Result> results_;
    std::queue<int> id_seq_;

    //资源自动释放机制
    void release(int thread_id);
    std::mutex release_mtx_;
    std::function<void(void*)> clearStaticMem_func_;

    //id信息
    inline void* get_staticMem_ptr(int thread_id);
    static int pools_count_;
    static std::vector<ThreadPool*> pools_ptr_;
    int pool_id_;
};


inline void* ThreadPool::get_staticMem_ptr(int thread_id)
{
    return static_memory_vector_[thread_id];
}

inline bool get_staticMem_ptr(int pool_id, int thread_id, void** ptr)
{
    ThreadPool* pool_ptr = ThreadPool::pools_ptr_[pool_id];
    if (pool_ptr != nullptr)
    {
        *ptr = pool_ptr->get_staticMem_ptr(thread_id);
        return true;
    }
    return false;
}

#endif //THREADPOOL_H
