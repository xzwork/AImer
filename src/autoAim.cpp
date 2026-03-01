#include "AutoAim.h"
#include "MouseController.hpp"
static constexpr int NEAR_DIST = 5;
static constexpr float MOUSE_SPEED = 2.4f;
static constexpr float FIELD_ANGLE = 17.0f; // Valorant's field of view (640x640 in 2560x1440)
static constexpr float PIXELS_PER_ANGLE = 145.8f; // when sensitivity = 0.1
static constexpr float DEFAULT_SENSITIVITY = 0.1f;
static constexpr float DEFAULT_CAPTURE_SIZE = 640.0f;

/**
 * @brief controls mouse movement and auto-fires
 *
 * Intelligently controls mouse movement by switching between
 * linear fine-tuning (for close targets) and angular projection (for large turns),
 * and auto-fires when aligned.
 *
 * @param x Horizontal offset on screen.
 * @param y Vertical offset on screen.
 * @param game_sensitivity In-game sensitivity setting.
 */
static void control_mouse(const float x, const float y, const float game_sensitivity) {
    const float sensitivity_modification = DEFAULT_SENSITIVITY / game_sensitivity;
    const MouseController &mouse = MouseController::getInstance();

    static bool aim = true;
    if (aim) {
        if (std::sqrt(x * x + y * y) <= NEAR_DIST) {
            mouse.MoveRelative(static_cast<int>(x * MOUSE_SPEED * sensitivity_modification),
                               static_cast<int>(y * MOUSE_SPEED * sensitivity_modification));
        } else {
            aim = false;
            constexpr float PI = 3.141592;
            const float pixels_per_angle = PIXELS_PER_ANGLE * sensitivity_modification;
            const float pixels_per_radian = pixels_per_angle / PI * 180.0f;
            const float dep = DEFAULT_CAPTURE_SIZE / 2.0f / std::tan(FIELD_ANGLE * PI / 180.0f);

            mouse.MoveRelative(static_cast<int>(std::atan(x / dep) * pixels_per_radian),
                               static_cast<int>(std::atan(y / dep) * pixels_per_radian));
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
 * @param target The specific class ID to aim at.
 * @param game_sensitivity Mouse sensitivity setting in the game.
 */
void auto_aim(const Detection &detection, const int cx, const int cy,
    const int target, const float game_sensitivity) {
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

    if (~nearest) control_mouse(x, y, game_sensitivity);
}
