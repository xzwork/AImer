/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <opencv2/core.hpp>

template<typename T>
class DXGIPtr {
    T *m_ptr = nullptr;

public:
    DXGIPtr() = default;

    ~DXGIPtr() { if (m_ptr) m_ptr->Release(); }

    T get() const { return *m_ptr; }
    T *getAddress() const { return m_ptr; }
    T **ptrAddress() { return &m_ptr; }
    void **voidPtrAddress() { return reinterpret_cast<void **>(&m_ptr); }

    void release() { if (m_ptr) m_ptr->Release(); }

    auto operator->() const { return m_ptr; }
};

class ScreenCapture {
public:
    static ScreenCapture &getInstance(int width = 640, int height = 640);

    bool CaptureFrame(cv::Mat &frame);

private:
    explicit ScreenCapture(int regionWidth, int regionHeight);

    ~ScreenCapture() = default;

    [[nodiscard]] int getWidth() const { return m_screenWidth; }
    [[nodiscard]] int getHeight() const { return m_screenHeight; }

private:
    int m_screenWidth = 0, m_screenHeight = 0;
    int m_regionWidth = 0, m_regionHeight = 0;
    D3D11_BOX m_region{};
    DXGIPtr<IDXGIOutput> m_output;
    DXGIPtr<ID3D11Device> m_device;
    DXGIPtr<IDXGIOutput1> m_output1;
    DXGIPtr<IDXGIFactory1> m_factory;
    DXGIPtr<IDXGIAdapter1> m_adapter;
    DXGIPtr<ID3D11Texture2D> m_texture;
    DXGIPtr<ID3D11DeviceContext> m_context;
    DXGIPtr<IDXGIOutputDuplication> m_duplication;

    void initDXGI();
};
