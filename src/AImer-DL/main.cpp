/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <iostream>
#include "launcher/Launcher.hpp"

[[noreturn]] int main(const int argc, const char **argv) {
    Launcher::instance().init(argc, argv);
    if (!Launcher::instance().isLaunched()) {
        std::cout << "Launcher cancelled or closed. Exiting." << std::endl;
        std::exit(0);
    }

    Launcher::instance().launchAutoAim();
}