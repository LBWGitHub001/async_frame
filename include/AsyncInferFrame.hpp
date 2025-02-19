//
// Created by lbw on 25-2-17.
//

#ifndef ASYNCINFERFRAME_H
#define ASYNCINFERFRAME_H

#include "inferer/AsyncInferer.h"
#include "threadPool/threadPool.h"

#ifdef TRT
#define AUTO_INFER TrtInfer
#else
#define AUTO_INFER VinoInfer
#endif

#endif //ASYNCINFERFRAME_H
