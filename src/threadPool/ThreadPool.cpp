//
// Created by lbw on 25-2-17.
//

#include "threadPool/ThreadPool.h"

ThreadPool::ThreadPool()
{
    threadNum_ = 8;
    ptr_ = 0;
    num_busy_ = 0;
    for (auto i = 0; i < threadNum_; ++i)
    {
        threads_.push_back(nullptr);
    }
}

ThreadPool::~ThreadPool()
{
    join();
}

void ThreadPool::join()
{
    std::cout << ".join() called" << std::endl;
    while (1)
    {
        std::cout << num_busy_.load() << std::endl;
        if (num_busy_ == 0)
        {
            for (auto i = 0; i < threadNum_; ++i)
            {
                auto& thread = threads_[i];
                if (thread != nullptr && thread->joinable())
                    thread->join();
                std::cout << "Released thread " << i << std::endl;
            }
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ThreadPool::push(std::function<void()>&& task)
{
    assign_mtx_.lock();
    while (threads_[ptr_] != nullptr)
    {
        ptr_ = (ptr_ + 1) % threadNum_;
    }
    assign_mtx_.unlock();

    int thread_id = ptr_;
    ++num_busy_;
    auto f = [this,task,thread_id]()
    {
        task();
        //TODO 这里需要以某种方式返回thread_id
        std::thread end(&ThreadPool::release, this, thread_id);
        end.detach();
        --num_busy_;
        std::cout << "Thread " << thread_id << " Free" << std::endl;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    };

    std::unique_ptr<std::thread> thread_ptr(new std::thread(f));
    threads_[thread_id] = std::move(thread_ptr);
}

void ThreadPool::force_push(std::function<void()>&& task)
{
    if (num_busy_ == threadNum_)
    {
        resize_mtx_.lock();
        resize();
        resize_mtx_.unlock();
    }
    push(std::move(task));
}

void ThreadPool::resize()
{
    int thread_num = threadNum_;
    thread_num *= 2;
    if (thread_num > threadNum_)
    {
        for (auto i = 0; i < thread_num - threadNum_; ++i)
        {
            threads_.push_back(nullptr);
        }
    }
    threadNum_ = thread_num;
}

void ThreadPool::release(int thread_id)
{
    threads_[thread_id]->detach();
    threads_[thread_id] = nullptr;
    std::cout << "Thread " << thread_id << " released" << std::endl;
}

void ThreadPool::get()
{
}
