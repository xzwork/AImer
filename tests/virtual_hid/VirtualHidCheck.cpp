#include "../../src/controller/MouseController.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--controller-noop") {
            // Validates configuration and missing-driver handling without moving/clicking.
            MouseController::getInstance().MoveRelative(0, 0);
            return 0;
        }
        if (argc != 1 && !(argc == 4 && std::string(argv[1]) == "--move")) {
            std::cerr << "Usage: VirtualHidCheck [--move dx dy | --controller-noop]\n";
            return 1;
        }
        int dx = 0, dy = 0;
        if (argc == 4) {
            size_t xUsed = 0, yUsed = 0;
            dx = std::stoi(argv[2], &xUsed);
            dy = std::stoi(argv[3], &yUsed);
            if (xUsed != std::string(argv[2]).size() || yUsed != std::string(argv[3]).size())
                throw std::invalid_argument("dx/dy must be signed integers");
        }
        VirtualHidBackend backend;
        if (!backend.Open()) return 2;
        if (argc == 4) {
            DWORD error = 0;
            if (!backend.MoveRelative(dx, dy, error)) {
                std::cerr << "DeviceIoControl MOVE_RELATIVE failed; win32_error=" << error
                          << "; some earlier chunks may already have moved\n";
                return 3;
            }
        }
        std::cout << "Virtual HID check OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
