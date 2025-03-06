//
// Created by lbw on 25-2-14.
//
#ifdef TRT

#include "inferer/preset/TrtInfer.h"

namespace fs = ghc::filesystem;

TrtInfer::TrtInfer(const std::string& model_path,bool is_warmup, const std::string&)
    : InferBase()
{
    std::cout << std::endl << "||--------------You are using TensorRT--------------||" << std::endl;
    device_name_ = "CUDA";
    fs::path path{model_path};
    std::string suffix = path.extension();
    if (suffix == ".onnx")
    {
        auto onnx_path = path;
        auto engine_path = path.replace_extension("engine");
        if (!fs::exists(engine_path))

        {
            onnx_to_engine(onnx_path.c_str(), engine_path.c_str());
        }
        model_path_ = engine_path.string();
    }
    else if (suffix == ".engine")
    {
        model_path_ = model_path;
    }
    else
    {
        FYT_ASSERT_MSG(false, "Unsupported model extension.");
    }

    TrtInfer::init();
    if (is_warmup)
        TrtInfer::warmup();
}

TrtInfer::TrtInfer(const TrtInfer& other):InferBase()
{
    model_path_ = other.getModelPath();
    device_name_ = "CUDA";
    TrtInfer::init();
}

void TrtInfer::setModel(const std::string& model_path)
{
    model_path_ = model_path;
}

std::string TrtInfer::getModelPath() const
{
    return model_path_;
}

int TrtInfer::get_size()
{
    return sizeof(TrtInfer);
}

std::string TrtInfer::get_name()
{
    return "TrtInfer";
}

TrtInfer::~TrtInfer()
{
    delete context_;
    delete engine_;
    delete runtime_;

    cudaStreamDestroy(stream_);
    for (auto& ptr : device_ptrs_)
    {
        CHECK(cudaFree(ptr));
    }

    for (auto& ptr : host_ptrs_)
    {
        CHECK(cudaFreeHost(ptr));
    }
}


void TrtInfer::init()
{
    //create runtime
    runtime_ = nvinfer1::createInferRuntime(logger_);
    FYT_ASSERT_MSG(runtime_!=nullptr, "Failed to create runtime");
    //read the model
    std::ifstream ifs(model_path_, std::ios::binary);
    char* trtModelStream = nullptr;

    FYT_ASSERT_MSG(ifs.good(), "The Model is broken");
    ifs.seekg(0, std::ifstream::end);
    int size = ifs.tellg();
    ifs.seekg(0, std::ifstream::beg);
    trtModelStream = new char[size];

    FYT_ASSERT_MSG(trtModelStream!=nullptr, "No more space to allocate");
    ifs.read(trtModelStream, size);
    ifs.close();

    //创建推理引擎
    engine_ = runtime_->deserializeCudaEngine(trtModelStream, size);
    FYT_ASSERT_MSG(engine_!=nullptr, "Failed to deserializeCudaEngine");

    //创建推理上下文
    context_ = engine_->createExecutionContext();
    FYT_ASSERT_MSG(context_ != nullptr, "Failed to create ExecutionContext");
    delete[] trtModelStream;

    //创建推理流
    cudaStreamCreate(&stream_);

    //获取输入输出维度信息
    num_bindings_ = engine_->getNbIOTensors();
    for (int i = 0; i < num_bindings_; i++)
    {
        det::Binding binding;
        nvinfer1::Dims dims;

        std::string name = engine_->getIOTensorName(i);
        nvinfer1::DataType dtype = engine_->getTensorDataType(name.c_str());
        binding.name = name;
        binding.dsize = type_to_size(dtype);
        binding.CV_type = type_to_CVtype(dtype);

        auto IOmode = engine_->getTensorIOMode(name.c_str());
        if (IOmode == nvinfer1::TensorIOMode::kINPUT)
        {
            num_inputs_++;
            dims = engine_->getProfileShape(name.c_str(), 0, nvinfer1::OptProfileSelector::kMAX);
            context_->setInputShape(name.c_str(), dims);

            binding.dims = dims;
            binding.size = get_size_by_dims(dims);
            input_bindings_.push_back(binding);
        }
        else
        {
            num_outputs_++;
            dims = context_->getTensorShape(name.c_str());

            binding.dims = dims;
            binding.size = get_size_by_dims(dims);
            output_bindings_.push_back(binding);
        }
    }

    preMalloc();
}

