#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

struct Detection {
    std::vector<cv::Rect> boxes;
    std::vector<int> label_id;
    std::vector<float> confidences;

    std::vector<int> valid;
};

class onnxDetector {
    Ort::SessionOptions session_options;
    Ort::Env env;

    std::unique_ptr<Ort::Session> session;

    Ort::AllocatorWithDefaultOptions allocator;

    int input_w{-1};
    int input_h{-1};
    int output_w{-1};
    int output_h{-1};
    std::string input_node_name;
    std::string output_node_name;
    std::vector<int64_t> input_dims;
    std::vector<int64_t> output_dims;

    float m_conf_threshold;
    float m_nms_threshold;

    void init_input_info();

    void init_output_info();

    void post_process(const cv::Mat &output_matrix, Detection &detection) const;

public :
    explicit onnxDetector(const std::wstring &model_path,
                          float conf_threshold,
                          float nms_threshold);

    ~onnxDetector();

    void infer(const cv::Mat &frame, Detection &detection) const;

    [[nodiscard]] int getInputWidth() const { return input_w; }
    [[nodiscard]] int getInputHeight() const { return input_h; }
};
