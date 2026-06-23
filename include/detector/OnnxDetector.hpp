/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include "Detector.hpp"

class OnnxDetector final : public Detector {
private:
    Ort::Env m_ortEnv;
    Ort::SessionOptions m_sessionOptions;
    std::unique_ptr<Ort::Session> m_session;
    Ort::AllocatorWithDefaultOptions m_allocator;

    std::string m_inputNodeName;
    std::string m_outputNodeName;
    std::vector<int64_t> m_inputDims;
    std::vector<int64_t> m_outputDims;

    void initInputInfo();

    void initOutputInfo();

public:
    explicit OnnxDetector(const std::string &model_path,
                          float conf_threshold = 0.6f,
                          float nms_threshold = 0.4f);

    ~OnnxDetector() override;

    void infer(const cv::Mat &frame, Detection &detection) override;
};
