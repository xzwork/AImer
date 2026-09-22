#pragma once
#include <optional>
#include <opencv2/opencv.hpp>
#include "controller/MouseController.hpp"

static constexpr float AIMLAB_PI = 3.141592f;
static constexpr float AIMLAB_TWO_PI = 2.0f * AIMLAB_PI;
static constexpr float AIMLAB_CAPTURE_SIZE = 640.0f;

inline struct AimlabSettings {
    float default_sensitivity;
    float field_radian;
    float pixels_per_radian;
} AIMLAB_VALORANT =
{
    0.1f,
    34.0f * AIMLAB_PI / 180.0f,
    52488.0f / AIMLAB_TWO_PI
};


std::optional<cv::Point2i> autoAim(const cv::Mat &originImg);

void autoAim(const cv::Point2i &offset,
             const MouseController &mouse,
             const AimlabSettings &settings,
             float sensitivity);