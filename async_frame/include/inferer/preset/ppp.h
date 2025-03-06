//
// Created by lbw on 25-2-14.
//

#ifndef PPP_H
#define PPP_H
#include <functional>
#include "inferer/common.hpp"

namespace prePreset
{
    //! 使用图形输入
    using ImageInFunc = std::function<void(cv::Mat&,const std::vector<det::Binding>&, void**)>;
    void image_input(cv::Mat& img, const std::vector<det::Binding>& input_bindings, void** buf);

    //! 使用张量输入
    using TensorInFunc = std::function<void(std::vector<float>&, void**)>;
    void tensor_input(std::vector<float>& data, void** buf);
}


namespace pppPreset
{
    using NOMAL_IMAGE = cv::Mat;
    using NOMAL_TENSOR = std::vector<float>;
    /**
     * @class yolov10_Armor
     * @bref YOLOv10的装甲板四点模型
     */
    class yolov10_Armor
    {
        struct output_struct
        {
            float conf = 0;
            int id = -1;
            cv::Rect bbox;
            std::vector<cv::Point> points;
        };

    public:
        //! 不再补充除了张量和图形之外的输入方式
        using Input = NOMAL_IMAGE;
        //! 模型输出类型
        using Output = std::vector<output_struct>;
        //! 不再进行预处理
        prePreset::ImageInFunc preProcess = prePreset::image_input;

        using PostFunction = std::function<Output(std::vector<void*>&, const std::vector<det::Binding>&)>;
        PostFunction postProcess =
            [](std::vector<void*>& origin_data,
                const std::vector<det::Binding>& output_binding)
            -> Output
        {
            Output output;
            std::vector<cv::Rect> rects;
            std::vector<float> confs;
            const det::Binding& data = output_binding[0];
            int size = data.size;
            // int dsize = data.dsize;
            auto dims = data.dims;
            int num_archs = dims.d[1];
            int size_pre_arch = dims.d[2];
            auto buf = static_cast<float*>(origin_data[0]);
            auto index = [size_pre_arch](int arch, int content) { return arch * size_pre_arch + content; };
            for (int j = 0; j < num_archs; j++)
            {
                output_struct arch;
                float cx = buf[index(j, 0)];
                float cy = buf[index(j, 1)];
                float w = buf[index(j, 2)];
                float h = buf[index(j, 3)];
                arch.conf = buf[index(j, 4)];
                arch.id = buf[index(j, 5)];
                arch.bbox = cv::Rect(cx-w/2, cy-h/2, w, h);
                for (int k = 1; k <= 4; k++)
                {
                    float xx = buf[index(j, 6 + 2 * (k - 1))];
                    float yy = buf[index(j, 7 + 2 * (k - 1))];
                    cv::Point p(xx, yy);
                    arch.points.push_back(p);
                }
                output.push_back(arch);
                confs.push_back(arch.conf);
                rects.push_back(arch.bbox);
            }
            std::vector<int> indexes;
            cv::dnn::NMSBoxes(rects, confs, 0.8, 0.6, indexes);
            Output result;
            for (auto i : indexes)
            {
                result.push_back(output[i]);
            }
                std::vector<float> tmp;
                for (auto i =0;i<size;i++)
                {
                    tmp.push_back(buf[i]);
                }
            return result;
        };
    };

    /**
     * @class yolov10_Robot
     * @bref YOLOv10的机器人识别模型
     */
    class yolov8_Robot
    {
        struct output_struct
        {
            float conf = 0;
            cv::Rect bbox;
        };

    public:
        using Input = NOMAL_IMAGE;
        using Output = std::vector<output_struct>;
        prePreset::ImageInFunc preProcess = prePreset::image_input;
        using PostFunction = std::function<Output(std::vector<void*>&, const std::vector<det::Binding>&)>;
        PostFunction postProcess =
            [](std::vector<void*>& origin_data,
                const std::vector<det::Binding>& output_binding)
            -> Output
        {
            Output output;
            std::vector<cv::Rect> rects;
            std::vector<float> confs;
            const det::Binding& data = output_binding[0];
            // int size = data.size;
            // int dsize = data.dsize;
            auto dims = data.dims;
            int num_archs = dims.d[2];
            // int size_pre_arch = dims.d[1];
            auto buf = static_cast<float*>(origin_data[0]);
            auto index = [num_archs](int arch, int content) { return arch + content * num_archs; };
            for (int j = 0; j < num_archs; j++)
            {
                output_struct arch;
                float cx = buf[index(j, 0)];
                float cy = buf[index(j, 1)];
                float w = buf[index(j, 2)];
                float h = buf[index(j, 3)];
                arch.conf = buf[index(j, 4)];
                arch.bbox = cv::Rect(cx-w/2.0, cy-h/2.0, w, h);
                output.push_back(arch);
                confs.push_back(arch.conf);
                rects.push_back(arch.bbox);
            }
            std::vector<float> tmp;
            for (int i =0;i<num_archs;i++)
            {
                auto bufff = static_cast<float*>(origin_data[0]);
                tmp.push_back(bufff[i]);
            }

            std::vector<int> indexes;
            cv::dnn::NMSBoxes(rects, confs, 0.6, 0.6, indexes);
            Output result;
            for (auto i : indexes)
            {
                //if (confs[i] < 1)
                result.push_back(output[i]);
            }
            return result;
        };
    };
}
#endif //PPP_H
