/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <stdexcept>

#include "GameSettings.hpp"

#define SETTINGS  getSettings(NAME, MOUSE_SPEED, DEFAULT_SENSITIVITY, FIELD_ANGLE, PIXELS_PER_CIRCLE, TARGETS)

static float to_radian(const float degree) { return degree * PI / 180.0f; }

GameSettings getSettings(const std::string &NAME,
                         const float MOUSE_SPEED,
                         const float DEFAULT_SENSITIVITY,
                         const float FIELD_ANGLE,
                         const float PIXELS_PER_CIRCLE,
                         const std::vector<std::string> &TARGETS) {
    return GameSettings{
        NAME,
        MOUSE_SPEED,
        DEFAULT_SENSITIVITY,
        to_radian(FIELD_ANGLE),
        PIXELS_PER_CIRCLE / TWO_PI,
        TARGETS
    };
}

// valorant's settings
namespace valorant {
    const std::string NAME = "valorant";
    constexpr float MOUSE_SPEED = 1.8f;
    constexpr float DEFAULT_SENSITIVITY = 0.1f;
    constexpr float FIELD_ANGLE = 34.0f;
    constexpr float PIXELS_PER_CIRCLE = 52488.0f;
    const std::vector<std::string> TARGETS = {"head", "enemy"};

    GameSettings settings = SETTINGS;
}

// cs2's settings
namespace cs2 {
    const std::string NAME = "cs2";
    constexpr float MOUSE_SPEED = 1.8f;
    constexpr float DEFAULT_SENSITIVITY = 1.0f;
    constexpr float FIELD_ANGLE = 37.0f;
    constexpr float PIXELS_PER_CIRCLE = 16296.0f;
    const std::vector<std::string> TARGETS = {"ct", "ct_head", "t", "t_head"};

    GameSettings settings = SETTINGS;
}

// ssjj's settings
namespace ssjj {
    const std::string NAME = "ssjj";
    constexpr float MOUSE_SPEED = 0.5f;
    constexpr float DEFAULT_SENSITIVITY = 10.0f;
    constexpr float FIELD_ANGLE = 28.2f;
    constexpr float PIXELS_PER_CIRCLE = 18000.0f;
    const std::vector<std::string> TARGETS = {"ct", "ct_head", "t", "t_head"};

    GameSettings settings = SETTINGS;
}

GameSettings GameSettings::getSettings(const std::string &game_name) {
    if (game_name == "valorant") return valorant::settings;
    if (game_name == "cs2") return cs2::settings;
    if (game_name == "ssjj") return ssjj::settings;
    std::string message = "Unknown game name: " + game_name + ". Supported: valorant, cs2, ssjj";
    throw std::invalid_argument(message);
}
