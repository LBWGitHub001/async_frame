# 神经网络异步推理框架

## 当前支持OpenVINO和TensorRT

在cmake/FindTensorRT.cmake的`set(_TensorRT_SEARCH_NORMAL PATHS "/usr" )`
中添加你的TensorRT安装路径

# 最简调用方式
1. setInfer
2. registerPostprocess
3. set_getResult_timer
4. pushInput

# 线程池(ThreadPool)
# 函数说明
1. push() 向线程池中加入任务，是线程安全的
2. free_push() 向线程池中快速加入任务，跳过了冲突检查，线程不安全
3. force_push() 向线程池中强制加入任务，自动选择线程数，直至输入和输出频率匹配。但是会占用大量CPU算力，也存在创建失败的可能。
在使用之前要仔细分析输入输出的频率，保证有足够的资源可以使用。
4. join() 等待线程池中所有任务全部完成。