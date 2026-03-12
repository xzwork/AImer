#include "GameSettings.hpp"

#include <array>
#define SETTINGS  getSettings(MOUSE_SPEED, DEFAULT_SENSITIVITY, FIELD_ANGLE, PIXELS_PER_CIRCLE)

static float to_radian(const float degree) { return degree * PI / 180.0f; }

GameSettings getSettings(const float MOUSE_SPEED,
                         const float DEFAULT_SENSITIVITY,
                         const float FIELD_ANGLE,
                         const float PIXELS_PER_CIRCLE) {
    return GameSettings{
        MOUSE_SPEED,
        DEFAULT_SENSITIVITY,
        to_radian(FIELD_ANGLE),
        PIXELS_PER_CIRCLE / TWO_PI
    };
}

// valorant's settings
namespace valorant {
    constexpr float MOUSE_SPEED = 1.8f;
    constexpr float DEFAULT_SENSITIVITY = 0.1f;
    constexpr float FIELD_ANGLE = 34.0f;
    constexpr float PIXELS_PER_CIRCLE = 52488.0f;

    GameSettings settings = SETTINGS;
}

// cs2's settings
namespace cs2 {
    constexpr float MOUSE_SPEED = 1.8f;
    constexpr float DEFAULT_SENSITIVITY = 1.0f;
    constexpr float FIELD_ANGLE = 37.0f;
    constexpr float PIXELS_PER_CIRCLE = 16296.0f;

    GameSettings settings = SETTINGS;
}

// ssjj's settings
namespace ssjj {
    constexpr float MOUSE_SPEED = 0.5f;
    constexpr float DEFAULT_SENSITIVITY = 10.0f;
    constexpr float FIELD_ANGLE = 28.2f;
    constexpr float PIXELS_PER_CIRCLE = 18000.0f;
    GameSettings settings = SETTINGS;
}

GameSettings GameSettings::getSettings(const int game) {
    const std::array settings{
        valorant::settings,
        cs2::settings,
        ssjj::settings
    };
    return settings[game];
}
