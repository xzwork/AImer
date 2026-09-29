/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include "InputSimulator.hpp"
#include "VirtualHidBackend.hpp"
#include <string>

class MouseController {
private:
    typedef Send::Error (__stdcall*pIbSendInit)(Send::SendType, Send::InitFlags, void *);

    typedef void (__stdcall*pIbSendDestroy)();

    typedef bool (__stdcall*pIbSendMouseMove)(int, int, Send::MoveMode);

    typedef bool (__stdcall*pIbSendMouseClick)(Send::MouseButton);

    HMODULE hMouseDll{nullptr};
    pIbSendInit IbSendInit_ptr{nullptr};
    pIbSendDestroy IbSendDestroy_ptr{nullptr};
    pIbSendMouseMove IbSendMouseMove_ptr{nullptr};
    pIbSendMouseClick IbSendMouseClick_ptr{nullptr};
    std::string backendName;
    bool traceInput = false;
    bool nativeInput = false;
    bool virtualInput = false;
    bool ready = false;
    VirtualHidBackend virtualHid;

public:
    static MouseController &getInstance();

private:
    MouseController();

    ~MouseController();

    bool loadDll();

    [[nodiscard]] bool selectDriver();

public:
    MouseController &operator=(const MouseController &) = delete;

    void MoveRelative(int dx, int dy) const;

    void fire() const;
};
