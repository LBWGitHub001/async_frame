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
    /*!
     * @brief 构造函数，启动所有服务
     */
    explicit ThreadPool();
    /*!
     * @brief 析构函数，阻塞当前线程，直至所有线程都结束，清空所有线程静态内存
     */
    ~ThreadPool();

    /*!
     * @brief 阻塞调用线程，等待所有线程完毕
     */
    void join();

    /*!
     * @brief 清除所有静态内存
     * @return 清楚成功为true,清除失败为false,并抛出runtime_error
     */
    bool clearStaticMem();

    /*!
     * @brief 设置静态内存的释放函数
     * @param func 静态内存释放函数，传入一个void*
     */
    void setClear(std::function<void(void*)> func);

    /*!
     * @brief 设置停止线程的阈值时间
     * @param time_ms 强制停止线程阈值时间
     * @attention 这个时间需要严谨设置
     */
    void setNoResponseThere(int time_ms){MIN_PUSH_DELAY_ms = time_ms;ForcecLoseTurnON();}

    /*!
     * @brief 开启线程监管,强制释放疑似未响应线程
     * @attention 不稳定，应该搭配setNoResponseThere使用
     */
    void ForcecLoseTurnON(){ForceClose_ = true;}

    /*!
     * @brief 创建一个任务，并将其推入池,如果池中没有空闲线程，将会阻塞
     * @param task 任务，接受两个参数pool_id,thread_id,用来访问静态内存，并且作为线程的唯一标识符
     */
    void push(std::function<void*(int pool_id,int thread_id)>&& task,void* tag = nullptr);

    /*!
     * 无锁推入,需要保证调用环境线程安全
     * @param task 任务，参数同上
     */
    void free_push(std::function<void*(int pool_id,int thread_id)>&& task);

    /*!
     * @brief 强制推入，如果没有空闲线程就会创建一个空闲线程，强行开始任务，不阻塞
     * @param task 任务，参数如上
     */
    void force_push(std::function<void*(int pool_id,int thread_id)>&& task,void* tag = nullptr);

    /*!
     * @brief 获取线程运算结果,遵守FIFO顺序,不阻塞,深拷贝值
     * @tparam Type 结果的类型
     * @param output 传入一个结果的指针
     * @return 返回一个布尔值,当获取成功时为true,获取失败时,为false,不阻塞
     */
    template <typename Type,class tagType = nullptr_t>
    bool get(Type** output,void** tag = nullptr)
    {
        if (results_.empty())
            return false;
        thread_pool::Result result = results_.front();
        results_.pop();
        if (result.result_ptr!=nullptr)
        {
            *output = new Type;
            memcpy(*output,result.result_ptr,sizeof(Type));
            delete static_cast<Type*>(result.result_ptr);
        }
        else
        {
            *output = nullptr;
        }
        if (tag != nullptr)
        {
            *tag = new tagType;
            memcpy(*tag,result.tag,sizeof(tagType));
            delete static_cast<tagType*>(result.tag);
        }
        else
        {
            *tag = nullptr;
        }
        return true;
    }

    /*!
     * @brief 快速获取值,不阻塞,不拷贝,需要手动释放资源
     * @param output 同上
     * @return 同上
     */
    bool fast_get(void** output,void** tag = nullptr);

    /*!
     * 如果运算结果是cv::Mat,则需要调用这个方法获取结果,否则将会造成内存泄漏的风险
     * @param image 同上
     * @return 同上
     */
    bool image_get(cv::Mat& image);

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
    friend bool get_staticMem_ptr(int pool_id, int thread_id,void** ptr);

    /*!
     * @brief 在线程静态内存空间中申请一块空间
     * @tparam _Infer 分配对象类型
     * @param pool_id 池id
     * @param thread_id 线程id
     * @param master 使用拷贝构造拷贝其中的信息
     * @return 布尔值,如果完成分配,返回true,否则,说明此空间上存在内容,先释放才能再次分配
     */
    template <class _Infer>
    static bool try_to_malloc_static(int pool_id,int thread_id,_Infer* master)
    {
        auto& staticMem = pools_ptr_[pool_id]->static_memory_vector_[thread_id];
        if (staticMem != nullptr)
            return false;
        staticMem = new _Infer(*master);
        return true;
    }

    /*!
     * @brief 释放静态内存空间
     * @param pool_id 池id
     * @param thread_id 线程id
     * @return 布尔值,释放成功为true,否则,说明此处原来没有没分配,返回false
     */
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
    bool ForceClose_ = false;

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
