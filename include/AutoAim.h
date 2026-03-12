#pragma once
#include "GameSettings.hpp"
#include "onnxDetector.hpp"

void autoAim(const Detection &detection, int cx, int cy,
             const GameSettings &settings, int target, float sensitivity);
