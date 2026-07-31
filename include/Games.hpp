/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

static constexpr int NEAR_DIST = 5;
static constexpr float PI = 3.141592;
static constexpr float TWO_PI = 2.0f * PI;
static constexpr float DEFAULT_CAPTURE_SIZE = 640.0f;

struct GameSettings {
    std::string name;
    float mouse_speed = 0.0f;
    float default_sensitivity = 0.0f;
    float field_radian = 0.0f;
    float pixels_per_radian = 0.0f;
    std::vector<std::string> targets;
};

class Games {
public:
    static Games &instance();

    [[nodiscard]] std::vector<std::string> gameNames() const;
    [[nodiscard]] const GameSettings &getSettings(const std::string &gameName) const;

    void loadFromFile(const std::filesystem::path &path);

private:
    Games() = default;

    std::vector<GameSettings> games;
};