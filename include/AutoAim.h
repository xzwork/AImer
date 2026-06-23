/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include "GameSettings.hpp"
#include "detector/Detector.hpp"

void autoAim(const Detector::Detection &detection, int cx, int cy,
             const GameSettings &settings, const std::string &target, float sensitivity);