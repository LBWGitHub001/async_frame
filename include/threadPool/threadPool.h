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

template <class _Result, class _Tag = nullptr_t>
class ThreadPool
{
    using TaskThread = thread_pool::TaskThread<_Result, _Tag>;
    using Result = thread_pool::Result<_Result, _Tag>;

public:
    /*!
     * @brief 构造函数，启动所有服务
     */
    explicit ThreadPool(int threadNum)
    {
        pools_count_++;
        pool_id_ = pools_count_ - 1;
        pools_ptr_.push_back(this);
        threadNum_ = threadNum;
        ptr_ = 0;
        num_busy_ = 0;
        for (auto i = 0; i < threadNum_; ++i)
        {
            task_threads_.push_back(nullptr);
            static_memory_vector_.push_back(nullptr);
        }
    }

    /*!
     * @brief 析构函数，阻塞当前线程，直至所有线程都结束，清空所有线程静态内存
     */
    ~ThreadPool()
    {
        join();
        clearStaticMem();
        pools_ptr_[pool_id_] = nullptr;
    }

    /*!
     * @brief 阻塞调用线程，等待所有线程完毕
     */
    void join()
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
                        _Result result = task_thread->future->get();
                        //free(task_thread->result_ptr);
                    }
                    std::cout << "Released thread " << i << std::endl;
                }
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /*!
     * @brief 清除所有静态内存
     * @return 清楚成功为true,清除失败为false,并抛出runtime_error
     */
    bool clearStaticMem()
    {
        for (auto i = 0; i < threadNum_; ++i)
        {
            if (static_memory_vector_[i] != nullptr)
            {
                if (clearStaticMem_func_ == nullptr)
                    throw std::runtime_error(
                        "ThreadPool::clearStaticMem() called,The static release method is not set, and it cannot be released automatically, resulting in memory leaks.");
                clearStaticMem_func_(static_memory_vector_[i]);
            }
        }
        return true;
    }

    /*!
     * @brief 设置静态内存的释放函数
     * @param func 静态内存释放函数，传入一个void*
     */
    void setClear(std::function<void(void*)> func)
    {
        clearStaticMem_func_ = std::move(func);
    }

    /*!
     * @brief 设置停止线程的阈值时间
     * @param time_ms 强制停止线程阈值时间
     * @attention 这个时间需要严谨设置
     */
    void setNoResponseThere(int time_ms)
    {
        MIN_PUSH_DELAY_ms = time_ms;
        ForcecLoseTurnON();
    }

    /*!
     * @brief 开启线程监管,强制释放疑似未响应线程
     * @attention 不稳定，应该搭配setNoResponseThere使用
     */
    void ForcecLoseTurnON() { ForceClose_ = true; }

    /*!
     * @brief 创建一个任务,并将其推入池,如果池中没有空闲线程,将会阻塞
     * @param task 任务,接受两个参数pool_id,thread_id,用来访问静态内存，并且作为线程的唯一标识符
     * @param tag 静态传递量,该量在任务运行过程中不会修改,可以在获取答案时获取使用,默认是nullptr
     * @param delay_us 延时，新线程运行一段时间后，将会析构传参时的栈内存,必要时填写该参数保证信息传递完成,默认是0
     */
    void push(std::function<_Result(int pool_id, int thread_id)>&& task, _Tag tag = nullptr, int delay_us = 0)
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
        auto fut = std::async(std::launch::async, [this,task,thread_id]()-> _Result
        {
            _Result result = task(pool_id_, thread_id);
            std::thread end(&ThreadPool::release, this, thread_id);
            end.detach();
            --num_busy_;
            // std::cout << "Thread " << thread_id << " Free" << std::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            return result;
        });

        std::this_thread::sleep_for(std::chrono::microseconds(delay_us));
        //存入线程池
        id_seq_.push(thread_id); //存储当前线程
        std::unique_ptr<TaskThread> task_thread = std::make_unique<TaskThread>();
        task_thread->future = std::make_unique<std::future<_Result>>(std::move(fut));
        task_thread->timestamp = timestamp;
        task_thread->tag = tag;
        task_threads_[thread_id] = std::move(task_thread);
    }

    /*!
     * 无锁推入,需要保证调用环境线程安全
     * @param task 任务，参数同上
     */
    [[deprecated("未完成")]]
    void free_push(std::function<void*(int pool_id, int thread_id)>&& task);

    /*!
     * @brief 强制推入，如果没有空闲线程就会创建一个空闲线程，强行开始任务，不阻塞
     * @param task 任务，参数如上
     */
    void force_push(std::function<void*(int pool_id, int thread_id)>&& task, void* tag = nullptr, int delay_us = 0)
    {
        if (num_busy_ == threadNum_)
        {
            resize_mtx_.lock();
            resize();
            resize_mtx_.unlock();
        }
        push(std::move(task), tag, delay_us);
    }

    /*!
     * @brief 获取线程运算结果,遵守FIFO顺序,不阻塞,深拷贝值
     * @tparam Type 结果的类型
     * @param output 传入一个结果的指针
     * @return 返回一个布尔值,当获取成功时为true,获取失败时,为false,不阻塞
     */

    bool get(_Result& output, _Tag& tag = nullptr)
    {
        if (results_.empty())
            return false;
        thread_pool::Result result = results_.front();
        results_.pop();
        output = result.result;
        tag = result.tag;
        return true;
    }

    /*!
     * @brief 快速获取值,不阻塞,不拷贝,需要手动释放资源
     * @param output 同上
     * @return 同上
     */
    bool fast_get(_Result& output, _Tag& tag)
    {
        if (results_.empty())
            return false;
        thread_pool::Result result = results_.front();
        results_.pop();
        output = result.result;
        tag = result.tag;
        return true;
    }

    /*!
     * 如果运算结果是cv::Mat,则需要调用这个方法获取结果,否则将会造成内存泄漏的风险
     * @param image 同上
     * @return 同上
     */
    bool image_get(cv::Mat& image)
    {
        if (results_.empty())
            return false;
        thread_pool::Result result = results_.front();
        results_.pop();
        image = result.result.clone();
        return true;
    }

    /*!
     * @brief 检查处理的数据是否支持静态内存的自动复制功能，如不能，需要重写拷贝
     * @tparam checkedClass 待检查的类型
     * @return 布尔值，true or false
     */
    template <class checkedClass>
    bool check_inlaw()
    {
        return CheckType<checkedClass>::value == std::true_type::value;
    }

    /*!
     * @brief 静态方法,获取线程的静态内存空间
     * @param pool_id 池id,一个线程池的唯一标识码
     * @param thread_id 线程id,一个线程的唯一标识码
     * @param ptr 提取的静态内存的指针
     * @return 布尔值,如果已经被分配,返回true,否则返回false
     */
    static bool get_staticMem_ptr(int pool_id, int thread_id, void** ptr)
    {
        ThreadPool* pool_ptr = ThreadPool::pools_ptr_[pool_id];
        if (pool_ptr != nullptr)
        {
            *ptr = pool_ptr->get_staticMem_ptr(thread_id);
            if (*ptr != nullptr)
                return true;
            else
                return false;
        }
        return false;
    }

    /*!
     * @brief 在线程静态内存空间中申请一块空间
     * @tparam _Infer 分配对象类型
     * @param pool_id 池id
     * @param thread_id 线程id
     * @param master 使用拷贝构造拷贝其中的信息
     * @return 布尔值,如果完成分配,返回true,否则,说明此空间上存在内容,先释放才能再次分配
     */
    template <class _Infer>
    static bool try_to_malloc_static(int pool_id, int thread_id, _Infer* master)
    {
        auto& staticMem = pools_ptr_[pool_id]->static_memory_vector_[thread_id];
        if (staticMem != nullptr)
            return false;
        staticMem = new _Infer(*master);
        return true;
    }

    /*!
       * @brief 向静态内存中放入一个指针
       * @param pool_id 池id
       * @param thread_id 线程id
       * @param ptr 指向一个空间的指针
       * @return 布尔值
       */
    static bool try_to_alloc_static(int pool_id, int thread_id, void* ptr)
    {
        auto& staticMem = pools_ptr_[pool_id]->static_memory_vector_[thread_id];
        if (staticMem == nullptr)
        {
            staticMem = ptr;
            return true;
        }
        return false;
    }

    /*!
     * @brief 释放静态内存空间
     * @param pool_id 池id
     * @param thread_id 线程id
     * @return 布尔值,释放成功为true,否则,说明此处原来没有没分配,返回false
     */
    static bool try_to_free_static(int pool_id, int thread_id)
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
    void resize()
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

    std::mutex resize_mtx_;

    //线程管理
    std::vector<std::unique_ptr<TaskThread>> task_threads_;
    std::vector<void*> static_memory_vector_;

    std::mutex assign_mtx_;
    int threadNum_;
    int ptr_;
    std::atomic<int> num_busy_{};

    std::queue<Result> results_;
    std::queue<int> id_seq_;

    //资源自动释放机制
    void release(int thread_id)
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
                _Result result_raw = fut.get();
                _Tag tag = task_threads_[thread_id]->tag;
                time_t timestamp = task_threads_[thread_id]->timestamp;
                Result result(timestamp, result_raw, tag);
                results_.push(result);
                //空出线程，其他线程也
                task_threads_[thread_id] = nullptr;
                id_seq_.pop();
                // std::cout << "Thread " << thread_id << " released" << std::endl;
                break;
            }
            if (ForceClose_)
            {
                if (try_count >= try_freq * 2) //等待时间过长
                {
                    release_mtx_.lock(); //防止同样的内容被执行两边
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
                    std::cout << "************************Thread " << thread_id <<
                        " released Forcely********************" << std::endl;
                    try_count = 0;
                    continue;
                }
                if (thread_id_first != id_seq_.front()) //检测到队列更新，重置等待信息
                {
                    try_count = 0;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(MIN_PUSH_DELAY_ms / try_freq));
            try_count++;
        }
    }


    std::mutex release_mtx_;
    std::function<void(void*)> clearStaticMem_func_;
    bool ForceClose_ = false;

    //id信息
    inline void* get_staticMem_ptr(int thread_id)
    {
        return static_memory_vector_[thread_id];
    }

    int pool_id_;

public:
    static int pools_count_;
    static std::vector<ThreadPool*> pools_ptr_;
};

template <class _Result, class _Tag>
int ThreadPool<_Result, _Tag>::pools_count_ = 0;

template <class _Result, class _Tag>
std::vector<ThreadPool<_Result, _Tag>*> ThreadPool<_Result, _Tag>::pools_ptr_ = std::vector<ThreadPool<_Result, _Tag>*>();

#endif //THREADPOOL_H
