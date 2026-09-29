/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <chrono>
#include <fstream>
#include <array>
#include <limits>

#include <CLI/CLI.hpp>

#include "../Games.hpp"
#include "Launcher.hpp"
#include "../detector/OnnxDetector.hpp"
#ifdef AIMER_WITH_OPENVINO
#include "../detector/OpenvinoDetector.hpp"
#endif
#include "controller/MouseController.hpp"
#include "capture/ScreenCapture.hpp"
#include "../AutoAim.h"

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

Launcher::Launcher()
    : args(), m_window(nullptr),
      m_scale(0),
      selectedGame(0),
      selectedTarget(0),
      shouldCancel(false),
      shouldLaunch(false),
      statusIsError(false) {
}

std::string Launcher::trim(std::string_view str) {
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front()))) str.remove_prefix(1);
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) str.remove_suffix(1);
    return std::string(str);
}

void Launcher::saveConfig() const {
    std::ofstream file((args.exeDir / "launcher_config.txt").string());
    if (!file.is_open()) return;
    file << "GameName=" << (selectedGame < static_cast<int>(m_allGameNames.size()) ? m_allGameNames[selectedGame] : "")
            << "\n"
            << "TargetName=" << (isTargetIndexValid() ? getCurrentGameTargets()[selectedTarget] : "") << "\n"
            << "Sensitivity=" << sensitivityStr << "\n"
            << "ModelPath=" << modelPath << "\n"
            << "WeightsPath=" << weightsPath << "\n";
}

void Launcher::loadConfig() {
    std::ifstream file((args.exeDir / "launcher_config.txt").string());
    if (!file.is_open()) return;

    std::unordered_map<std::string, std::string> kv;
    for (std::string line; std::getline(file, line);) {
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }

    if (kv.contains("GameName")) {
        const auto &gameName = kv["GameName"];
        selectedGame = 0;
        for (int i = 0; i < static_cast<int>(m_allGameNames.size()); ++i) {
            if (m_allGameNames[i] != gameName) continue;
            selectedGame = i;
            break;
        }
    }

    if (kv.contains("Sensitivity")) sensitivityStr = kv["Sensitivity"];
    if (kv.contains("ModelPath")) modelPath = kv["ModelPath"];
    if (kv.contains("WeightsPath")) weightsPath = kv["WeightsPath"];

    if (kv.contains("TargetName")) {
        const auto &targetName = kv["TargetName"];
        const auto targets = getCurrentGameTargets();
        selectedTarget = 0;
        for (int i = 0; i < static_cast<int>(targets.size()); ++i) {
            if (targets[i] != targetName) continue;
            selectedTarget = i;
            break;
        }
    }
}

bool Launcher::validateAndLaunch() {
    if (selectedGame < 0 || selectedGame >= static_cast<int>(m_allGameNames.size()))
        return updateStatus("Please select a game", true), false;
    if (!isTargetIndexValid())
        return updateStatus("Please select a target", true), false;
    if (sensitivityStr.empty())
        return updateStatus("Please enter sensitivity", true), false;

    float sensitivity;
    try {
        size_t pos = 0;
        sensitivity = std::stof(sensitivityStr, &pos);
        if (pos != sensitivityStr.size() || !std::isfinite(sensitivity) || sensitivity <= 0.0f)
            return updateStatus("Sensitivity must be a positive number", true), false;
    } catch (...) { return updateStatus("Invalid sensitivity format", true), false; }

    if (modelPath.empty())
        return updateStatus("Please enter model path", true), false;
    if (!std::filesystem::exists(modelPath))
        return updateStatus("Model file does not exist: " + modelPath, true), false;
    if (!weightsPath.empty() && !std::filesystem::exists(weightsPath))
        return updateStatus("Weights file does not exist: " + weightsPath, true), false;
#ifndef AIMER_WITH_OPENVINO
    if (!weightsPath.empty())
        return updateStatus("OpenVINO is disabled in this build; select an ONNX model", true), false;
#endif
    if (getCurrentGameName() == "apex" &&
        (!weightsPath.empty() || std::filesystem::path(modelPath).extension() != ".onnx"))
        return updateStatus("Apex requires an ONNX model with CUDA", true), false;

    args.gameName = getCurrentGameName();
    args.target = getCurrentGameTargets()[selectedTarget];
    args.sensitivity = sensitivity;
    args.modelPath = modelPath;
    args.weightsPath = weightsPath;

    saveConfig();
    shouldLaunch = true;
    updateStatus("Launching...", false);
    return true;
}

