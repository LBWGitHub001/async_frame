//
// Created by lbw on 25-2-14.
//

#include "inferer/preset/VinoInfer.h"
namespace fs = ghc::filesystem;

VinoInfer::VinoInfer(const std::string& model_path, bool is_warmup, const std::string& device_name)
    : InferBase(),MemBlockBase()
{
    std::cout << std::endl << "||--------------You are using OpenVino--------------||" << std::endl;
    fs::path path{model_path};
    const std::string suffix = path.extension();
    FYT_ASSERT_MSG(suffix==".onnx"||suffix==".xml", "Wrong suffix");
    model_path_ = model_path;
    device_name_ = device_name;

    VinoInfer::init();
    if (is_warmup)
        VinoInfer::warmup();
}

VinoInfer::VinoInfer(VinoInfer& other):InferBase(),MemBlockBase()
{
    model_path_ = other.getModelPath();
    device_name_ = other.getDevice();
    VinoInfer::init();
}

VinoInfer::~VinoInfer()
{
}

void VinoInfer::setModel(const std::string& model_pat)
{
    model_path_ = model_pat;
}

void VinoInfer::setDevice(const std::string& device_name)
{
    device_name_ = device_name;
}

const std::string& VinoInfer::getModelPath() const
{
    return model_path_;
}

const std::string& VinoInfer::getDevice() const
{
    return device_name_;
}

const int VinoInfer::get_size()
{
    return sizeof(VinoInfer);
}

const std::string VinoInfer::get_name()
{
    return "VinoInfer";
}

void VinoInfer::init()
{
    if (ov_core_ == nullptr)
    {
        ov_core_ = std::make_unique<ov::Core>();
    }

    std::vector<std::string> availableDevices = ov_core_->get_available_devices();
    bool gpuAvailable = std::find(availableDevices.begin(), availableDevices.end(), "GPU") != availableDevices.end();

    auto model = ov_core_->read_model(model_path_);
    FYT_ASSERT_MSG(model != nullptr, "Failed to read model");
    bool useGPU = device_name_ == "GPU" && gpuAvailable;
    if (device_name_ == "GPU" && !useGPU)
    {
        std::cout << "Your device do not support GPU-infer..." << std::endl;
        std::cout << "Will use CPU" << std::endl;
    }
    auto perf_mode = useGPU
                         ? ov::hint::performance_mode(ov::hint::PerformanceMode::THROUGHPUT)
                         : ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY);

    // Compile model
    compiled_model_ = std::make_unique<ov::CompiledModel>(
        ov_core_->compile_model(model_path_, device_name_, perf_mode));
    FYT_ASSERT_MSG(compiled_model_ != nullptr, "Failed to compile model");

    request_ = compiled_model_->create_infer_request();
    FYT_ASSERT_MSG(request_, "Failed to create infer request");

    auto input_Shapes = model->inputs();
    auto output_Shapes = model->outputs();

    auto input_ov_to_nvifer =
        [&](const std::vector<ov::Output<ov::Node>>& ov_Shapes, const std::string& in_or_out) mutable
    {
        std::vector<det::Binding> bindings;
        for (auto& Node : ov_Shapes)
        {
            det::Binding binding;
            int size = 1;
            int dsize = 1;
            int CV_type = 1;
            nvinfer1::Dims dims;
            std::string name;
            bool is_dynamic_layer = false;

            //获取输入的信息
            ov::element::Type type = Node.get_element_type();
            dsize = type_to_size(type);
            CV_type = type_to_CVtype(type);
            try
            {
                name = Node.get_any_name();
            }
            catch (...)
            {
                name = "";
            }
            ov::Node* node = Node.get_node();
            if (node->is_dynamic())
            {
                is_dynamic_ = true;
                is_dynamic_layer = true;
            }
            ov::Tensor tensor;
            if (in_or_out == "input")
                tensor = request_.get_input_tensor();
            else if (in_or_out == "output")
                tensor = request_.get_output_tensor();
            else
                throw std::runtime_error("Only support \"input\" and \"output\" param");
            ov::Shape shape = tensor.get_shape();
            //获取输入输出等信息
            auto dim = shape.data();
            auto num_dims = shape.size();
            dims.nbDims = num_dims;
            for (int i = 0; i < num_dims; i++)
            {
                dims.d[i] = dim[i];
                size *= dim[i];
            }

            binding.dims = dims;
            binding.dsize = dsize;
            binding.size = size;
            binding.CV_type = CV_type;
            binding.name = name;
            binding.is_dynamic = is_dynamic_layer;
            bindings.push_back(binding);
        }
        return bindings;
    };


    std::vector<det::Binding> input_dims = input_ov_to_nvifer(input_Shapes, "input");
    std::vector<det::Binding> output_dims = input_ov_to_nvifer(output_Shapes, "output");
    num_inputs_ = input_dims.size();
    num_outputs_ = output_dims.size();
    input_bindings_ = input_dims;
    output_bindings_ = output_dims;
}

void VinoInfer::warmup()
{
    for (int i = 0; i < 10; i++)
    {
        for (auto& binding : input_bindings_)
        {
            float* input_data = new float[binding.size];
            memset(input_data, 0, sizeof(float) * binding.size);
            copy_from_data(input_data);
            infer();
            delete[] input_data;
        }
    }
}

void VinoInfer::preMalloc()
{
    //Nothing,Vino seems not to need this to accuralate
}

void VinoInfer::copy_from_data(const void* data, const ov::Shape& shape)
{
    request_ = compiled_model_->create_infer_request();
    const auto input_port_ = compiled_model_->input();
    int size = 1;
    for (auto len : shape)
        size *= len;
    auto input_port = compiled_model_->input();
    ov::Tensor input = request_.get_tensor(input_port);
    memcpy(input.data(), data, size * sizeof(float));
    //request_.set_input_tensor(input_tensor);
}

void VinoInfer::copy_from_data(const void* data)
{
    auto data_ptr = data;
    for (auto& binding : input_bindings_)
    {
        auto size = binding.size;
        auto dims = binding.dims;
        ov::Shape shape(dims.d, dims.d + dims.nbDims);
        copy_from_data(data_ptr, shape);
        data_ptr += size;
    }
}

void VinoInfer::infer()
{
    request_.infer();
}

void VinoInfer::infer_async(const void* input, void** output)
{
    auto request = compiled_model_->create_infer_request();
    const auto input_port_ = compiled_model_->input();

    int input_total_size = 1;
    for (auto& input_binding : input_bindings_)
    {
        int input_size = input_binding.size;
        int input_dsize = input_binding.dsize;
        input_total_size *= input_size * input_dsize;
    }
    auto input_port = compiled_model_->input();
    ov::Tensor input_tensor = request.get_tensor(input_port);
    memcpy(input_tensor.data(), input, input_total_size);
    request.infer();

    int output_total_size = 1;
    for (auto& output_binding : output_bindings_)
    {
        int output_size = output_binding.size;
        int output_dsize = output_binding.dsize;
        output_total_size *= output_size * output_dsize;
    }

    ov::Tensor output_tensor = request.get_output_tensor();
    *output = malloc(output_total_size);
    memcpy(*output, output_tensor.data(), output_total_size);
}

std::vector<void*>& VinoInfer::getResult()
{
    auto output_tensor = request_.get_output_tensor();
    //TODO 这里应该适配更多的数据输出情况
    outputs_.push_back(output_tensor.data<float>());
    return outputs_;
}
