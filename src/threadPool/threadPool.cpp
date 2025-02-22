//
// Created by lbw on 25-2-17.
//

#include "threadPool/threadPool.h"

#include <future>
#include <utility>
int ThreadPool::pools_count_ = 0;
std::vector<ThreadPool*> ThreadPool::pools_ptr_;

ThreadPool::ThreadPool()
{
    pools_count_++;
    pool_id_ = pools_count_-1;
    pools_ptr_.push_back(this);
    threadNum_ = 8;
    ptr_ = 0;
    num_busy_ = 0;
    for (auto i = 0; i < threadNum_; ++i)
    {
        task_threads_.push_back(nullptr);
        static_memory_vector_.push_back(nullptr);
    }
}

ThreadPool::~ThreadPool()
{
    join();
    clearStaticMem();
    pools_ptr_[pool_id_] = nullptr;
}

void ThreadPool::join()
{
    std::cout << ".join() called" << std::endl;
    while (true)
    {
        std::cout << num_busy_.load() << std::endl;
        if (num_busy_ == 0)
        {
            for (auto i = 0; i < threadNum_; ++i)
            {
                auto& task_thread = task_threads_[i];
                if (task_thread != nullptr && task_thread->future->valid())
                {
                    void* result = task_thread->future->get();
                    //free(task_thread->result_ptr);
                }
                std::cout << "Released thread " << i << std::endl;
            }
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool ThreadPool::clearStaticMem()
{
    for (auto i = 0; i < threadNum_; ++i)
    {
        if (static_memory_vector_[i] != nullptr)
        {
            if (clearStaticMem_func_ == nullptr)
                throw std::runtime_error("ThreadPool::clearStaticMem() called,The static release method is not set, and it cannot be released automatically, resulting in memory leaks.");
            clearStaticMem_func_(static_memory_vector_[i]);
        }

    }
    return true;
}

void ThreadPool::setClear(std::function<void(void*)> func)
{
    clearStaticMem_func_ = std::move(func);
}

void ThreadPool::free_push(std::function<void*(int pool_id,int thread_id)>&& task)
{
}

void ThreadPool::push(std::function<void*(int pool_id,int thread_id)>&& task,void* tag)
{
    //分配线程和时间戳
    time_t timestamp = NULL;
    assign_mtx_.lock();
    while (task_threads_[ptr_] != nullptr)
    {
        ptr_ = (ptr_ + 1) % threadNum_;
    }
    auto now = std::chrono::system_clock::now();
    timestamp = std::chrono::system_clock::to_time_t(now);
    assign_mtx_.unlock();
    int thread_id = ptr_;
    ++num_busy_;

    //创建任务
    auto fut = std::async(std::launch::async,[this,task,thread_id]()
    {
        void* result = task(pool_id_,thread_id);
        std::thread end(&ThreadPool::release, this, thread_id);
        end.detach();
        --num_busy_;
        std::cout << "Thread " << thread_id << " Free" << std::endl;
        // std::this_thread::sleep_for(std::chrono::microseconds(100));
        return result;
    });

    //存入线程池
    id_seq_.push(thread_id); //存储当前线程
    std::unique_ptr<thread_pool::TaskThread> task_thread = std::make_unique<thread_pool::TaskThread>();
    task_thread->future = std::make_unique<std::future<void*>>(std::move(fut));
    task_thread->timestamp = timestamp;
    task_thread->tag = tag;
    task_threads_[thread_id] = std::move(task_thread);
}

void ThreadPool::force_push(std::function<void*(int pool_id,int thread_id)>&& task,void* tag)
{
    if (num_busy_ == threadNum_)
    {
        resize_mtx_.lock();
        resize();
        resize_mtx_.unlock();
    }
    push(std::move(task),tag);
}

bool ThreadPool::fast_get(void** output,void** tag)
{
    if (results_.empty())
        return false;
    thread_pool::Result result = results_.front();
    results_.pop();
    if (output!=nullptr)
    {
        *output = result.result_ptr;
    }
    if (tag!=nullptr)
    {
        *tag = result.tag;
    }
    return true;
}

bool ThreadPool::image_get(cv::Mat& image)
{
    if (results_.empty())
        return false;
    thread_pool::Result result = results_.front();
    results_.pop();
    image = ((cv::Mat*)result.result_ptr)->clone();
    return true;
}

void ThreadPool::resize()
{
    int thread_num = threadNum_;
    thread_num *= 2;
    if (thread_num > threadNum_)
    {
        for (auto i = 0; i < thread_num - threadNum_; ++i)
        {
            task_threads_.push_back(nullptr);
            static_memory_vector_.push_back(nullptr);
        }
    }
    threadNum_ = thread_num;
}

void ThreadPool::release(int thread_id)
{
    int try_count = 0;
    int try_freq = 5;
    while (true)
    {
        int thread_id_first = id_seq_.front();
        if (thread_id == thread_id_first) //就是当前线程取答案，快速去除结果放到结果队列中
        {
            //结果转存到结果队列
            auto fut = task_threads_[thread_id]->future->share();
            void* result_ptr = fut.get();
            void* tag = task_threads_[thread_id]->tag;
            time_t timestamp = task_threads_[thread_id]->timestamp;
            thread_pool::Result result(result_ptr,tag,timestamp);
            results_.push(result);
            //空出线程，其他线程也
            task_threads_[thread_id] = nullptr;
            id_seq_.pop();
            // std::cout << "Thread " << thread_id << " released" << std::endl;
            break;
        }
        if (ForceClose_)
        {
            if (try_count>=try_freq*2)//等待时间过长
        {
            release_mtx_.lock();//防止同样的内容被执行两边
            if (id_seq_.front() == thread_id_first)
                id_seq_.pop();
            else
                continue;
            release_mtx_.unlock();
            //强制将该线程从线程池中移除，这可能导致资源的错误泄漏
            //TODO 检查这里的资源泄漏问题
            //delete task_threads_[thread_id_first];
            auto fut = std::move(task_threads_[thread_id_first]->future);
            task_threads_[thread_id_first] = nullptr;
            auto stop = fut->get();
            delete stop;
            std::cout << "************************Thread " << thread_id << " released Forcely********************" << std::endl;
            try_count=0;
            continue;
        }
        if (thread_id_first != id_seq_.front())//检测到队列更新，重置等待信息
        {
            try_count = 0;
        }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MIN_PUSH_DELAY_ms/try_freq));
        try_count++;
    }


}

