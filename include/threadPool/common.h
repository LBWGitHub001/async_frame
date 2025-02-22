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
        // void* result_ptr = nullptr;
        void* tag = nullptr;

        ~TaskThread()
        {
            // result_ptr = nullptr;
            tag = nullptr;
        }
    };

    template <class _Static = nullptr_t>
    struct StaticTaskThread
    {
        std::unique_ptr<std::future<void*>> future = nullptr;
        time_t timestamp = 1e10;
        void* result_ptr = nullptr;
        _Static infer;
    };

    struct Result
    {
        void* result_ptr = nullptr;
        void* tag = nullptr;
        time_t timestamp = 1e10;

        Result& operator=(Result& other)
        {
            result_ptr = other.result_ptr;
            tag = other.tag;
            timestamp = other.timestamp;
            return *this;
        }
        Result(void* result_ptr, void* tag, time_t timestamp):
        result_ptr(result_ptr),tag(tag),timestamp(timestamp)
        {}
    };

    struct threadBinding
    {
        int pool_id;
        int thread_id;
        void* static_memory;
        threadBinding(int _pool_id_) { pool_id = _pool_id_; }
    };

    namespace Array
    {
        struct Char
        {
            void* ptr = nullptr;
            size_t size = 0;
            unsigned int dsize = sizeof(char);
        };

        struct Int
        {
            void* ptr = nullptr;
            size_t size = 0;
            unsigned int dsize = sizeof(int);
        };

        struct Float
        {
            void* ptr = nullptr;
            size_t size = 0;
            unsigned int dsize = sizeof(float);
        };

        struct Double
        {
            void* ptr;
            size_t size = 0;
            unsigned int dsize = sizeof(double);
        };

        struct Array
        {
            void* ptr = nullptr;
            size_t size = 0;
            unsigned dsize = 0;

            explicit operator Char() const
            {
                return Char{ptr, size, dsize};
            }

            explicit operator Int() const
            {
                return Int{ptr, size, dsize};
            }

            explicit operator Float() const
            {
                return Float{ptr, size, dsize};
            }

            explicit operator Double() const
            {
                return Double{ptr, size, dsize};
            }
        };
    }
}


#endif //COMMON_H
