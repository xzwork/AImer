#pragma once

static constexpr int NEAR_DIST = 5;
static constexpr float PI = 3.141592;
static constexpr float TWO_PI = 2.0f * PI;
static constexpr float DEFAULT_CAPTURE_SIZE = 640.0f;

struct GameSettings {
    float mouse_speed;
    float default_sensitivity;
    float field_radian;
    float pixels_per_radian;

    static GameSettings getSettings(int game);
};
