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
#include <cstdlib>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

MouseController &MouseController::getInstance() {
    static MouseController instance;
    return instance;
}

MouseController::MouseController() {
    const char *trace = std::getenv("AIMER_INPUT_TRACE");
    traceInput = trace && std::string(trace) == "1";
    try {
        ready = selectDriver();
    } catch (const std::exception &e) {
        std::cerr << "[mouse] backend initialization failed: " << e.what() << std::endl;
    }
    if (!ready) std::cerr << "[mouse] movement and clicks disabled; no automatic fallback" << std::endl;
}

MouseController::~MouseController() {
    if (!hMouseDll) return;
    if (IbSendDestroy_ptr) IbSendDestroy_ptr();
    FreeLibrary(hMouseDll);
}

void MouseController::MoveRelative(const int dx, const int dy) const {
    bool sent;
    DWORD error = 0;
    if (virtualInput) {
        sent = virtualHid.MoveRelative(dx, dy, error);
    } else if (!ready) {
        sent = false;
        error = ERROR_NOT_READY;
    } else if (nativeInput) {
        INPUT input{};
        input.type = INPUT_MOUSE;
        input.mi.dx = dx;
        input.mi.dy = dy;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        SetLastError(0);
        sent = ::SendInput(1, &input, sizeof(INPUT)) == 1;
        if (!sent) error = GetLastError();
    } else {
        sent = IbSendMouseMove_ptr(dx, dy, Send::MoveMode::Relative);
    }
    if (!traceInput && sent) return;
    static std::ofstream log;
    static unsigned calls = 0, failures = 0, nonzero = 0;
    static bool failureLogged = false;
    static auto last = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    ++calls;
    if (!sent) ++failures;
    if (dx != 0 || dy != 0) ++nonzero;
    const auto now = std::chrono::steady_clock::now();
    if (now - last < std::chrono::seconds(1) && (sent || failureLogged)) return;
    const std::string line = "[mouse] backend=" + backendName + " dx=" + std::to_string(dx) +
        " dy=" + std::to_string(dy) + " api=" + (sent ? "OK" : "FAILED") +
        " calls=" + std::to_string(calls) + " nonzero=" + std::to_string(nonzero) +
        " failures=" + std::to_string(failures) + " win32_error=" + std::to_string(error) +
        (virtualInput && !sent ? (ready
            ? " (DeviceIoControl MOVE_RELATIVE failed; earlier chunks may have moved)"
            : " (Virtual HID device unavailable; CreateFile failed)") : "");
    std::cout << line << std::endl;
    if (traceInput) {
        if (!log.is_open()) log.open("mouse-input.log", std::ios::trunc);
        if (log) log << line << '\n' << std::flush;
    }
    calls = failures = nonzero = 0;
    failureLogged = !sent;
    last = now;
}


void MouseController::fire() const {
    if (virtualInput) {
        static const bool warned = [] {
            std::cerr << "[mouse] backend=virtual_hid fire ignored: only relative movement is supported" << std::endl;
            return true;
        }();
        (void)warned;
        return;
    }
    if (!ready) return;
    if (nativeInput) {
        INPUT inputs[2]{};
        inputs[0].type = inputs[1].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        const UINT sent = ::SendInput(2, inputs, sizeof(INPUT));
        // If only the press was accepted, make one release attempt.
        if (sent == 1) ::SendInput(1, &inputs[1], sizeof(INPUT));
        if (sent != 2) std::cerr << "Win32 mouse click incomplete: events=" << sent << std::endl;
        return;
    }
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

bool MouseController::selectDriver() {
    const char *configured = std::getenv("AIMER_INPUT_BACKEND");
    std::string mode = "Win32";
    if (configured && *configured) {
        mode = configured;
    } else {
        // Resolve beside the executable, regardless of the launcher's working directory.
        wchar_t exePath[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, exePath, 32768);
        if (!length || length >= 32768) throw std::runtime_error("cannot resolve mouse.yaml path");
        const auto path = std::filesystem::path(exePath).parent_path() / "mouse.yaml";
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            if (!file) throw std::runtime_error("cannot open mouse.yaml");
            const auto config = YAML::Load(file);
            mode = config["mouse_backend"].as<std::string>("Win32");
        }
    }
    if (mode == "send_input") mode = "Win32";
    if (mode == "ib_input") mode = "SendInput";
    if (mode != "Auto" && mode != "SendInput" && mode != "Win32" && mode != "virtual_hid")
        throw std::invalid_argument("mouse_backend / AIMER_INPUT_BACKEND must be virtual_hid, send_input, ib_input, Win32, Auto or SendInput");
    std::cout << "Mouse input mode: " << mode << std::endl;
    if (traceInput)
        std::cout << "Mouse tracing ON: API success does not confirm game acceptance; log=mouse-input.log" << std::endl;
    if (mode == "virtual_hid") {
        virtualInput = true;
        backendName = "virtual_hid";
        return virtualHid.Open();
    }
    if (mode == "Win32") {
        nativeInput = true;
        backendName = "Win32 SendInput";
        std::cout << "Using driver type: " << backendName << " (native user32.dll)" << std::endl;
        return true;
    }
    if (!loadDll()) throw std::runtime_error("failed to load IbInputSimulator.dll");
    const std::vector<std::pair<Send::SendType, std::string> > drivers = {
        {Send::SendType::Logitech, "Logitech Gaming Software"},
        {Send::SendType::Razer, "Razer Gaming Software"},
        {Send::SendType::AnyDriver, "Any Driver"},
        {Send::SendType::SendInput, "Windows Standard Input"},
        {Send::SendType::DD, "Direct Input"},
        {Send::SendType::MouClassInputInjection, "Mouse Class Input Injection"}
    };

    for (const auto &[type, name]: drivers) {
        if (mode == "SendInput" && type != Send::SendType::SendInput) continue;
        const auto result = IbSendInit_ptr(type, 0, nullptr);
        if (result == Send::Error::Success) {
            backendName = name;
            std::cout << "Using driver type: " << name << std::endl;
            return true;
        }
        if (mode == "SendInput") {
            std::cerr << "SendInput initialization failed, code=" << static_cast<unsigned>(result) << std::endl;
            return false;
        }
        std::cout << "Input backend unavailable: " << name << "; trying next backend" << std::endl;
    }

    return false;
}
