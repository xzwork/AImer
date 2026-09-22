/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "OpenvinoDetector.hpp"
#include <opencv2/core.hpp>

OpenvinoDetector::OpenvinoDetector(const std::string &model_path_xml,
                                   const std::string &model_path_bin,
                                   const std::string &device,
                                   const float conf_threshold,
                                   const float nms_threshold)
    : Detector(conf_threshold, nms_threshold) {
    // Create OpenVINO Core
    ov::Core core;
    auto model = core.read_model(model_path_xml, model_path_bin);
    m_processor = std::make_unique<ov::preprocess::PrePostProcessor>(model); // Preprocessor

    // Set input tensor
    m_processor->input()
            .tensor()
            .set_layout("NHWC")
            .set_element_type(ov::element::u8)
            .set_color_format(ov::preprocess::ColorFormat::BGR);

    // Set input preprocess
    m_processor->input()
            .preprocess()
            .convert_layout("NCHW")
            .convert_element_type(ov::element::f32)
            .convert_color(ov::preprocess::ColorFormat::RGB)
            .scale({255., 255., 255.});

    // Set output tensor
    m_processor->output()
            .tensor()
            .set_element_type(ov::element::f32);

    // Build model
    model = m_processor->build();
    m_compiled = core.compile_model(model, device);
    m_inferRequest = m_compiled.create_infer_request();


    // Check input and output tensor shape
    const auto input_shape = m_compiled.input().get_shape();
    if (input_shape.size() != 4) {
        const std::string message = "[ERROR] expected 4d-input-tensor model";
        throw std::runtime_error(message);
    }

    const auto output_shape = m_compiled.output().get_shape();
    if (output_shape.size() != 3) {
        const std::string message = "[ERROR] expected 3d-output-tensor model";
        throw std::runtime_error(message);
    }

    // Print input and output tensor shape
    input_h = static_cast<int>(input_shape[1]);
    input_w = static_cast<int>(input_shape[2]);
    output_h = static_cast<int>(output_shape[1]);
    output_w = static_cast<int>(output_shape[2]);
    std::cout << "input format: NxCxHxW = "
            << input_shape[0] << "x"
            << input_shape[1] << "x"
            << input_shape[2] << "x"
            << input_shape[3] << std::endl;
    std::cout << "output format: NxHxW = "
            << output_shape[0] << "x"
            << output_shape[1] << "x"
            << output_shape[2] << std::endl;
}

void OpenvinoDetector::infer(const cv::Mat &img, Detection &detection) {
    //Create input tensor
    const auto input_tensor = ov::Tensor(m_compiled.input().get_element_type(),
                                         m_compiled.input().get_shape(), img.data);
    m_inferRequest.set_input_tensor(input_tensor);

    // Run inference
    m_inferRequest.infer();

    // Receive inference results and convert to cv::Mat
    ov::Tensor outTensor = m_inferRequest.get_output_tensor(0);
    const cv::Mat outMatrix(output_h, output_w, CV_32F, outTensor.data());

    // Post process
    post_process(outMatrix, detection);
}