void Launcher::installCallbacks() const {
    core::installInputCallbacks(m_window);
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow *, int, int) { app::detail::requestFullPaint(); });
    glfwSetWindowRefreshCallback(m_window, [](GLFWwindow *) { app::detail::requestFullPaint(); });
    glfwSetWindowContentScaleCallback(m_window, [](GLFWwindow *, float, float) { app::detail::requestFullPaint(); });
}

void Launcher::firstSetWindowSize() {
    const auto w = static_cast<float>(app::initialWindowWidth());
    const auto h = static_cast<float>(app::initialWindowHeight());
    m_scale = getDpiScale();
    glfwSetWindowSize(m_window,
                      static_cast<int>(std::round(w * m_scale)),
                      static_cast<int>(std::round(h * m_scale)));
}

void Launcher::handleDpiScaleChange() {
    const float curScale = getDpiScale();
    if (std::abs(curScale - m_scale) < 0.001f) return;
    int w = 0, h = 0;
    glfwGetWindowSize(m_window, &w, &h);
    glfwSetWindowSize(m_window,
                      static_cast<int>(static_cast<float>(w) * curScale / m_scale),
                      static_cast<int>(static_cast<float>(h) * curScale / m_scale));
    m_scale = curScale;
}

void Launcher::throttleAnimationFrame() const {
    if (m_appRunner.anyAnimating(false)) {
        const double remaining = m_appRunner.nextFrameTime - glfwGetTime();
        app::detail::waitForFrameDuration(remaining);
    }
}

void Launcher::render() const {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(m_window, &framebufferWidth, &framebufferHeight);
    const float frameContentScale = getDpiScale();
    const float pointerScale = getPointerScale();
    const app::MainWindowMetrics metric(framebufferWidth,
                                        framebufferHeight,
                                        frameContentScale,
                                        pointerScale);

    constexpr bool inputEnabled = true;

    auto afterUpdate = [] {
    };
    auto updateChildren = [](float, bool) {
    };
    auto setTitle = [&](const char *title) { glfwSetWindowTitle(m_window, title); };
    auto childAnimating = [] { return false; };

    m_mainWindowRuntime->runFrame(
        m_window,
        *m_renderBackend,
        metric,
        glfwGetTime(),
        60,
        inputEnabled,
        afterUpdate,
        updateChildren,
        setTitle,
        childAnimating
    );
}

void Launcher::cleanup() {
    core::releaseInputQueue(m_window);
    m_renderBackend->makeCurrent();
    m_renderBackend->releaseRenderCache(); {
        core::render::ScopedRenderBackend scopedRenderBackend(*m_renderBackend);
        app::shutdown();
    }
    m_renderBackend.reset();
    glfwTerminate();
}

