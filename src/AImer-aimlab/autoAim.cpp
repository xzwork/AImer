#include "autoAim.h"
#include <cmath>
#include <limits>
#include <chrono>
constexpr int MIN_BALL_AREA = 20;
constexpr double FIRE_THRESHOLD_PX = 40.0;
constexpr auto MOVE_MIN_INTERVAL_MS = std::chrono::milliseconds(1);

std::optional<cv::Point2i> autoAim(const cv::Mat &originImg) {
    // get binary image
    cv::Mat binaryImg;
    static const cv::Scalar lower_blue(82, 199, 118);
    static const cv::Scalar upper_blue(97, 255, 255);
    cv::cvtColor(originImg, binaryImg, cv::COLOR_BGR2HSV);
    cv::inRange(binaryImg, lower_blue, upper_blue, binaryImg);

    // find contours
    std::vector<cv::Vec4i> hierarchy;
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(binaryImg, contours, hierarchy,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    if (contours.empty()) return std::nullopt;

    // find the closest ball
    int dx = 0, dy = 0;
    const int cx = originImg.cols / 2;
    const int cy = originImg.rows / 2;
    double nearest_dist = (std::numeric_limits<double>::max)();

    for (const auto &contour: contours) {
        const auto rect = cv::boundingRect(contour);
        if (rect.area() < MIN_BALL_AREA) continue;

        const int tx = rect.x + rect.width / 2;
        const int ty = rect.y + rect.height / 2;
        const double dist = std::hypot(tx - cx,
                                       ty - cy);
        if (dist < nearest_dist) {
            nearest_dist = dist;
            dx = tx - cx;
            dy = ty - cy;
        }
    }

    if (nearest_dist == (std::numeric_limits<double>::max)()) return std::nullopt;
    return cv::Point2i(dx, dy);
}

void autoAim(const cv::Point2i &offset,
             const MouseController &mouse,
             const AimlabSettings &settings,
             const float sensitivity) {
    // fire
    const auto x = static_cast<float>(offset.x);
    const auto y = static_cast<float>(offset.y);
    if (std::hypot(x, y) <= FIRE_THRESHOLD_PX) {
        mouse.fire();
        return;
    }
    // move mouse
    const float modification = settings.default_sensitivity / sensitivity;
    const float dep = AIMLAB_CAPTURE_SIZE / std::tan(settings.field_radian);
    mouse.MoveRelative(
        static_cast<int>(std::atan(x / dep) * settings.pixels_per_radian * modification),
        static_cast<int>(std::atan(y / dep) * settings.pixels_per_radian * modification));
    std::this_thread::sleep_for(MOVE_MIN_INTERVAL_MS);
}