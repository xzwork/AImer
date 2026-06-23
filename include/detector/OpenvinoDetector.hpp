/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>
#include "Detector.hpp"

class OpenvinoDetector final : public Detector {
private:
    std::unique_ptr<ov::preprocess::PrePostProcessor> m_processor;
    ov::CompiledModel m_compiled;
    ov::InferRequest m_inferRequest;

public:
    OpenvinoDetector(const std::string &model_path_xml,
                     const std::string &model_path_bin,
                     const std::string &device,
                     float conf_threshold = 0.6f,
                     float nms_threshold = 0.4f);

    ~OpenvinoDetector() override = default;

    void infer(const cv::Mat &img, Detection &detection) override;
};
