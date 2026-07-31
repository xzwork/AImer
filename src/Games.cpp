/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include "Games.hpp"
#include <yaml-cpp/yaml.h>

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
    games.reserve(doc.size());
    // clang-format off
    for (const auto &node: doc) {
        GameSettings settings;
        settings.name                = node.first.as<std::string>();
        settings.mouse_speed         = node.second["mouse_speed"].as<float>();
        settings.default_sensitivity = node.second["default_sensitivity"].as<float>();
        settings.field_radian        = node.second["field_angle"].as<float>() * PI / 180.0f;
        settings.pixels_per_radian   = node.second["pixels_per_circle"].as<float>() / TWO_PI;

        settings.targets.reserve(node.second["targets"].size());
        for (const auto &target: node.second["targets"])
            settings.targets.push_back(target.as<std::string>());

        games.push_back(std::move(settings));
    }
    // clang-format on
}