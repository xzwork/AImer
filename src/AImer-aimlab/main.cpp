#include <iostream>
#include <Windows.h>
#include <string>

#include <CLI/CLI.hpp>

#include "capture/ScreenCapture.hpp"
#include "controller/MouseController.hpp"
#include "autoAim.h"

float GAME_SENSITIVITY = 0.1f;

static void parseArgs(int argc, char **argv) {
    CLI::App app{"AImer-aimlab - AI Aim Assistant for Aim Lab"};
    app.add_option("-s,--sensitivity", GAME_SENSITIVITY, "Mouse sensitivity")
            ->default_val(0.1f)
            ->check(CLI::PositiveNumber);
    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &) {
        std::cout << app.help() << std::endl;
        std::exit(0);
    } catch (const CLI::ParseError &e) {
        std::exit(app.exit(e));
    }
}

int main(int argc, char **argv) {
    parseArgs(argc, argv);

    std::cout << "AImer-aimlab starting..." << std::endl;
    std::cout << "  Sensitivity: " << GAME_SENSITIVITY << std::endl;

    const int screen_width = GetSystemMetrics(SM_CXSCREEN);
    const int screen_height = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "Screen: " << screen_width << "x" << screen_height << std::endl;

    auto &capture = ScreenCapture::getInstance(screen_width, screen_height);
    const MouseController &mouse = MouseController::getInstance();

    const AimlabSettings settings = AIMLAB_VALORANT;

    cv::Mat frame;
    std::cout << "Press F5 to exit." << std::endl;

    while (true) {
        if (GetAsyncKeyState(VK_F5) & 0x8000) break;
        if (!capture.CaptureFrame(frame)) continue;

        auto offset = autoAim(frame);

        if (!offset.has_value()) continue;

        autoAim(offset.value(), mouse, settings, GAME_SENSITIVITY);
    }

    std::cout << "AImer-aimlab exited." << std::endl;
    return 0;
}