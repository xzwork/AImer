/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "AutoAim.h"
#include "controller/MouseController.hpp"
#include "Games.hpp"
#include <cmath>
#include <chrono>
#include <limits>

static void controlMouse(const float x, const float y,
                         const GameSettings &settings, const float sensitivity,
                         const int screenWidth, const int screenHeight) {
    const float modification = settings.default_sensitivity / sensitivity;
    const MouseController &mouse = MouseController::getInstance();
    constexpr auto delta = std::chrono::milliseconds(50);
    const auto now = std::chrono::steady_clock::now();
    static auto lastInstance = now - delta;
    if (now - lastInstance < delta) return;

    float focalLength;
    if (settings.name == "apex") {
        if (screenWidth <= 0 || screenHeight <= 0) return;
        // Apex FOV is horizontal at 4:3. Convert to actual desktop HFOV.
        // Capture and model have equal dimensions, so offsets are desktop pixels.
        const float aspect = static_cast<float>(screenWidth) / screenHeight;
        const float tanHalfHorizontal = std::tan(settings.fov * PI / 360.0f) * aspect / (4.0f / 3.0f);
        focalLength = screenWidth / (2.0f * tanHalfHorizontal);
    } else {
        focalLength = DEFAULT_CAPTURE_SIZE / std::tan(settings.field_radian);
    }
    mouse.MoveRelative(static_cast<int>(std::lround(std::atan(x / focalLength) * settings.pixels_per_radian * modification)),
                       static_cast<int>(std::lround(std::atan(y / focalLength) * settings.pixels_per_radian * modification)));
    lastInstance = now;
}

void autoAim(const Detector::Detection &detection, const int cx, const int cy,
             const GameSettings &settings, const std::string &target, const float sensitivity,
             const int screenWidth, const int screenHeight) {
    const bool apex = settings.name == "apex";
    int target_id = apex ? 0 : -1;
    if (!apex) {
        for (size_t i = 0; i < settings.targets.size(); ++i)
            if (settings.targets[i] == target) target_id = static_cast<int>(i);
    }
    if (target_id < 0 || !std::isfinite(sensitivity) || sensitivity <= 0) return;

    // Simple frame-to-frame association; never retain an absent/out-of-FOV box.
    static cv::Rect lockedBox;
    static bool locked = false;
    static int previousWidth = 0, previousHeight = 0;
    if (!apex || previousWidth != screenWidth || previousHeight != screenHeight) locked = false;
    previousWidth = screenWidth;
    previousHeight = screenHeight;
    int nearest = -1, matched = -1;
    float nearestDistance = (std::numeric_limits<float>::max)();
    float bestIou = settings.target_lock_iou;
    for (const int id : detection.valid) {
        if (detection.label_id[id] != target_id) continue;
        const auto &box = detection.boxes[id];
        if (box.width <= 0 || box.height <= 0) continue;
        const float x = box.x + box.width * 0.5f - cx;
        const float y = box.y + box.height * 0.5f - cy;
        const float distance = std::hypot(x, y);
        if (apex && distance > settings.aim_fov) continue;
        if (distance < nearestDistance) {
            nearest = id;
            nearestDistance = distance;
        }
        if (apex && locked) {
            const float intersection = static_cast<float>((box & lockedBox).area());
            const float iou = intersection / (static_cast<float>(box.area()) + lockedBox.area() - intersection);
            if (iou >= bestIou) {
                bestIou = iou;
                matched = id;
            }
        }
    }
    const int selected = matched >= 0 ? matched : nearest;
    if (selected < 0) {
        locked = false;
        return;
    }
    lockedBox = detection.boxes[selected];
    locked = apex;
    controlMouse(lockedBox.x + lockedBox.width * 0.5f - cx,
                 lockedBox.y + lockedBox.height * 0.5f - cy,
                 settings, sensitivity, screenWidth, screenHeight);
}
