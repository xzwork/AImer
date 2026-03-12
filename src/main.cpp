#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

#include "ScreenCapture.hpp"
#include "onnxDetector.hpp"
#include "GameSettings.hpp"
#include "AutoAim.h"

static constexpr int CAPTURE_SIZE = 640;

void process_args(const int argc, char **argv,
                  GameSettings &settings,
                  int &target,
                  float &sensitivity,
                  std::wstring &model_path) {
    if (argc < 5) throw std::invalid_argument("Invalid arguments");

    settings = GameSettings::getSettings(std::stoi(argv[1]));
    target = std::stoi(argv[2]);
    sensitivity = std::stof(argv[3]);
    model_path = std::wstring(argv[4], argv[4] + strlen(argv[4]));
}


class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    int count = 0;

public:
    Timer() { start = std::chrono::high_resolution_clock::now(); }

    void finish() {
        count++;
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::duration<double> >(end - start).count();
        const double fps = 1.0 / duration * count;
        std::cout << "fps: " << fps << "\n";
        start = std::chrono::high_resolution_clock::now();
        count = 0;
    }
};

[[noreturn]] int main(int argc, char **argv) {
    // arguments
    int target;
    float sensitivity;
    std::wstring model;
    GameSettings settings{};
    process_args(argc, argv, settings, target, sensitivity, model);

    //  onnx runtime detector
    Detection detection;
    onnxDetector detector(model, 0.4, 0.4);

    // screen capture
    cv::Mat frame;
    ScreenCapture &capture = ScreenCapture::getInstance(detector.getInputWidth(), detector.getInputHeight());

    Timer timer;
    while (true) {
        if (!capture.CaptureFrame(frame)) continue;
        detector.infer(frame, detection);
        autoAim(detection, CAPTURE_SIZE / 2, CAPTURE_SIZE / 2, settings, target, sensitivity);
        timer.finish();
    }
}
