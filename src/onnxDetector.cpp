#include "onnxDetector.hpp"
#include <numeric>

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
static void maxLoc(const cv::Mat &output_matrix, const int col,
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

void onnxDetector::init_input_info() {
    if (session->GetInputCount() != 1) {
        const std::string message = "[ERROR] expected only-one-input model";
        throw std::runtime_error(message);
    }

    // Get input node name
    input_node_name = session->GetInputNameAllocated(0, allocator).get();

    // Get input dimension information
    input_dims = session
            ->GetInputTypeInfo(0)
            .GetTensorTypeAndShapeInfo()
            .GetShape();
    input_h = static_cast<int>(input_dims[2]);
    input_w = static_cast<int>(input_dims[3]);

    // Print input node name and dimension information
    std::cout << "input name: " << input_node_name << std::endl;
    std::cout << "input format: NxCxHxW = "
            << input_dims[0] << "x" << input_dims[1] << "x"
            << input_dims[2] << "x" << input_dims[3] << std::endl;
}

void onnxDetector::init_output_info() {
    if (session->GetOutputCount() != 1) {
        const std::string message = "[ERROR] expected only-one-output model";
        throw std::runtime_error(message);
    }

    // Get output node name
    output_node_name = session->GetOutputNameAllocated(0, allocator).get();

    // Get output dimension information
    output_dims = session
            ->GetOutputTypeInfo(0)
            .GetTensorTypeAndShapeInfo()
            .GetShape();
    output_h = static_cast<int>(output_dims[1]);
    output_w = static_cast<int>(output_dims[2]);

    // Print output node name and dimension information
    std::cout << "output name: " << output_node_name << std::endl;
    std::cout << "output format : HxW = "
            << output_dims[1] << "x" << output_dims[2]
            << std::endl;
}

/**
 * @brief generate final detection results.
 * @param output_matrix The raw output matrix from the ONNX session.
 *                      Expected format: [4 + NumClasses] rows x [NumCandidates] columns.
 *                      - Rows 0-3: Bounding box parameters (cx, cy, w, h).
 *                      - Rows 4+: Confidence scores for each class.
 *                      Note: Coordinates are assumed to be in the original image scale.
 *                      Note: output_matrix must be of type CV_32F.
 * @param detection The detection object to store the final detection results.
 */
void onnxDetector::post_process(const cv::Mat &output_matrix, Detection &detection) const {
    detection.boxes.clear();
    detection.label_id.clear();
    detection.confidences.clear();

    for (int i = 0; i < output_matrix.cols; i++) {
        double score;
        int class_id;

        maxLoc(output_matrix, i, score, class_id);
        if (score < m_conf_threshold) continue;

        const float cx = output_matrix.at<float>(0, i);
        const float cy = output_matrix.at<float>(1, i);
        const float ow = output_matrix.at<float>(2, i);
        const float oh = output_matrix.at<float>(3, i);

        int x = static_cast<int>(cx - 0.5 * ow);
        int y = static_cast<int>(cy - 0.5 * oh);
        int width = static_cast<int>(ow);
        int height = static_cast<int>(oh);

        detection.boxes.emplace_back(x, y, width, height);
        detection.label_id.push_back(class_id);
        detection.confidences.push_back(static_cast<float>(score));
    }

    cv::dnn::NMSBoxes(detection.boxes, detection.confidences,
                      m_conf_threshold, m_nms_threshold, detection.valid);
}

onnxDetector::onnxDetector(const std::wstring &model_path,
                           const float conf_threshold,
                           const float nms_threshold)
    : m_conf_threshold(conf_threshold),
      m_nms_threshold(nms_threshold) {
    // Configure session
    session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    OrtSessionOptionsAppendExecutionProvider_CUDA(session_options, 0);

    // Create environment object
    // Set logging level: Error
    // Session log identifier: yolo
    env = Ort::Env(ORT_LOGGING_LEVEL_ERROR, "yolo");

    session = std::make_unique<Ort::Session>(env, model_path.c_str(), session_options);

    // Initialize model input and output formats
    init_input_info();
    init_output_info();
}

onnxDetector::~onnxDetector() {
    session->release();
    session_options.release();
}

void onnxDetector::infer(const cv::Mat &frame, Detection &detection) const {
    // Image preprocessing
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0,
                                          cv::Size(input_w, input_h),
                                          cv::Scalar(0, 0, 0), true, false);

    // Input and output node names saved as arrays
    static const std::array<const char *, 1> inputNames{input_node_name.c_str()};
    static const std::array<const char *, 1> outputNames{output_node_name.c_str()};
    static const auto allocator_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    // Create input tensor
    const auto tensor = Ort::Value::CreateTensor<float>(
        allocator_info, blob.ptr<float>(),
        input_h * input_w * 3,
        input_dims.data(), input_dims.size());

    // Run inference
    std::vector<Ort::Value> ort_outputs = session
            ->Run(Ort::RunOptions{nullptr},
                  inputNames.data(), &tensor, inputNames.size(),
                  outputNames.data(), outputNames.size());

    // Receive inference results and convert to cv::Mat
    const cv::Mat output_matrix(output_h, output_w, CV_32F,
                                ort_outputs[0].GetTensorMutableData<float>());

    // Post process
    post_process(output_matrix, detection);
}
