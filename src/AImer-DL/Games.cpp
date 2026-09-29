/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "Games.hpp"
#include <yaml-cpp/yaml.h>
#include <cmath>

Games &Games::instance() {
    static Games games;
    return games;
}

std::vector<std::string> Games::gameNames() const {
    std::vector<std::string> names;
    names.reserve(games.size());
    for (const auto &game: games) names.push_back(game.name);
    return names;
}

const GameSettings &Games::getSettings(const std::string &gameName) const {
    for (const auto &game: games) {
        if (game.name == gameName) return game;
    }
    throw std::invalid_argument("Unknown game: " + gameName);
}

void Games::loadFromFile(const std::filesystem::path &path) {
    const auto doc = YAML::LoadFile(path.string());
    games.clear();
    games.reserve(doc.size());
    // clang-format off
    for (const auto &node: doc) {
        GameSettings settings;
        settings.name                = node.first.as<std::string>();
        settings.mouse_speed         = node.second["mouse_speed"].as<float>();
        settings.default_sensitivity = node.second["default_sensitivity"].as<float>();
        settings.field_radian        = node.second["field_angle"].as<float>(0.0f) * PI / 180.0f;
        settings.pixels_per_radian   = node.second["pixels_per_circle"].as<float>() / TWO_PI;
        settings.sensitivity = node.second["sensitivity"].as<float>(settings.default_sensitivity);
        if (!std::isfinite(settings.sensitivity) || settings.sensitivity <= 0)
            throw std::invalid_argument("Invalid game sensitivity: " + settings.name);
        if (settings.name == "apex") {
            settings.fov = node.second["fov"].as<float>(110.0f);
            settings.aim_fov = node.second["aim_fov"].as<float>(120.0f);
            settings.target_lock_iou = node.second["target_lock_iou"].as<float>(0.2f);
            if (!std::isfinite(settings.fov) || settings.fov <= 0 || settings.fov >= 180 ||
                !std::isfinite(settings.aim_fov) || settings.aim_fov <= 0 ||
                !std::isfinite(settings.target_lock_iou) || settings.target_lock_iou <= 0 ||
                settings.target_lock_iou > 1)
                throw std::invalid_argument("Invalid Apex FOV or target lock settings");
        }
        if (!std::isfinite(settings.default_sensitivity) || settings.default_sensitivity <= 0 ||
            !std::isfinite(settings.pixels_per_radian) || settings.pixels_per_radian <= 0)
            throw std::invalid_argument("Invalid sensitivity calibration: " + settings.name);

        settings.targets.reserve(node.second["targets"].size());
        for (const auto &target: node.second["targets"])
            settings.targets.push_back(target.as<std::string>());

        if (settings.name == "apex") settings.targets = {"enemy"};

        games.push_back(std::move(settings));
    }
    // clang-format on
}
