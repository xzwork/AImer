#include "AutoAim.h"
#include "MouseController.hpp"
#include "GameSettings.hpp"
#include <numbers>

/**
 * @brief controls mouse movement and auto-fires
 *
 * Intelligently controls mouse movement by switching between
 * linear fine-tuning (for close targets) and angular projection (for large turns),
 * and auto-fires when aligned.
 *
 * @param x Horizontal offset on screen.
 * @param y Vertical offset on screen.
 * @param sensitivity In-game sensitivity setting.
 * @param settings
 */
static void controlMouse(const float x, const float y,
                         const GameSettings &settings, const float sensitivity) {
    auto &[speed, default_sensitivity, field_radian, pixels_per_radian] = settings;
    const float modification = default_sensitivity / sensitivity;

    const MouseController &mouse = MouseController::getInstance();

    static bool aim = true;
    if (aim) {
        if (std::sqrt(x * x + y * y) <= NEAR_DIST) {
            mouse.MoveRelative(static_cast<int>(x * speed * modification),
                               static_cast<int>(y * speed * modification));
        } else {
            aim = false;
            const float dep = DEFAULT_CAPTURE_SIZE / std::tan(field_radian);
            mouse.MoveRelative(static_cast<int>(std::atan(x / dep) * pixels_per_radian * modification),
                               static_cast<int>(std::atan(y / dep) * pixels_per_radian * modification));
        }
    } else aim = true;

    if (std::abs(x) <= 1 && std::abs(y) <= 1) mouse.fire();
}

/**
 * @brief Automatically aims at the nearest target of a specified class ID.
 *
 * @param detection Detection results containing boxes, labels, and valid indices.
 * @param cx Screen center X coordinate.
 * @param cy Screen center Y coordinate.
 * @param settings
 * @param target The specific class ID to aim at.
 * @param sensitivity Mouse sensitivity setting in the game.
 */
void autoAim(const Detection &detection, const int cx, const int cy,
             const GameSettings &settings, const int target, const float sensitivity) {
    int nearest = -1;
    int nearest_dist2 = 0x7fffffff;
    float x = 0, y = 0;

    for (const int id: detection.valid) {
        if (detection.label_id[id] != target) continue;

        const int tx = detection.boxes[id].x + detection.boxes[id].width / 2;
        const int ty = detection.boxes[id].y + detection.boxes[id].height / 2;
        const int dist2 = (tx - cx) * (tx - cx) + (ty - cy) * (ty - cy);

        if (nearest == -1 || dist2 < nearest_dist2) {
            nearest = id;
            nearest_dist2 = dist2;
            x = static_cast<float>(tx - cx);
            y = static_cast<float>(ty - cy);
        }
    }

    if (~nearest) controlMouse(x, y, settings, sensitivity);
}
