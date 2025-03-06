//
// Created by lbw on 25-2-14.
//
#ifndef INFERBASE_H
#define INFERBASE_H
#include <vector>
#include <string>
#include "inferer/common.hpp"

class InferBase
{
public:
    virtual ~InferBase() = default;

    InferBase() = default;
    InferBase(const InferBase& other);

    virtual void setModel(const std::string& model_path) = 0;
    [[nodiscard]] virtual std::string getModelPath() const = 0;
    virtual void init() =0;
    virtual const std::vector<det::Binding>& getInputBinding();
    virtual const std::vector<det::Binding>& getOutputBinding();
    virtual int get_size() = 0;
    virtual std::string get_name() = 0;

    virtual void copy_from_data(const void* data) = 0;
    virtual void infer() = 0;
    virtual void infer_async(const void* input,void** output) = 0;
    virtual void warmup() = 0;
    virtual std::vector<void*>& getResult() =0;

protected:
    std::string model_path_;
    std::string device_name_;

    std::vector<det::Binding> input_bindings_;
    std::vector<det::Binding> output_bindings_;
    int num_inputs_ = 0;
    int num_outputs_ = 0;

};



#endif //INFERBASE_H
