/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "Detector.hpp"

Detector::Detector(const float conf_threshold,
                   const float nms_threshold)
    : m_conf_threshold(conf_threshold),
      m_nms_threshold(nms_threshold) {
}

/**
 * @brief Finds the class with the highest confidence score for a specific detection candidate.
 *
 * you can also use cv::minMaxLoc() instead.
 * but it's just too damn slow.
 *
 * The detection output matrix is expected to have the following structure per column:
 * - Row 0: Center X (cx)
 * - Row 1: Center Y (cy)
 * - Row 2: Width (w)
 * - Row 3: Height (h)
 * - Row 4 to (4 + class_tot - 1): Confidence scores for each class.
 *
 * @param output_matrix The output matrix from the ONNX model (Format: [NumClasses+4] x [NumCandidates]).
 * @param col The column index representing the specific candidate bounding box to analyze.
 * @param max_val Output parameter: Stores the highest confidence score found.
 * @param max_index Output parameter: Stores the class index (0-based) corresponding to the highest score.
 *                  Initialized to -1 if no valid score is found.
 */
void Detector::maxLoc(const cv::Mat &output_matrix, const int col,
                      double &max_val, int &max_index) {
    max_val = 0.0f;
    max_index = -1;

    for (int row = 4; row < output_matrix.rows; row++) {
        const float val = output_matrix.at<float>(row, col);
        if (val < max_val) continue;
        max_val = val;
        max_index = row - 4;
    }
}

/**
 * @brief generate final detection results.
 * @param outputMatrix The raw output matrix from the ONNX session.
 *                      Expected format: [4 + NumClasses] rows x [NumCandidates] columns.
 *                      - Rows 0-3: Bounding box parameters (cx, cy, w, h).
 *                      - Rows 4+: Confidence scores for each class.
 *                      Note: Coordinates are assumed to be in the original image scale.
 *                      Note: outputMatrix must be of type CV_32F.
 * @param detection The detection object to store the final detection results.
 */
void Detector::post_process(const cv::Mat &outputMatrix, Detection &detection) const {
    detection.boxes.clear();
    detection.label_id.clear();
    detection.confidences.clear();

    for (int i = 0; i < outputMatrix.cols; i++) {
        double score;
        int class_id;

        maxLoc(outputMatrix, i, score, class_id);
        if (score < m_conf_threshold) continue;

        const float cx = outputMatrix.at<float>(0, i);
        const float cy = outputMatrix.at<float>(1, i);
        const float ow = outputMatrix.at<float>(2, i);
        const float oh = outputMatrix.at<float>(3, i);

        int x = static_cast<int>(cx - 0.5 * ow);
        int y = static_cast<int>(cy - 0.5 * oh);
        int width = static_cast<int>(ow);
        int height = static_cast<int>(oh);

        detection.boxes.emplace_back(x, y, width, height);
        detection.label_id.push_back(class_id);
        detection.confidences.push_back(static_cast<float>(score));
    }

    cv::dnn::NMSBoxes(detection.boxes,
                      detection.confidences,
                      m_conf_threshold,
                      m_nms_threshold,
                      detection.valid);
}
