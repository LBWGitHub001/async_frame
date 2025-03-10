//
// Created by lbw on 25-2-19.
//

#ifndef COMMON_H
#define COMMON_H
#include "threadPool/check.h"
#include <future>

namespace thread_pool
{
    template<class _Result,class _Tag>
    struct TaskThread
    {
        std::unique_ptr<std::future<_Result>> future = nullptr;
        time_t timestamp = 1e10;
        _Result result;
        _Tag tag;

    };

    template<class _Result,class _Tag>
    struct Result
    {
        time_t timestamp = 1e10;
        _Result result;
        _Tag tag;
        Result(time_t timestamp, _Result result, _Tag tag) :timestamp(timestamp), result(result), tag(tag) {}
        Result() = default;
    };

    template <class _Static = nullptr_t>
    struct StaticTaskThread
    {
        std::unique_ptr<std::future<void*>> future = nullptr;
        time_t timestamp = 1e10;
        void* result_ptr = nullptr;
        _Static infer;
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