void TrtInfer::warmup()
{
    for (int i = 0; i < 10; i++)
    {
        auto index = 0;
        for (const auto& binding : input_bindings_)
        {
            const size_t size = binding.size * binding.dsize;
            void* h_ptr = malloc(size);
            memset(h_ptr, 0, size);
            CHECK(cudaMemcpyAsync(device_ptrs_[index], h_ptr, size, cudaMemcpyHostToDevice, stream_));
            index++;
            free(h_ptr);
        }
        infer();
    }
}

void TrtInfer::preMalloc()
{
    for (auto& binding : input_bindings_)
    {
        void* d_ptr;
        CHECK(cudaMallocAsync(&d_ptr, binding.size*binding.dsize,stream_));
        device_ptrs_.push_back(d_ptr);

        const char* name = binding.name.c_str();
        context_->setInputShape(name, binding.dims);
        context_->setTensorAddress(name, d_ptr);
    }

    for (auto& binding : output_bindings_)
    {
        void *d_ptr, *h_ptr;
        size_t size = binding.size * binding.dsize;
        CHECK(cudaMallocAsync(&d_ptr,size*binding.dsize,stream_));
        CHECK(cudaHostAlloc(&h_ptr,size,0));
        device_ptrs_.push_back(d_ptr);
        host_ptrs_.push_back(h_ptr);

        const char* name = binding.name.c_str();
        context_->setTensorAddress(name, d_ptr);
    }
}

void TrtInfer::copy_from_data(const void* data)
{
    int start = 0;
    int index = 0;
    for (auto& binding : input_bindings_)
    {
        const size_t size = binding.size * binding.dsize;
        auto data_h_ptr = data + start;
        CHECK(cudaMemcpyAsync(device_ptrs_[index],data_h_ptr,size,cudaMemcpyHostToDevice,stream_));

        start += size;
        index++;
    }
}

void TrtInfer::infer()
{
    context_->enqueueV3(stream_);

    for (int i = 0; i < num_outputs_; i++)
    {
        size_t o_size = output_bindings_[i].size * output_bindings_[i].dsize;
        CHECK(cudaMemcpyAsync(
            host_ptrs_[i],device_ptrs_[i+num_inputs_],o_size,cudaMemcpyDeviceToHost,stream_));
    }
    CHECK(cudaStreamSynchronize(stream_));
}

void TrtInfer::infer_async(const void* input, void** output)
{
    nvinfer1::IExecutionContext* context = nullptr;
    context = engine_->createExecutionContext();

    cudaStream_t stream =nullptr;
    CHECK(cudaStreamCreate(&stream));
    std::vector<void*> d_ptrs;
    int start = 0;
    for (auto& input_binding : input_bindings_)
    {
        void* device_ptr;
        int size = input_binding.size * input_binding.dsize;
        CHECK(cudaMalloc(&device_ptr, size));
        CHECK(cudaMemcpyAsync(device_ptr,input+start,size,cudaMemcpyHostToDevice,stream));
        d_ptrs.push_back(device_ptr);

        context->setInputShape(input_binding.name.c_str(),input_binding.dims);
        context->setTensorAddress(input_binding.name.c_str(),device_ptr);
        start += size;
    }

    for (auto output_binding : output_bindings_)
    {
        void* device_ptr;
        int size = output_binding.size * output_binding.dsize;
        CHECK(cudaMalloc(&device_ptr, size));
        d_ptrs.push_back(device_ptr);

        context->setOutputTensorAddress(output_binding.name.c_str(),device_ptr);
    }
    context->enqueueV3(stream);

    int total_output_size = 1;
    for (auto output_binding : output_bindings_)
    {
        void* device_ptr;
        int size = output_binding.size * output_binding.dsize;
        CHECK(cudaMalloc(&device_ptr, size));
        d_ptrs.push_back(device_ptr);

        total_output_size *= output_binding.size*output_binding.dsize;
        context->setOutputTensorAddress(output_binding.name.c_str(),device_ptr);
    }
    *output = malloc(total_output_size);

    start = 0;
    for (int index = 0; index < num_outputs_; index++)
    {
        auto output_binding = output_bindings_[index];
        int size = output_binding.size * output_binding.dsize;
        CHECK(cudaMemcpyAsync(*output+start,d_ptrs[index+num_inputs_],size,cudaMemcpyDeviceToHost,stream));
        start += size;
    }

    cudaStreamDestroy(stream);
    delete context;
    for (auto& d_ptr : d_ptrs)
    {
        CHECK(cudaFree(d_ptr));
    }
}

