//
// Created by lbw on 25-2-18.
//
#include "threadPool/ThreadPool.h"
#include <opencv2/opencv.hpp>

void function(cv::Mat image)
{
    image.resize(640,640);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}


int main()
{
    ThreadPool pool;
    cv::Mat img = cv::imread("/home/lbw/RM2025/kalman-fix/RM2024_nice/src/rm_utils/picture/000006.jpg");
    for (auto i = 0; i < 100; ++i)
    {
        pool.force_push(std::bind(function,img.clone()));
    }
    // pool.join();
return 0;
}
