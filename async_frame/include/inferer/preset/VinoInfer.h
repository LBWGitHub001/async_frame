//
// Created by lbw on 25-2-14.
//

#ifndef VINOINFER_H
#define VINOINFER_H

#ifdef VINO
#include "inferer/preset/InferBase.h"

class VinoInfer :
    public InferBase
{
public:
    VinoInfer() = default;
    explicit VinoInfer(const std::string& model_path, bool is_warmup = false, const std::string& device_name_ = "CPU");
    VinoInfer(const VinoInfer& other);
    ~VinoInfer() override;
    void setModel(const std::string& model_pat) override;
    void setDevice(const std::string& device_name);
    [[nodiscard]] std::string getModelPath() const override;
    [[nodiscard]] std::string getDevice() const;
    int get_size() override;
    std::string get_name() override;

    void init() override;
    void warmup() override;
    void preMalloc();
    void copy_from_data(const void* data) override;
    void copy_from_data(const void* data, const ov::Shape& shape);
    void infer() override;
    void infer_async(const void* input, void** output) override;
    std::vector<void*>& getResult() override;

    [[nodiscard]] bool get_dynamic() const { return is_dynamic_; }

private:
    std::unique_ptr<ov::Core> ov_core_;
    std::unique_ptr<ov::CompiledModel> compiled_model_;
    ov::InferRequest request_;
    std::vector<void*> outputs_;

    bool is_dynamic_ = false;
};
#endif

#endif //VINOINFER_H
