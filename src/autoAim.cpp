/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "AutoAim.h"
#include "MouseController.hpp"
#include "Games.hpp"
#include <cmath>

static void controlMouse(const float x, const float y,
                         const GameSettings &settings, const float sensitivity) {
    auto &[name,
        speed,
        default_sensitivity,
        field_radian,
        pixels_per_radian,
        targets] = settings;
    const float modification = default_sensitivity / sensitivity;

    const MouseController &mouse = MouseController::getInstance();

    constexpr auto delta = std::chrono::milliseconds(30);
    constexpr auto short_delta = std::chrono::milliseconds(10);
    const auto now = std::chrono::steady_clock::now();
    static auto lastInstance = std::chrono::steady_clock::now();

    if (now - lastInstance >= delta) {
        const float dep = DEFAULT_CAPTURE_SIZE / std::tan(field_radian);
        mouse.MoveRelative(static_cast<int>(std::atan(x / dep) * pixels_per_radian * modification),
                           static_cast<int>(std::atan(y / dep) * pixels_per_radian * modification));
        lastInstance = now;
    }
}

void autoAim(const Detector::Detection &detection, const int cx, const int cy,
             const GameSettings &settings, const std::string &target, const float sensitivity) {
    int target_id = -1;
    for (size_t i = 0; i < settings.targets.size(); i++) {
        if (settings.targets[i] == target) {
            target_id = static_cast<int>(i);
            break;
        }
    }
    if (target_id == -1) return;

    int nearest = -1;
    int nearest_dist = 0x7fffffff;

    float x = 0, y = 0;

    for (const int id: detection.valid) {
        if (detection.label_id[id] != target_id) continue;

        const int tx = detection.boxes[id].x + detection.boxes[id].width / 2;
        const int ty = detection.boxes[id].y + detection.boxes[id].height / 2;
        const int dist = std::hypot(tx - cx,
                                    ty - cy);
        if (nearest == -1 || dist < nearest_dist) {
            nearest = id;
            nearest_dist = dist;
            x = static_cast<float>(tx - cx);
            y = static_cast<float>(ty - cy);
        }
    }

    if (~nearest) controlMouse(x, y, settings, sensitivity);
}
