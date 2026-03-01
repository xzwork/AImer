#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include "ScreenCapture.hpp"
#include "onnxDetector.hpp"
#include "AutoAim.h"

static constexpr int CAPTURE_SIZE = 640;

enum target { head = 0, enemy = 1 };

void process_args(const int argc, char **argv,
                  std::wstring &model_path,
                  float &sensitivity) {
    if (argc <= 2) throw std::runtime_error("Usage: [model] [sensitivity]");
    model_path = std::wstring(argv[1], argv[1] + strlen(argv[1]));
    sensitivity = std::stof(argv[2]);
}


class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    int count = 0;

public:
    Timer() { start = std::chrono::high_resolution_clock::now(); }

    void finish() {
        count++;
        if (count < 50) return;
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::duration<double> >(end - start).count();
        const double fps = 1.0 / duration * count;
        std::cout << "fps: " << fps << "\n";
        start = std::chrono::high_resolution_clock::now();
        count = 0;
    }
};

[[noreturn]] int main(int argc, char **argv) {
    std::wstring model;
    float sensitivity;
    process_args(argc, argv, model, sensitivity);

    //  onnx runtime detector
    Detection detection;
    onnxDetector detector(model, 0.4, 0.4);

    // screen capture
    cv::Mat frame;
    ScreenCapture &capture = ScreenCapture::getInstance(detector.getInputWidth(), detector.getInputHeight());

    Timer timer;

    // capture.CaptureFrame(frame);
    while (true) {
        if (!capture.CaptureFrame(frame)) continue;
        detector.infer(frame, detection);
        auto_aim(detection, CAPTURE_SIZE / 2, CAPTURE_SIZE / 2, head, sensitivity);
        timer.finish();
    }
}
