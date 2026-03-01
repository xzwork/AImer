#pragma once
#include "../deps/IbInputSimulator/InputSimulator.hpp"

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

public:
    static MouseController &getInstance();

private:
    MouseController();

    ~MouseController();

    bool loadDll();

    [[nodiscard]] bool selectDriver() const;

public:
    MouseController &operator=(const MouseController &) = delete;

    void MoveRelative(int dx, int dy) const;

    void fire() const;
};
