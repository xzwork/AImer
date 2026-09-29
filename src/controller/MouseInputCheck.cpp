#include "InputSimulator.hpp"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wchar_t executable[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, executable, MAX_PATH)) return 1;
    const auto directory = std::filesystem::path(executable).parent_path();
    std::ofstream log(directory / "mouse-desktop-test.log", std::ios::trunc);
    if (!log) {
        std::cerr << "Cannot create mouse-desktop-test.log\n";
        return 1;
    }
    auto report = [&](const std::string &line) {
        std::cout << line << std::endl;
        log << line << '\n' << std::flush;
    };
    report("Desktop mouse check: no model, capture, clicks, or game integration.");
    report("Close AImer, minimize the game, and put the pointer near the desktop center.");
    report("Press Enter to start. Then do not touch the mouse for 5 seconds.");
    std::string answer;
    std::getline(std::cin, answer);
    report("Starting in 3 seconds...");
    Sleep(3000);

    RECT clip{};
    if (GetClipCursor(&clip))
        report("Cursor clip rectangle: " + std::to_string(clip.left) + "," + std::to_string(clip.top) +
               " to " + std::to_string(clip.right) + "," + std::to_string(clip.bottom));

    using Init = Send::Error (__stdcall *)(Send::SendType, Send::InitFlags, void *);
    using Move = bool (__stdcall *)(int, int, Send::MoveMode);
    using Destroy = void (__stdcall *)();
    const HMODULE dll = LoadLibraryW((directory / "IbInputSimulator.dll").c_str());
    Init init = dll ? reinterpret_cast<Init>(GetProcAddress(dll, "IbSendInit")) : nullptr;
    Move move = dll ? reinterpret_cast<Move>(GetProcAddress(dll, "IbSendMouseMove")) : nullptr;
    Destroy destroy = dll ? reinterpret_cast<Destroy>(GetProcAddress(dll, "IbSendDestroy")) : nullptr;
    bool ibReady = false;
    if (init && move && destroy) {
        const auto result = init(Send::SendType::SendInput, 0, nullptr);
        report("Ib SendInput init code=" + std::to_string(static_cast<unsigned>(result)));
        ibReady = result == Send::Error::Success;
    } else {
        report("Ib DLL unavailable or missing required exports; native test will still run.");
    }

    auto measure = [&](const char *name, int dx, bool native) {
        POINT before{}, after{};
        if (!GetCursorPos(&before)) {
            report(std::string(name) + " GetCursorPos failed=" + std::to_string(GetLastError()));
            return;
        }
        SetLastError(0);
        unsigned result;
        if (native) {
            INPUT input{};
            input.type = INPUT_MOUSE;
            input.mi.dx = dx;
            input.mi.dwFlags = MOUSEEVENTF_MOVE;
            result = SendInput(1, &input, sizeof(input));
        } else {
            result = move(dx, 0, Send::MoveMode::Relative) ? 1u : 0u;
        }
        const DWORD error = GetLastError();
        Sleep(300);
        if (!GetCursorPos(&after)) {
            report(std::string(name) + " after GetCursorPos failed=" + std::to_string(GetLastError()));
            return;
        }
        report(std::string(name) + " requested_dx=" + std::to_string(dx) +
               " api_result=" + std::to_string(result) + " last_error=" + std::to_string(error) +
               " before=" + std::to_string(before.x) + "," + std::to_string(before.y) +
               " after=" + std::to_string(after.x) + "," + std::to_string(after.y) +
               " actual_dx=" + std::to_string(after.x - before.x) +
               " actual_dy=" + std::to_string(after.y - before.y));
    };

    if (ibReady) measure("IbInputSimulator", 80, false);
    // Release the wrapper before testing the Windows API independently.
    if (ibReady) destroy();
    if (dll) FreeLibrary(dll);
    Sleep(700);
    measure("Windows SendInput", -80, true);
    report("Finished. Pointer acceleration can change actual distance; this only checks desktop movement.");
    report("Log saved beside this executable: mouse-desktop-test.log");
    return 0;
}
