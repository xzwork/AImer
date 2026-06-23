/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <vector>
#include <string>

static constexpr int NEAR_DIST = 5;
static constexpr float PI = 3.141592;
static constexpr float TWO_PI = 2.0f * PI;
static constexpr float DEFAULT_CAPTURE_SIZE = 640.0f;

struct GameSettings {
    const std::string name;
    const float mouse_speed;
    const float default_sensitivity;
    const float field_radian;
    const float pixels_per_radian;
    const std::vector<std::string> targets;

    static GameSettings getSettings(const std::string &game_name);
};