std::vector<void*>& TrtInfer::getResult()
{
    return host_ptrs_;
}

void TrtInfer::onnx_to_engine(const char* onnx_filename, const char* engine_filePath)
{
    std::cout << std::endl << "Detected ONNX..." << std::endl;
    std::cout << "Prepare to transfrom to Engine..." << std::endl;

	Logger logger{nvinfer1::ILogger::Severity::kERROR};
	//通过Logger类创建builder
	nvinfer1::IBuilder* builder = nvinfer1::createInferBuilder(logger);

	// 创建network kEXPLICIT_BATCH代表显式batch（推荐使用），即tensor中包含batch这个纬度。
	uint32_t flag = 1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    nvinfer1::INetworkDefinition* network = builder->createNetworkV2(flag);

	// 创建onnx模型解析器
	nvonnxparser::IParser* parser = nvonnxparser::createParser(*network, logger);

	// 解析模型
	parser->parseFromFile(onnx_filename, static_cast<uint32_t>(nvinfer1::ILogger::Severity::kWARNING));

	// 设置config
    nvinfer1::IBuilderConfig* config = builder->createBuilderConfig();

	// 设置工作空间大小
	config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, 16 * (1 << 20));

	// 设置以半精度构建engine，我们在torch模型中的数据是32位浮点数即fp32，
	// tensorrt中可以直接将权重量化为FP16，以提升速度，若要量化为INT8，则需要设置数据校准。
	// INT8量化可以参照YOLO的代码。
	// 这里不介绍模型量化的原理。
	config->setFlag(nvinfer1::BuilderFlag::kFP16);

	// 创建profile，设置engine序列化
    nvinfer1::IOptimizationProfile* profile = builder->createOptimizationProfile();

	// 如果在导出onnx模型时，设置了动态batch（可以看我上一篇博客），这里需要设置输入模型的纬度范围。
	// 最小纬度
	profile->setDimensions(onnx_filename, nvinfer1::OptProfileSelector::kMIN, nvinfer1::Dims4(1, 3, 640, 640));
	// // 最合适的纬度
	// profile->setDimensions("onnx导出时的输入名字，一定要一样", nvinfer1::OptProfileSelector::kOPT, nvinfer1::Dims4(1, 3, 1080, 1920));
	// // 最大纬度，建议设置多batch，后续如果要使用多batch推理，就不用重新导出engine。
	// profile->setDimensions("onnx导出时的输入名字，一定要一样", nvinfer1::OptProfileSelector::kMAX, Dims4(8, 3, 1080, 1920));

	// 设置profile，并序列化构建engine
	config->addOptimizationProfile(profile);
    nvinfer1::IHostMemory* serializedModel = builder->buildSerializedNetwork(*network, *config);
	std::ofstream p(engine_filePath, std::ios::binary);
	//p.write(reinterpret_cast<const char*>(serializedModel->data()), serializedModel->size());

	// 最后别忘了清理内存空间
	delete parser;
	delete network;
	delete config;
	delete builder;
	delete serializedModel;
}

#endif