void Launcher::launchFromArgs(const int argc, const char **argv) {
    CLI::App app{"AImer - AI Aim Assistant"};
    app.add_flag("--diagnostics", args.diagnostics, "Preview detections and log counts without mouse input");
    app.add_option("-n,--name", args.gameName, "Game name")
            ->required()
            ->check([&](const std::string &str) {
                for (const auto &n: m_allGameNames) if (n == str) return std::string{};
                return "Unknown game: " + str;
            });
    app.add_option("-t,--target", args.target, "Target name (Apex always uses enemy/class 0)");
    auto *sensitivityOption = app.add_option("-s,--sensitivity", args.sensitivity, "Mouse sensitivity");
    app.add_option("-m,--model", args.modelPath, "Model path (.xml/.onnx)")
            ->required()->check(CLI::ExistingFile);
    app.add_option("-w,--weights", args.weightsPath, "Weights path (.bin), use OpenVINO when provided")
            ->check(CLI::ExistingFile);
    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &) {
        std::cout << app.help() << std::endl;
        std::exit(0);
    }
    const auto &settings = Games::instance().getSettings(args.gameName);
    if (sensitivityOption->count() == 0) args.sensitivity = settings.sensitivity;
    if (!std::isfinite(args.sensitivity) || args.sensitivity <= 0)
        throw std::invalid_argument("Sensitivity must be a finite positive number");
    if (settings.name == "apex") args.target = "enemy";
    else if (std::find(settings.targets.begin(), settings.targets.end(), args.target) == settings.targets.end())
        throw std::invalid_argument("Select a valid target for " + args.gameName);
    shouldLaunch = true;
}

void Launcher::launchFromGUI() {
    shouldLaunch = false;
    shouldCancel = false;

    loadConfig();

    core::window::WindowCreateRequest windowRequest;
    windowRequest.width = app::initialWindowWidth();
    windowRequest.height = app::initialWindowHeight();
    windowRequest.title = app::windowTitle();
    windowRequest.renderApi = core::render::windowRenderApi();

    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    m_window = static_cast<GLFWwindow *>(core::window::createWindow(windowRequest));

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }
    m_renderBackend = core::render::createRenderBackend(m_window);
    if (!m_renderBackend->initialize()) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize render backend");
    }

    m_appRunner.resetTiming(glfwGetTime());
    m_mainWindowRuntime = std::make_unique<app::MainWindowRuntime>(m_appRunner);

    firstSetWindowSize();

    installCallbacks();

    while (!windowShouldClose() && !shouldLaunch && !shouldCancel) {
        handleDpiScaleChange();
        throttleAnimationFrame();
        render();
        isAnimating() ? glfwPollEvents() : glfwWaitEvents();
    }
    cleanup();
}

float Launcher::getDpiScale() const {
    float x_scale = 1.0f, y_scale = 1.0f;
    glfwGetWindowContentScale(m_window, &x_scale, &y_scale);
    return (x_scale + y_scale) * 0.5f;
}

float Launcher::getPointerScale() const {
    int frameWidth = 0, frameHeight = 0;
    int windowWidth = 0, windowHeight = 0;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(m_window, &frameWidth, &frameHeight);

    const float x_scale = static_cast<float>(frameWidth) / static_cast<float>(windowWidth);
    const float y_scale = static_cast<float>(frameHeight) / static_cast<float>(windowHeight);

    return (x_scale + y_scale) * 0.5f;
}

Launcher &Launcher::instance() {
    static Launcher instance;
    return instance;
}

void Launcher::init(const int argc, const char **argv) {
    namespace fs = std::filesystem;

    if (argc > 0 && argv && argv[0]) {
        std::error_code ec;
        const auto p = fs::absolute(fs::path(argv[0]), ec);
        if (!ec) args.exeDir = p.parent_path();
    }
    if (args.exeDir.empty()) {
        std::error_code ec;
        args.exeDir = fs::current_path(ec);
    }

    Games::instance().loadFromFile(args.exeDir / "games.yaml");
    m_allGameNames = Games::instance().gameNames();
    if (m_allGameNames.empty()) throw std::runtime_error("No games configured");
    sensitivityStr = std::to_string(Games::instance().getSettings(m_allGameNames.front()).sensitivity);

    if (argc > 1) {
        launchFromArgs(argc, argv);
    } else launchFromGUI();
}

void Launcher::updateStatus(const std::string &msg, const bool isError) {
    statusMsg = msg;
    statusIsError = isError;
}

