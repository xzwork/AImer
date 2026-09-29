/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <iostream>
#include "launcher/Launcher.hpp"
#include <Windows.h>

[[noreturn]] int main(const int argc, const char **argv) {
    // CLI/diagnostic startup does not initialize GLFW, which normally sets DPI awareness.
    // Set it before any windows or DXGI calls so all entry paths use physical pixels.
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        std::cerr << "DPI awareness setup returned Windows error: " << GetLastError() << std::endl;
    Launcher::instance().init(argc, argv);
    if (!Launcher::instance().isLaunched()) {
        std::cout << "Launcher cancelled or closed. Exiting." << std::endl;
        std::exit(0);
    }

    Launcher::instance().launchAutoAim();
}
