/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>
#include <openvino/openvino.hpp>
#include <CLI/CLI.hpp>
#include "detector/OnnxDetector.hpp"
#include "detector/OpenvinoDetector.hpp"
#include "ScreenCapture.hpp"
#include "GameSettings.hpp"
#include "AutoAim.h"

static constexpr int CAPTURE_SIZE = 640;

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start;
    int count = 0;

public:
    Timer() { start = std::chrono::high_resolution_clock::now(); }

    void finish() {
        count++;
        if (count < 100) return;
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::duration<double> >(end - start).count();
        const double fps = 1.0 / duration * count;
        std::cout << "fps: " << fps << "\n";
        start = std::chrono::high_resolution_clock::now();
        count = 0;
    }
};


struct Args {
    std::string gameName;
    std::string target;
    float sensitivity{};
    std::string modelPath;
    std::string weightsPath;
};

Args parse_args(const int argc, const char **argv) {
    CLI::App app{"AImer - AI Aim Assistant"};
    Args args;

    app.add_option("-n,--name", args.gameName, "Game name (valorant|cs2|ssjj)")
            ->required()
            ->check([](const std::string &str) {
                if (str != "valorant" && str != "cs2" && str != "ssjj")
                    return "Game name must be valorant, cs2, or ssjj";
                return "";
            });
    app.add_option("-t,--target", args.target,
                   "Target name (e.g. head, enemy, ct, t)")->required();
    app.add_option("-s,--sensitivity", args.sensitivity,
                   "Mouse sensitivity")->required();
    app.add_option("-m,--model", args.modelPath,
                   "Model path (.xml/.onnx)")->required()->check(CLI::ExistingFile);
    app.add_option("-w,--weights", args.weightsPath,
                   "Weights path (.bin), use OpenVINO when provided")->check(CLI::ExistingFile);
    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &) {
        std::cout << app.help() << std::endl;
        std::exit(0);
    }

    return args;
}

[[noreturn]] int main(const int argc, const char **argv) {
    try {
        const auto [
            gameName,
            target,
            sensitivity,
            modelPath,
            weightsPath] = parse_args(argc, argv);

        // Detector setup
        Detector::Detection detection;
        std::unique_ptr<Detector> detector;
        if (weightsPath.empty()) {
            detector = std::make_unique<OnnxDetector>(modelPath);
        } else {
            detector = std::make_unique<OpenvinoDetector>
                    (modelPath, weightsPath, "CPU");
        }

        // Game settings
        const GameSettings settings = GameSettings::getSettings(gameName);

        // Screen capture
        cv::Mat frame;
        ScreenCapture &capture = ScreenCapture::getInstance(640, 640);

        // Main loop
        Timer timer;
        while (true) {
            if (!capture.CaptureFrame(frame)) continue;

            detector->infer(frame, detection);
            autoAim(detection, CAPTURE_SIZE / 2, CAPTURE_SIZE / 2, settings, target, sensitivity);

            timer.finish();
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
