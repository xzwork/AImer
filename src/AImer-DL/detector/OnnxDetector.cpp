/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "OnnxDetector.hpp"
#include <numeric>

void OnnxDetector::initInputInfo() {
    if (m_session->GetInputCount() != 1) {
        const std::string message = "[ERROR] expected only-one-input model";
        throw std::runtime_error(message);
    }

    // Get input node name and input tensor shape
    m_inputNodeName = m_session->GetInputNameAllocated(0, m_allocator).get();

    // Get input tensor shape
    m_inputDims = m_session
            ->GetInputTypeInfo(0)
            .GetTensorTypeAndShapeInfo()
            .GetShape();

    // Check input tensor shape
    if (m_inputDims.size() != 4) {
        const std::string message = "[ERROR] expected 4d-input-tensor model";
        std::cerr << message << std::endl;
        throw std::runtime_error(message);
    }

    // Print input node name and input tensor shape
    input_h = static_cast<int>(m_inputDims[2]);
    input_w = static_cast<int>(m_inputDims[3]);
    std::cout << "input name: " << m_inputNodeName << std::endl;
    std::cout << "input format: NxCxHxW = "
            << m_inputDims[0] << "x"
            << m_inputDims[1] << "x"
            << m_inputDims[2] << "x"
            << m_inputDims[3] << std::endl;
}

void OnnxDetector::initOutputInfo() {
    if (m_session->GetOutputCount() != 1) {
        const std::string message = "[ERROR] expected only-one-output model";
        throw std::runtime_error(message);
    }

    // Get output node name
    m_outputNodeName = m_session->GetOutputNameAllocated(0, m_allocator).get();

    // Get output tensor shape
    m_outputDims = m_session
            ->GetOutputTypeInfo(0)
            .GetTensorTypeAndShapeInfo()
            .GetShape();

    // Check output tensor shape
    if (m_outputDims.size() != 3) {
        const std::string message = "[ERROR] expected 3d-output-tensor model";
        std::cerr << message << std::endl;
        throw std::runtime_error(message);
    }

    // Print output node name and output tensor shape
    output_h = static_cast<int>(m_outputDims[1]);
    output_w = static_cast<int>(m_outputDims[2]);
    std::cout << "output name: " << m_outputNodeName << std::endl;
    std::cout << "output format: NxHxW = "
            << m_outputDims[0] << "x"
            << m_outputDims[1] << "x"
            << m_outputDims[2] << std::endl;
}

OnnxDetector::OnnxDetector(const std::string &model_path,
                           const float conf_threshold,
                           const float nms_threshold)
    : Detector(conf_threshold, nms_threshold) {
    // Configure session
    m_sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    OrtSessionOptionsAppendExecutionProvider_CUDA(m_sessionOptions, 0);

    // Create environment object
    // Set logging level: Error
    // Session log identifier: onnx_detector
    m_ortEnv = Ort::Env(ORT_LOGGING_LEVEL_ERROR, "onnx_detector");
    const auto modelPath_w = std::wstring(model_path.begin(), model_path.end());
    m_session = std::make_unique<Ort::Session>(m_ortEnv, modelPath_w.c_str(), m_sessionOptions);

    // Initialize model input and output formats
    initInputInfo();
    initOutputInfo();
}

OnnxDetector::~OnnxDetector() {
    m_session->release();
    m_sessionOptions.release();
}

void OnnxDetector::infer(const cv::Mat &frame, Detection &detection) {
    // Image preprocessing
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0 / 255.0,
                                          cv::Size(input_w, input_h),
                                          cv::Scalar(0, 0, 0), true, false);

    // Input and output node names saved as arrays
    static const std::array<const char *, 1> inputNames{m_inputNodeName.c_str()};
    static const std::array<const char *, 1> outputNames{m_outputNodeName.c_str()};
    static const auto allocator_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    // Create input tensor
    const auto tensor = Ort::Value::CreateTensor<float>(
        allocator_info,
        blob.ptr<float>(), input_h * input_w * 3,
        m_inputDims.data(), m_inputDims.size());

    // Run inference
    std::vector<Ort::Value> ort_outputs = m_session
            ->Run(Ort::RunOptions{nullptr},
                  inputNames.data(), &tensor, inputNames.size(),
                  outputNames.data(), outputNames.size());

    // Receive inference results and convert to cv::Mat
    const cv::Mat output_matrix(output_h, output_w, CV_32F,
                                ort_outputs[0].GetTensorMutableData<float>());

    // Post process
    post_process(output_matrix, detection);
}
