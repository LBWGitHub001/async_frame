//
// Created by ubuntu on 1/24/23.
//

#ifndef DETECT_NORMAL_COMMON_HPP
#define DETECT_NORMAL_COMMON_HPP
#include "opencv2/opencv.hpp"
#include "assert.hpp"
#include "filesystem.hpp"
#ifdef VINO
#include "openvino/openvino.hpp"
#endif

#ifdef TRT
#include "NvInfer.h"
#define CHECK(call)                                                                                                    \
    do {                                                                                                               \
        const cudaError_t error_code = call;                                                                           \
        if (error_code != cudaSuccess) {                                                                               \
            printf("CUDA Error:\n");                                                                                   \
            printf("    File:       %s\n", __FILE__);                                                                  \
            printf("    Line:       %d\n", __LINE__);                                                                  \
            printf("    Error code: %d\n", error_code);                                                                \
            printf("    Error text: %s\n", cudaGetErrorString(error_code));                                            \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)

class Logger : public nvinfer1::ILogger
{
public:
    nvinfer1::ILogger::Severity reportableSeverity;

    explicit Logger(nvinfer1::ILogger::Severity severity = nvinfer1::ILogger::Severity::kINFO):
        reportableSeverity(severity)
    {
    }

    void log(nvinfer1::ILogger::Severity severity, const char* msg) noexcept override
    {
        if (severity > reportableSeverity)
        {
            return;
        }
        switch (severity)
        {
        case nvinfer1::ILogger::Severity::kINTERNAL_ERROR:
            std::cerr << "INTERNAL_ERROR: ";
            break;
        case nvinfer1::ILogger::Severity::kERROR:
            std::cerr << "ERROR: ";
            break;
        case nvinfer1::ILogger::Severity::kWARNING:
            std::cerr << "WARNING: ";
            break;
        case nvinfer1::ILogger::Severity::kINFO:
            std::cerr << "INFO: ";
            break;
        default:
            std::cerr << "VERBOSE: ";
            break;
        }
        std::cerr << msg << std::endl;
    }
};
#endif



#ifndef TRT
namespace nvinfer1
{
    class Dims
    {
    public:
        //! The maximum rank (number of dimensions) supported for a tensor.
        static constexpr int32_t MAX_DIMS{8};

        //! The rank (number of dimensions).
        int32_t nbDims;

        //! The extent of each dimension.
        int64_t d[MAX_DIMS];
    };
}
#endif

#ifdef TRT
inline int get_size_by_dims(const nvinfer1::Dims& dims)
{
    int size = 1;
    for (int i = 0; i < dims.nbDims; i++)
    {
        size *= dims.d[i];
    }
    return size;
}

inline int type_to_size(const nvinfer1::DataType& dataType)
{
    switch (dataType)
    {
    case nvinfer1::DataType::kFLOAT:
        return 4;
    case nvinfer1::DataType::kHALF:
        return 2;
    case nvinfer1::DataType::kINT32:
        return 4;
    case nvinfer1::DataType::kINT8:
        return 1;
    case nvinfer1::DataType::kBOOL:
        return 1;
    default:
        return 4;
    }
}


inline int type_to_CVtype(const nvinfer1::DataType& dataType)
{
    switch (dataType)
    {
    case nvinfer1::DataType::kFLOAT:
        return CV_32F;
    case nvinfer1::DataType::kHALF:
        return CV_16F;
    case nvinfer1::DataType::kINT32:
        return CV_32S;
    case nvinfer1::DataType::kINT8:
        return CV_8S;
    default:
        return CV_32F;
    }
}
#endif

#ifdef VINO
inline int type_to_size(const ov::element::Type& dataType)
{
    switch (dataType)
    {
    case ov::element::bf16:
    case ov::element::f16:
        return 2;
    case ov::element::f32:
        return 4;
    case ov::element::f64:
        return 8;
    // case ov::element::i4:
    //    return 0.5;
    case ov::element::i8:
        return 1;
    case ov::element::i16:
        return 2;
    case ov::element::i32:
        return 4;
    case ov::element::i64:
        return 8;
    case ov::element::u8:
        return 8;
    case ov::element::u16:
        return 16;
    case ov::element::u32:
        return 32;
    case ov::element::u64:
        return 64;
    default:
        return 4;
    //FYT_ASSERT("this inputType is Incorrect!");
    }
}

inline int type_to_CVtype(const ov::element::Type& dataType)
{
    switch (dataType)
    {
    case ov::element::f16:
        return CV_16F;
    case ov::element::f32:
        return CV_32F;
    case ov::element::f64:
        return CV_64F;
    case ov::element::i8:
        return CV_8S;
    case ov::element::i16:
        return CV_16S;
    case ov::element::i32:
        return CV_32S;
    case ov::element::u8:
        return CV_8U;
    case ov::element::u16:
        return CV_16U;
    default:
        return CV_32F;
    //FYT_ASSERT("this inputType is Incorrect!");
    }
}
#endif

inline static float clamp(float val, float min, float max)
{
    return val > min ? (val < max ? val : max) : min;
}

namespace det
{
    /**
     * size:        张量的总长度
     * dsize:       张量单个元素的大小（byte）
     * dims:        具体尺寸信息
     * name:        张量名称
     * is_dynamic   是否是动态尺寸
     */
    struct Binding
    {
        size_t size = 1;
        size_t dsize = 1;
        nvinfer1::Dims dims;
        std::string name;
        int CV_type = CV_32F;

        bool is_dynamic = false;
    };

    struct Object
    {
        cv::Rect_<float> rect;
        int label = 0;
        float prob = 0.0;
    };

    struct PreParam
    {
        float ratio = 1.0f;
        float dw = 0.0f;
        float dh = 0.0f;
        float height = 0;
        float width = 0;
    };
} // namespace det

enum class InferState
{
    Balance,
    Accelerate,

};

#endif  // DETECT_NORMAL_COMMON_HPP
