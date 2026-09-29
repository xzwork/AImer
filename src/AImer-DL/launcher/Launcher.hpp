/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>

#include <GLFW/glfw3.h>
#include <eui/signal.h>
#include <core/app/main_window_runtime.h>
#include <core/render/render_backend.h>

class Launcher {
public:
    struct Args {
        std::string gameName;
        std::string target;
        float sensitivity;
        std::string modelPath;
        std::string weightsPath;
        std::filesystem::path exeDir;
        bool diagnostics = false;
    };

private:
    Args args;

    std::vector<std::string> m_allGameNames;

    GLFWwindow *m_window;
    app::AppRunner m_appRunner;
    std::unique_ptr<app::MainWindowRuntime> m_mainWindowRuntime;
    std::unique_ptr<core::render::RenderBackend> m_renderBackend;
    float m_scale;

public:
    int selectedGame;
    int selectedTarget;
    std::string sensitivityStr;
    std::string modelPath;
    std::string weightsPath;
    std::string statusMsg;
    eui::Signal<bool> gameDropdownOpen;
    eui::Signal<bool> targetDropdownOpen;
    bool shouldCancel;
    bool shouldLaunch;
    bool statusIsError;

private:
    Launcher();

    static std::string trim(std::string_view str);

    void saveConfig() const;

    void loadConfig();

    void installCallbacks() const;

    void firstSetWindowSize();

    void handleDpiScaleChange();

    void throttleAnimationFrame() const;

    void render() const;

    void cleanup();

    void launchFromArgs(int argc, const char **argv);

    void launchFromGUI();

    [[nodiscard]] float getDpiScale() const;

    [[nodiscard]] float getPointerScale() const;

public:
    static Launcher &instance();

    void init(int argc, const char **argv);

    void updateStatus(const std::string &msg, bool isError);

    bool validateAndLaunch();

    [[noreturn]] void launchAutoAim();

    [[nodiscard]] bool isLaunched() const;

    [[nodiscard]] const std::vector<std::string> &getGames() const;

    [[nodiscard]] std::string getCurrentGameName() const;

    [[nodiscard]] const std::vector<std::string> &getCurrentGameTargets() const;

    [[nodiscard]] bool isTargetIndexValid() const;

    [[nodiscard]] bool isAnimating() const;

    [[nodiscard]] bool windowShouldClose() const;
};
