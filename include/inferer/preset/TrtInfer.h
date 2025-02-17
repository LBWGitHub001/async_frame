//
// Created by lbw on 25-2-14.
//

#ifndef TRTINFER_H
#define TRTINFER_H
#include "inferer/preset/InferBase.h"

#ifdef TRT
#include <NvInfer.h>
#include <NvInferPlugin.h>
#include <NvOnnxParser.h>
#include <NvInferRuntime.h>
#endif

#ifdef TRT

class TrtInfer
    : public InferBase
{
public:
    explicit TrtInfer(const std::string& model_path,bool is_warmup = false,const std::string& device_name_="CUDA");
    ~TrtInfer() override;
    void init() override;
    void warmup() override;
    void preMalloc();
    void copy_from_data(const void* data) override;
    void infer() override;
    void infer_async(const void* input,void** output) override;
    std::vector<void*>& getResult() override;
    //tool
    void onnx_to_engine(const char* onnx_filename, const char* engine_filePath);

private:
    nvinfer1::IRuntime* runtime_ = nullptr;
    nvinfer1::ICudaEngine* engine_ = nullptr;
    nvinfer1::IExecutionContext* context_ = nullptr;
    cudaStream_t stream_ = nullptr;

    int num_bindings_ = 0;

    std::vector<void*> host_ptrs_;
    std::vector<void*> device_ptrs_;
    Logger logger_{nvinfer1::ILogger::Severity::kERROR};
};

#endif



#endif //TRTINFER_H
