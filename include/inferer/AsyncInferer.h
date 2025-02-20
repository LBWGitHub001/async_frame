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
    /*!
     * @brief 构造函数，启动所有服务
     */
    explicit AsyncInferer()
    {
        result_start_ = true;
        std::thread th(&AsyncInferer::result_loop, this);
        th.detach();

        thread_pool_.setClear([](void* infer)
        {
            _Infer* infer_ptr = (_Infer*)infer;
            delete infer_ptr;
        });

    }

    /*!
     * @brief 析构函数，调用时会阻塞当前线程，直至所有线程全部结束，然后停止所有服务
     */
    ~AsyncInferer()
    {
        thread_pool_.join();
        result_start_ = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(result_delay_*5));
    }

    /*!
     * 设置推理器
     * @param infer 推理器的unique_ptr
     * @return 返回一个布尔值，当有异常时为false，当正常时为true
     */
    bool setInfer(std::unique_ptr<_Infer> infer)
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

    /*!
     * @brief 注册后处理函数，推理器完成推理后调用
     * @param postprocess 后处理函数，接受两个参数，(原始数据,NN输出层形状)
     */
    void registerPostprocess(std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> postprocess)
    {
        post_function_ = std::move(postprocess);
        std::cout << "Registering callback function" << std::endl;
    }

    /*!
     * @brief 注册回调函数，当取得答案后会调用该方法
     * @param callback 回调函数，参数是postprocess返回的类型
     * @attention 需要确保手动释放，避免内存泄漏
     */
    void registerCallback(std::function<void(void*)> callback)
    {
        callback_ = std::move(callback);
    }

    /*!
     * @brief 释放线程池静态空间方法的覆写
     * @attention 现有预设已有自动方法自动析构，不需要进行覆写该成员，如果使用第三方推理器，可能需要覆写
     * @param FreeStatic 释放线程池静态空间的方法覆写方法
     */
    void registerFreeStatic(std::function<void(void*)> FreeStatic)
    {
        thread_pool_.setClear(FreeStatic);
    }

    /*!
     * @brief 获取输入数据，并创建任务，推入线程池
     * @param get_input 提供一个指针，获取数据的指针，其参数是NN的输入结构，返回值是数据的指针
     * @attention 需要使用malloc分配内存，否则可能导致自动释放内存出现故障
     */
    void pushInput(const std::function<void*(std::vector<det::Binding>&)>& get_input)
    {
        auto ff = [this,get_input](int pool_id, int thread_id)
        {
            void* input = get_input(input_bindings_);
            void* ptr;
            if (!ThreadPool::try_to_malloc_static<_Infer>(pool_id, thread_id, infer_.get()) && !get_staticMem_ptr(pool_id, thread_id, &ptr))
            {
                std::cout << "Failed to allocate memory for pool " << pool_id << " thread " << thread_id << std::endl;
                throw std::runtime_error("Failed to allocate static memory for pool");
                return nullptr;
            }
            get_staticMem_ptr(pool_id, thread_id, &ptr);
            _Infer* infer = (_Infer*)ptr;
            infer->copy_from_data(input);
            infer->infer();
            std::vector<void*> output_vec = infer->getResult();
            post_function_(output_vec, output_bindings_);
            return nullptr;
        };
        thread_pool_.push(std::move(ff));
    }

    /*!
     * @brief 设置取答案循环的运行周期
     * @attention 请确保严谨设置，否则将影响程序运行效率或导致程序崩溃
     * @param time 取答案循环的运行周期
     */
    void set_getResult_timer(int time)
    {
        result_delay_ = time;
    }
private:
    /*!
     * @brife 转存NN网络接口
     */
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
    std::shared_ptr<_Infer> infer_;
    std::function<void*(std::vector<void*>&, std::vector<det::Binding>&)> post_function_;
    //线程管理
    ThreadPool thread_pool_;
    //结果管理
    int result_delay_ = 100;
    bool result_start_ = true;
    std::function<void(void*)> callback_;

    /*!
     * @brief 调起程序的取结果循环，取到答案后会调用回调函数
     */
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
