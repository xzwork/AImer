/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <iostream>
#include <opencv2/imgproc.hpp>
#include "ScreenCapture.hpp"

#define DXGI_INIT(msg, hr)      if (FAILED(hr)) initError(msg, #hr, __LINE__)
#define DXGI_PROC(msg, hr, dup) if (FAILED(hr)) procError(msg, #hr, __LINE__, dup)

static void initError(const std::string &msg, const std::string &funcName, const int line) {
    std::cerr << "Error at " << funcName << ":" << line << std::endl;
    throw std::runtime_error("Failed to " + msg);
}

static void procError(const std::string &msg,
                      const std::string &funcName,
                      const int line,
                      const DXGIPtr<IDXGIOutputDuplication> &duplication) {
    duplication->ReleaseFrame();
    std::cerr << "Error at " << funcName << ":" << line << std::endl;
    throw std::runtime_error("Failed to " + msg);
}

ScreenCapture &ScreenCapture::getInstance(const int width, const int height) {
    static ScreenCapture capture(width, height);
    if (capture.m_regionWidth != width || capture.m_regionHeight != height) {
        capture.m_regionWidth = width;
        capture.m_regionHeight = height;
        capture.initDXGI();
    }
    return capture;
}


bool ScreenCapture::CaptureFrame(cv::Mat &frame) {
    // Acquire the next desktop frame
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    DXGIPtr<IDXGIResource> p_desktopResource;
    const HRESULT hr = m_duplication->AcquireNextFrame(10, &frameInfo, p_desktopResource.ptrAddress());
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return false;
    if (FAILED(hr)) {
        initDXGI();
        return false;
    }

    // Convert the desktop resource to a 2D texture
    DXGIPtr<ID3D11Texture2D> p_desktopImg;
    DXGI_PROC("convert desktop resource to texture",
              p_desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), p_desktopImg.voidPtrAddress()),
              m_duplication);

    // Copy the region of interest to the staging texture
    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->CopySubresourceRegion(m_texture.getAddress(), 0, 0, 0, 0,
                                     p_desktopImg.getAddress(), 0, &m_region);

    // Map the staging texture to CPU memory
    DXGI_PROC("map GPU texture to CPU memory",
              m_context->Map(m_texture.getAddress(), 0, D3D11_MAP_READ, 0, &mapped),
              m_duplication);

    // Convert BGRA to BGR
    const cv::Mat bgra_frame(m_regionHeight, m_regionWidth,
                             CV_8UC4, mapped.pData, mapped.RowPitch);
    cv::cvtColor(bgra_frame, frame, cv::COLOR_BGRA2BGR);

    // Clean up DirectX resources
    m_context->Unmap(m_texture.getAddress(), 0);
    m_duplication->ReleaseFrame();

    return true;
}


ScreenCapture::ScreenCapture(const int regionWidth, const int regionHeight)
    : m_regionWidth(regionWidth), m_regionHeight(regionHeight) {
    initDXGI();
}

void ScreenCapture::initDXGI() {
    m_output.release();
    m_device.release();
    m_output1.release();
    m_factory.release();
    m_adapter.release();
    m_texture.release();
    m_context.release();
    m_duplication.release();

    // initialize DXGI components for screen capture
    DXGI_INIT("create DXGI Factory",
              CreateDXGIFactory1(__uuidof(IDXGIFactory1), m_factory.voidPtrAddress()));

    DXGI_INIT("enumerate adapters",
              m_factory->EnumAdapters1(0, m_adapter.ptrAddress()));

    DXGI_INIT("enumerate outputs",
              m_adapter->EnumOutputs(0, m_output.ptrAddress()));

    DXGI_INIT("create D3D11 device",
              D3D11CreateDevice( m_adapter.getAddress(), D3D_DRIVER_TYPE_UNKNOWN,
                  nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
                  m_device.ptrAddress(), nullptr, m_context.ptrAddress()));

    DXGI_INIT("query IDXGIOutput1",
              m_output->QueryInterface(__uuidof(IDXGIOutput1), m_output1.voidPtrAddress()));

    DXGI_INIT("create D3D11 output duplication",
              m_output1->DuplicateOutput(m_device.getAddress(), m_duplication.ptrAddress()));

    // Get output display description
    // clang-format off
    DXGI_OUTPUT_DESC outputDesc;
    DXGI_INIT("get output display description", m_output->GetDesc(&outputDesc));
    // DesktopCoordinates may be DPI-virtualized. Duplication dimensions are pixels
    // in the image being copied, so cropping and mouse geometry must use these.
    DXGI_OUTDUPL_DESC duplicationDesc{};
    m_duplication->GetDesc(&duplicationDesc);
    m_screenWidth  = static_cast<int>(duplicationDesc.ModeDesc.Width);
    m_screenHeight = static_cast<int>(duplicationDesc.ModeDesc.Height);
    if (m_regionWidth <= 0 || m_regionHeight <= 0 ||
        m_regionWidth > m_screenWidth || m_regionHeight > m_screenHeight)
        throw std::runtime_error("Capture dimensions must fit within the DXGI display");
    m_region.front  = 0;
    m_region.back   = 1;
    m_region.left   = (m_screenWidth  - m_regionWidth) >> 1;
    m_region.top    = (m_screenHeight - m_regionHeight) >> 1;
    m_region.right  = (m_screenWidth  + m_regionWidth) >> 1;
    m_region.bottom = (m_screenHeight + m_regionHeight) >> 1;
    // clang-format on

    // Create staging texture
    // clang-format off
    D3D11_TEXTURE2D_DESC desc;
    desc.Width              = m_regionWidth;
    desc.Height             = m_regionHeight;;
    desc.MipLevels          = 1;
    desc.ArraySize          = 1;
    desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_STAGING;
    desc.BindFlags          = 0;
    desc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags          = 0;
    DXGI_INIT("create staging texture", m_device->CreateTexture2D(&desc, nullptr, m_texture.ptrAddress()));
    // clang-format on

    std::cout << "ScreenCapture: " << m_screenWidth << "x" << m_screenHeight << std::endl;
    std::cout << "Capture region (physical pixels): left=" << m_region.left
              << " top=" << m_region.top << " width=" << m_regionWidth
              << " height=" << m_regionHeight << std::endl;
}
