/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "MouseController.hpp"
#include "InputSimulator.hpp"
#include <Windows.h>
#include <iostream>
#include <map>
#include <string>
#include <vector>

MouseController &MouseController::getInstance() {
    static MouseController instance;
    return instance;
}

MouseController::MouseController() {
    if (!loadDll()) throw std::runtime_error("failed to load IbInputSimulator.dll");
    if (!selectDriver()) throw std::runtime_error("failed to find available driver");
}

MouseController::~MouseController() {
    if (!hMouseDll) return;
    if (IbSendDestroy_ptr) IbSendDestroy_ptr();
    FreeLibrary(hMouseDll);
}

void MouseController::MoveRelative(const int dx, const int dy) const {
    IbSendMouseMove_ptr(dx, dy, Send::MoveMode::Relative);
}


void MouseController::fire() const {
    IbSendMouseClick_ptr(Send::MouseButton::Left);
}


bool MouseController::loadDll() {
    hMouseDll = LoadLibrary("IbInputSimulator.dll");
    if (!hMouseDll) return false;

    IbSendInit_ptr = reinterpret_cast<pIbSendInit>(GetProcAddress(hMouseDll, "IbSendInit"));
    IbSendDestroy_ptr = reinterpret_cast<pIbSendDestroy>(GetProcAddress(hMouseDll, "IbSendDestroy"));
    IbSendMouseMove_ptr = reinterpret_cast<pIbSendMouseMove>(GetProcAddress(hMouseDll, "IbSendMouseMove"));
    IbSendMouseClick_ptr = reinterpret_cast<pIbSendMouseClick>(GetProcAddress(hMouseDll, "IbSendMouseClick"));
    if (!IbSendInit_ptr || !IbSendDestroy_ptr || !IbSendMouseMove_ptr || !IbSendMouseClick_ptr) return false;

    return true;
}

bool MouseController::selectDriver() const {
    const std::vector<std::pair<Send::SendType, std::string> > drivers = {
        {Send::SendType::Logitech, "Logitech Gaming Software"},
        {Send::SendType::Razer, "Razer Gaming Software"},
        {Send::SendType::AnyDriver, "Any Driver"},
        {Send::SendType::SendInput, "Windows Standard Input"},
        {Send::SendType::DD, "Direct Input"},
        {Send::SendType::MouClassInputInjection, "Mouse Class Input Injection"}
    };

    for (const auto &[type, name]: drivers) {
        if (IbSendInit_ptr(type, 0, nullptr) == Send::Error::Success) {
            std::cout << "Using driver type: " << name << std::endl;
            return true;
        }
    }

    return false;
}