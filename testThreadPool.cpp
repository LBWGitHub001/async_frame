//
// Created by lbw on 25-2-18.
//
#include "threadPool/ThreadPool.h"
#include <opencv2/opencv.hpp>

std::vector<int>* function(std::vector<int>& i)
{
    auto* ans = new std::vector<int>;
    *ans = i;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return ans;
}

cv::Mat* imgfuncton(cv::Mat& img)
{
    auto* img_ptr = new cv::Mat;
    img_ptr = &img;
    return img_ptr;
}

int main()
{
    ThreadPool pool;
    cv::Mat img = cv::imread("/home/lbw/RM2025/kalman-fix/RM2024_nice/src/rm_utils/picture/000006.jpg");
    std::vector<int> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (auto i = 0; i < 1000; ++i)
    {
        pool.force_push(std::bind(imgfuncton, std::ref(img)));
    }
    std::vector<std::vector<int>> results;
    for (auto i = 0; i < 1000;)
    {
        cv::Mat image;
        if (pool.image_get(image))
        {
            //results.push_back(out);
            cv::imshow("image", image);
            cv::waitKey(1);
            i++;
        }
    }

    pool.join();
    // std::vector<int> vec{0,1,2,3,4,5,6,7,8,9};
    // std::cout << sizeof(std::vector<int>) << std::endl;
    return 0;
}
