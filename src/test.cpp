//
// Created by lbw on 25-2-11.
//
#include <inferer/AsyncInferer.h>
#include <iostream>
#include <thread>
using namespace std::placeholders;

void preProcess(void** input, std::vector<det::Binding>& input_bindings, cv::Mat img)
{
    cv::Mat img_tmp;
    auto input_binding = input_bindings[0];
    auto input_dim = input_binding.dims.d;
    auto cv_type = input_binding.CV_type;
    cv::cvtColor(img, img_tmp, cv::COLOR_BGR2RGB);

    cv::Size target_size{static_cast<int>(input_dim[3]), static_cast<int>(input_dim[2])};
    cv::resize(img_tmp, img_tmp, target_size);

    if (img_tmp.type() != cv_type)
    {
        img_tmp.convertTo(img_tmp, cv_type, 1 / 255.0);
    }

    cv::Mat input_tensor = cv::dnn::blobFromImage(img_tmp);
    auto out = input_tensor.ptr<float>();
    int size = input_bindings[0].size * input_bindings[0].dsize;
    *input = malloc(size);
    memcpy(*input, out, size);
}

void postProcess(void* output, std::vector<det::Binding>& output_bindings,long timestamp)
{
    std::vector<cv::Rect> results;
    // std::cout << "postprocessing" << std::endl;
    std::vector<cv::Rect> rects;
    std::vector<float> confs;
    const det::Binding& data = output_bindings[0];
    int size = data.size;
    int dsize = data.dsize;
    auto dims = data.dims;
    int num_archs = dims.d[2];
    int size_pre_arch = dims.d[1];
    auto buf = static_cast<float*>(output);
    auto index = [num_archs](int arch, int content) { return arch + content * num_archs; };
    for (int j = 0; j < num_archs; j++)
    {
        cv::Rect arch;
        float cx = buf[index(j, 0)];
        float cy = buf[index(j, 1)];
        float w = buf[index(j, 2)];
        float h = buf[index(j, 3)];
        arch = cv::Rect(cx - w / 2.0, cy - h / 2.0, w, h);
        float conf = buf[index(j, 4)];
        rects.push_back(arch);
        confs.push_back(conf);
    }
    std::vector<int> indexes;
    cv::dnn::NMSBoxes(rects, confs, 0.6, 0.6, indexes);
    for (auto i : indexes)
    {
        results.push_back(rects[i]);
    }
    // std::cout << "PostProcess Done" << std::endl;
}

int main()
{
    AsyncInferer infer;

    infer.setInfer(std::make_unique<VinoInfer>(
        "/home/lbw/RM2025/kalman-fix/RM2024_nice/src/rm_utils/model/robot.onnx",
        false,
        "CPU"
    ));
    infer.registerCallback(postProcess);

    std::string path = "/home/lbw/RM2025/kalman-fix/RM2024_nice/src/rm_utils/picture/0000";
    auto now = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    for (auto j=0;j<5;j++)
    {
           for (auto i = 0; i < 30; i++)
    {
        std::string num = std::to_string(i);
        if (i < 10)
            num = "0" + num;
        std::string pic = num + ".jpg";
        std::string pic_path = path + pic;
        cv::Mat img = cv::imread(pic_path, cv::IMREAD_COLOR);
        infer.pushInput(std::bind(preProcess, _1, _2, std::move(img.clone())), i+j*30);
               std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    }


    std::this_thread::sleep_for(std::chrono::milliseconds(200000));
    std::cout << "SHUTDOWN" << std::endl;
    return 0;
}