[[noreturn]] void Launcher::launchAutoAim() {
    if (args.gameName == "apex" &&
        (!args.weightsPath.empty() || std::filesystem::path(args.modelPath).extension() != ".onnx"))
        throw std::invalid_argument("Apex requires an ONNX model with CUDA");
    Detector::Detection detection;
    std::unique_ptr<Detector> detector;
    if (args.weightsPath.empty()) {
        detector = std::make_unique<OnnxDetector>(args.modelPath);
    } else {
#ifdef AIMER_WITH_OPENVINO
        detector = std::make_unique<OpenvinoDetector>
                (args.modelPath, args.weightsPath, "CPU");
#else
        throw std::runtime_error("OpenVINO is disabled; rebuild with AIMER_WITH_OPENVINO=ON");
#endif
    }

    const GameSettings settings = Games::instance().getSettings(args.gameName);

    cv::Mat frame;
    ScreenCapture &capture = ScreenCapture::getInstance(detector->getInputWidth(), detector->getInputHeight());
    if (!args.diagnostics) MouseController::getInstance();

    std::cout << "AImer starting..." << std::endl;
    std::cout << "  Game:        " << args.gameName << std::endl;
    std::cout << "  Target:      " << args.target << std::endl;
    std::cout << "  Sensitivity: " << args.sensitivity << std::endl;
    std::cout << "  Model:       " << args.modelPath << std::endl;
    std::cout << "  Capture:     " << detector->getInputWidth() << "x" << detector->getInputHeight() << std::endl;
    if (settings.name == "apex")
        std::cout << "  Aim radius:  " << settings.aim_fov << " px; FOV (4:3): " << settings.fov << std::endl;
    if (!args.weightsPath.empty())
        std::cout << "  Weights:     " << args.weightsPath << std::endl;

    const std::string previewName = "AImer diagnostics - NO MOUSE - Q/Esc: quit, S: save";
    std::ofstream diagnosticLog;
    if (args.diagnostics) {
        diagnosticLog.open(args.exeDir / "diagnostics.log", std::ios::trunc);
        if (!diagnosticLog) throw std::runtime_error("Cannot write diagnostics.log beside AImer.exe");
        std::cout << "Diagnostics ON: mouse input disabled. Confidence threshold: 0.6. Log: "
                  << (args.exeDir / "diagnostics.log") << std::endl;
        diagnosticLog << "Model=" << args.modelPath << " screen=" << capture.getWidth() << 'x' << capture.getHeight()
                      << " capture=" << detector->getInputWidth() << 'x' << detector->getInputHeight()
                      << " confidence=0.6 aim_radius=" << settings.aim_fov << " mouse=DISABLED\n";
        cv::namedWindow(previewName, cv::WINDOW_NORMAL);
        cv::resizeWindow(previewName, 640, 640);
        cv::moveWindow(previewName, 0, 0);
    }
    auto lastDiagnostic = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    auto lastFrame = std::chrono::steady_clock::now();
    auto lastCaptureWarning = lastFrame;
    Timer timer;
    while (true) {
        if (!capture.CaptureFrame(frame)) {
            if (args.diagnostics) {
                const auto now = std::chrono::steady_clock::now();
                if (now - lastFrame >= std::chrono::seconds(2) && now - lastCaptureWarning >= std::chrono::seconds(2)) {
                    std::cout << "[diag] No new DXGI frame for 2 seconds" << std::endl;
                    diagnosticLog << "[diag] No new DXGI frame for 2 seconds\n" << std::flush;
                    lastCaptureWarning = now;
                }
                const int key = cv::waitKey(1) & 0xff;
                if (key == 27 || key == 'q' || cv::getWindowProperty(previewName, cv::WND_PROP_VISIBLE) < 1) {
                    diagnosticLog.close();
                    cv::destroyAllWindows();
                    std::exit(0);
                }
            }
            continue;
        }
        lastFrame = std::chrono::steady_clock::now();

        detector->infer(frame, detection);
        if (args.diagnostics) {
            cv::Mat preview = frame.clone();
            const cv::Point center(frame.cols / 2, frame.rows / 2);
            std::array<int, 4> counts{};
            int inside = 0;
            float bestEnemyScore = 0.0f;
            float nearest = (std::numeric_limits<float>::max)();
            for (const int id : detection.valid) {
                const int label = detection.label_id[id];
                if (label >= 0 && label < static_cast<int>(counts.size())) ++counts[label];
                const auto &box = detection.boxes[id];
                const float distance = std::hypot(box.x + box.width * 0.5f - center.x,
                                                  box.y + box.height * 0.5f - center.y);
                const bool inFov = label == 0 && box.width > 0 && box.height > 0 && distance <= settings.aim_fov;
                if (label == 0) {
                    bestEnemyScore = (std::max)(bestEnemyScore, detection.confidences[id]);
                    nearest = (std::min)(nearest, distance);
                    if (inFov) ++inside;
                }
                const cv::Scalar color = inFov ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255);
                cv::rectangle(preview, box, color, 2);
                cv::putText(preview, cv::format("class %d %.2f", label, detection.confidences[id]),
                            cv::Point((std::max)(0, box.x), (std::max)(18, box.y)),
                            cv::FONT_HERSHEY_SIMPLEX, 0.5, color, 1);
            }
            cv::circle(preview, center, static_cast<int>(settings.aim_fov), cv::Scalar(255, 255, 0), 1);
            cv::drawMarker(preview, center, cv::Scalar(255, 255, 255), cv::MARKER_CROSS, 16, 1);
            const std::string summary = cv::format("c0=%d c1=%d c2=%d c3=%d inFov=%d", counts[0], counts[1], counts[2], counts[3], inside);
            cv::putText(preview, summary, cv::Point(8, 22), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
            cv::putText(preview, "NO MOUSE INPUT", cv::Point(8, 44), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1);
            cv::imshow(previewName, preview);
            if (lastFrame - lastDiagnostic >= std::chrono::seconds(1)) {
                const auto line = "[diag] " + summary + cv::format(" preNms=%d maxEnemy=%.3f nearest=%.1f mouse=DISABLED",
                    static_cast<int>(detection.boxes.size()), bestEnemyScore, counts[0] ? nearest : -1.0f);
                std::cout << line << std::endl;
                diagnosticLog << line << '\n' << std::flush;
                lastDiagnostic = lastFrame;
            }
            const int key = cv::waitKey(1) & 0xff;
            if (key == 's') {
                cv::imwrite((args.exeDir / "diagnostic-capture.png").string(), frame);
                cv::imwrite((args.exeDir / "diagnostic-preview.png").string(), preview);
                std::cout << "Saved diagnostic-capture.png and diagnostic-preview.png" << std::endl;
            }
            if (key == 27 || key == 'q' || cv::getWindowProperty(previewName, cv::WND_PROP_VISIBLE) < 1) {
                diagnosticLog.close();
                cv::destroyAllWindows();
                std::exit(0);
            }
        } else {
            autoAim(detection, frame.cols / 2, frame.rows / 2, settings, args.target, args.sensitivity,
                    capture.getWidth(), capture.getHeight());
        }

        timer.finish();
    }
}

bool Launcher::isLaunched() const { return shouldLaunch; }

const std::vector<std::string> &Launcher::getGames() const { return m_allGameNames; }

std::string Launcher::getCurrentGameName() const { return m_allGameNames[selectedGame]; }

const std::vector<std::string> &Launcher::getCurrentGameTargets() const {
    return Games::instance().getSettings(m_allGameNames[selectedGame]).targets;
}

bool Launcher::isTargetIndexValid() const {
    const auto targets = getCurrentGameTargets();
    return selectedTarget >= 0 && selectedTarget < static_cast<int>(targets.size());
}

bool Launcher::isAnimating() const { return m_appRunner.anyAnimating(false); }

bool Launcher::windowShouldClose() const { return glfwWindowShouldClose(m_window); }
