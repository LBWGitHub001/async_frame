//
// Created by lbw on 25-2-14.
//
#include "inferer/preset/ppp.h"

namespace prePreset
{
    void image_input(cv::Mat& img, const std::vector<det::Binding>& input_bindings, void** buf)
    {
        cv::Mat img_scale;
        auto input_dim = input_bindings[0].dims.d;
        //对图像进行格式转换
        int cv_type = input_bindings[0].CV_type;
        //! 调整通道顺序
        cv::cvtColor(img, img_scale, cv::COLOR_BGR2RGB);
        //! 调整图像大小
        cv::Size target_size{static_cast<int>(input_dim[3]), static_cast<int>(input_dim[2])};
        cv::resize(img, img_scale, target_size);
        //! 调整图像类型
        if (img_scale.type() != cv_type)
        {
            img_scale.convertTo(img_scale, cv_type, 1 / 255.0);
        }
        //! 调整为张量形式NCWH
        cv::Mat input;
        input = cv::dnn::blobFromImage(img_scale);
        auto out = input.ptr<float>();
        int size = input_bindings[0].size * input_bindings[0].dsize;
        *buf = malloc(size);
        memcpy(*buf, out, size);
    }

    void tensor_input(std::vector<float>& data, void** buf)
    {
        *buf = new float[data.size()];
        memcpy(*buf, data.data(), data.size() * sizeof(float));
    }
}
