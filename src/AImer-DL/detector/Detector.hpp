/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <opencv2/opencv.hpp>

class Detector {
public:
    struct Detection {
        std::vector<cv::Rect> boxes;
        std::vector<int> label_id;
        std::vector<float> confidences;
        std::vector<int> valid;
    };

    virtual ~Detector() = default;

    virtual void infer(const cv::Mat &img, Detection &detection) = 0;

    [[nodiscard]] int getInputWidth() const { return input_w; }
    [[nodiscard]] int getInputHeight() const { return input_h; }

protected:
    int input_w{-1};
    int input_h{-1};
    int output_w{-1};
    int output_h{-1};
    float m_conf_threshold;
    float m_nms_threshold;

    Detector(float conf_threshold, float nms_threshold);

    static void maxLoc(const cv::Mat &output_matrix, int col,
                       double &max_val, int &max_index);

    void post_process(const cv::Mat &outputMatrix, Detection &detection) const;
};
