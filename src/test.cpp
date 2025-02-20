//
// Created by lbw on 25-2-11.
//
#include "AsyncInferFrame.hpp"
#include <iostream>
#include <thread>
using namespace std::placeholders;

void* preProcess(std::vector<det::Binding>& input_bindings, cv::Mat& img)
{
    std::cout << "preProcess Start" << std::endl;
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
    std::cout << "Preprocess Done" << std::endl;
    cv::Mat input_tensor = cv::dnn::blobFromImage(img_tmp);
    auto out = input_tensor.ptr<float>();
    int size = input_bindings[0].size * input_bindings[0].dsize;
    void* input = malloc(size);
    memcpy(input, out, size);
    return input;
}

void* postProcess(std::vector<void*>& output_vec, std::vector<det::Binding>& output_bindings)
{
    auto output = output_vec[0];
    std::vector<cv::Rect>* results = new std::vector<cv::Rect>();
    std::cout << "postprocessing" << std::endl;
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
    if (!indexes.empty())
        for (auto i : indexes)
        {
            results->push_back(rects[i]);
        }
    return results;
}

void callback(void* result)
{
    std::cout << "copy that" << std::endl;
}

int main()
{
    AsyncInferer<AUTO_INFER> infer;

    infer.setInfer(std::make_unique<AUTO_INFER>(
        "/home/lbw/RM2025/kalman-fix/AsyncInferFrame/model/robot.engine"
    ));
    infer.registerPostprocess(postProcess);
    infer.registerCallback(callback);
    // infer.registerFreeStatic([](void* result)
    // {
    //     std::vector<cv::Rect>* results = (std::vector<cv::Rect>*)result;
    //     delete results;
    // });
    std::string path = "/home/lbw/RM2025/kalman-fix/RM2024_nice/src/rm_utils/picture/0000";

    ThreadPool pool;
    for (auto j = 0; j < 5; j++)
    {
        for (auto i = 0; i < 10; i++)
        {
            std::string num = std::to_string(i);
            if (i < 10)
                num = "0" + num;
            std::string pic = num + ".jpg";
            std::string pic_path = path + pic;
            cv::Mat img = cv::imread(pic_path, cv::IMREAD_COLOR);
            infer.pushInput(std::bind(preProcess, _1, std::move(img.clone())));
        }
    }

    return 0;
}
