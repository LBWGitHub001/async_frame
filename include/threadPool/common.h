//
// Created by lbw on 25-2-19.
//

#ifndef COMMON_H
#define COMMON_H
#include "threadPool/check.h"
#include <future>

namespace thread_pool
{
    struct TaskThread
    {
        std::unique_ptr<std::future<void*>> future = nullptr;
        time_t timestamp = 1e10;
        void* result_ptr = nullptr;

        ~TaskThread()
        {
            result_ptr = nullptr;
        }
    };

    struct Result
    {
        void* result_ptr = nullptr;
        time_t timestamp = 1e10;
        Result& operator=(Result& other)
        {
            result_ptr = other.result_ptr;
            timestamp = other.timestamp;
        }
    };
}


#endif //COMMON_H
