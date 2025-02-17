//
// Created by lbw on 25-2-14.
//

#include "inferer/preset/InferBase.h"

const std::vector<det::Binding>& InferBase::getInputBinding()
{
    return input_bindings_;
}

const std::vector<det::Binding>& InferBase::getOutputBinding()
{
    return output_bindings_;
}