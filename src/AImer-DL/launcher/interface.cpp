/*
Required Notice: Copyright (c) 2026 何昊阳(He Haoyang) <hehaoyang1124@outlook.com>
Full license: PolyForm Noncommercial License 1.0.0
Complete license text located at repository root LICENSE file
https://polyformproject.org/licenses/noncommercial/1.0.0
*/
#include <eui_neo.h>
#include "Launcher.hpp"
#include "../Games.hpp"

namespace {
    constexpr float kMarginX = 24.0f;
    constexpr float kDropdownHeight = 24.0f;
    constexpr float kDropdownFontSize = 20.0f;
    constexpr float kDropdownRadius = 12.0f;
    constexpr float kMainTitleFontSize = 32.0f;
    constexpr float kMainTitleLineHeight = 40.0f;
    constexpr float kMainTitleTopGap = 20.0f;
    constexpr float kMainTitleBottomGap = 20.0f;
    constexpr float kLabelHeight = 26.0f;
    constexpr float kLabelFontSize = 18.0f;
    constexpr float kLabelFieldGap = 6.0f;
    constexpr float kFieldHeight = 42.0f;
    constexpr float kSectionGap = 14.0f;
    constexpr float kButtonHeight = 46.0f;
    constexpr float kButtonGap = 10.0f;
    constexpr float kBrowseButtonWidth = 90.0f;
    constexpr float kBrowseButtonGap = 8.0f;
    constexpr float kInputFontSize = 14.0f;
    constexpr float kColGap = 10.0f;
    constexpr float kComponentRadius = 10.0f;
    constexpr float kFirstRowSectionHeight = kLabelHeight + kLabelFieldGap + kFieldHeight;
    constexpr float kPopupGap = 8.0f;
    constexpr float kPopupPadding = 6.0f;
    constexpr float kItemHeight = 44.0f;
    constexpr float kItemTextPaddingX = 12.0f;
    constexpr float kWindowWidth = 480.0f;
    constexpr float kWindowHeight = 500.0f;
    constexpr float kWindowFps = 60.0f;

    void buildDropdownItems(eui::Ui &ui,
                            const std::string &prefix,
                            const std::vector<std::string> &items,
                            const int selected,
                            const float colWidth,
                            const components::theme::ThemeColorTokens &theme,
                            const std::function<void(int)> &onSelect) {
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const bool active = i == selected;
            const float itemY = kPopupPadding + static_cast<float>(i) * kItemHeight;
            ui.rect(prefix + std::to_string(i))
                    .x(kPopupPadding).y(itemY)
                    .size(colWidth - kPopupPadding * 2.0f, kItemHeight)
                    .states(components::theme::color(0, 0, 0, 0), theme.surfaceHover, theme.surfaceActive)
                    .radius(kComponentRadius)
                    .onClick([onSelect, i] { onSelect(i); })
                    .build();
            ui.text(prefix + ".label." + std::to_string(i))
                    .x(kPopupPadding + kItemTextPaddingX)
                    .y(itemY + std::max(0.0f, (kItemHeight - kDropdownHeight) * 0.5f))
                    .size(colWidth - kPopupPadding * 2.0f - kDropdownHeight, kDropdownHeight)
                    .text(items[i])
                    .fontSize(kDropdownFontSize)
                    .lineHeight(kDropdownHeight)
                    .color(active ? theme.primary : theme.text)
                    .build();
        }
    }

    void buildDropdownPopup(eui::Ui &ui,
                            const std::string &stackName,
                            const float x,
                            const float y,
                            const float colWidth,
                            const float popupH,
                            const components::theme::ThemeColorTokens &theme,
                            const eui::Color &borderColor,
                            const std::vector<std::string> &items,
                            const int selected,
                            const std::function<void(int)> &onSelect) {
        ui.stack(stackName)
                .position(x, y)
                .size(colWidth, popupH)
                .zIndex(100)
                .content([&] {
                    ui.rect(stackName + ".bg")
                            .size(colWidth, popupH)
                            .color(theme.surface)
                            .radius(kDropdownRadius)
                            .border(1.0f, borderColor)
                            .shadow(components::theme::popupShadow(theme))
                            .build();
                    ui.rect(stackName + ".hit")
                            .size(colWidth, popupH)
                            .states(components::theme::color(0, 0, 0, 0),
                                    components::theme::color(0, 0, 0, 0),
                                    components::theme::color(0, 0, 0, 0))
                            .onClick([] {
                            }).build();
                    buildDropdownItems(ui, stackName + ".item.", items, selected, colWidth, theme, onSelect);
                }).build();
    }
}

namespace app {
    const DslAppConfig &dslAppConfig() {
        static DslAppConfig config = DslAppConfig{}
                .title("AImer Launcher")
                .pageId("aimer_launcher")
                .clearColor({0.96f, 0.96f, 0.97f, 1.0f})
                .windowSize(kWindowWidth, kWindowHeight)
                .fps(kWindowFps);
        return config;
    }

    void compose(eui::Ui &ui, const eui::Screen &screen) {
        auto &launcher = Launcher::instance();

        auto theme = components::theme::light();
        auto accent = theme.primary;
        auto textPrimary = theme.text;
        auto textMuted = components::theme::withOpacity(theme.text, 0.60f);
        auto borderColor = components::theme::withOpacity(theme.border, 0.78f);
        auto errorColor = eui::Color{0.95f, 0.40f, 0.40f, 1.0f};

        const float contentWidth = screen.width - kMarginX * 2.0f;
        const float buttonWidth = (contentWidth - kButtonGap) * 0.5f;
        float currentY = kMainTitleTopGap;

        ui.text("title")
                .position(kMarginX, currentY)
                .size(contentWidth, kMainTitleLineHeight)
                .text("AImer Launcher")
                .fontSize(kMainTitleFontSize)
                .lineHeight(kMainTitleLineHeight)
                .fontWeight(900)
                .color(accent)
                .build();
        currentY += kMainTitleLineHeight + kMainTitleBottomGap;

        const float colWidth = (contentWidth - kColGap * 2.0f) / 3.0f;
        const float secondColX = kMarginX + colWidth + kColGap;
        const float thirdColX = secondColX + colWidth + kColGap;

        const float labelRowY = currentY;
        const float fieldRowY = labelRowY + kLabelHeight + kLabelFieldGap;

        ui.text("label.game")
                .position(kMarginX, labelRowY)
                .size(colWidth, kLabelHeight)
                .text("Game")
                .fontSize(kLabelFontSize)
                .lineHeight(kLabelHeight)
                .color(textMuted)
                .build();

        ui.text("label.target")
                .position(secondColX, labelRowY)
                .size(colWidth, kLabelHeight)
                .text("Target")
                .fontSize(kLabelFontSize)
                .lineHeight(kLabelHeight)
                .color(textMuted)
                .build();

        ui.text("label.sensitivity")
                .position(thirdColX, labelRowY)
                .size(colWidth, kLabelHeight)
                .text("Sensitivity")
                .fontSize(kLabelFontSize)
                .lineHeight(kLabelHeight)
                .color(textMuted)
                .build();

        ui.rect("game.dropdown.trigger")
                .position(kMarginX, fieldRowY)
                .size(colWidth, kFieldHeight)
                .states(theme.surface, theme.surfaceHover, theme.surfaceActive)
                .radius(kComponentRadius)
                .border(1.0f, borderColor)
                .onClick([&]() {
                    launcher.gameDropdownOpen.set(!launcher.gameDropdownOpen.get());
                    if (launcher.gameDropdownOpen.get()) launcher.targetDropdownOpen.set(false);
                })
                .build();

        ui.text("game.dropdown.label")
                .position(kMarginX + kItemTextPaddingX, fieldRowY)
                .size(colWidth, kFieldHeight)
                .text(launcher.getCurrentGameName())
                .fontSize(kDropdownFontSize)
                .lineHeight(kDropdownHeight)
                .color(textPrimary)
                .verticalAlign(core::VerticalAlign::Center)
                .build();

        ui.rect("target.dropdown.trigger")
                .position(secondColX, fieldRowY)
                .size(colWidth, kFieldHeight)
                .states(theme.surface, theme.surfaceHover, theme.surfaceActive)
                .radius(kComponentRadius)
                .border(1.0f, borderColor)
                .onClick([&]() {
                    launcher.targetDropdownOpen.set(!launcher.targetDropdownOpen.get());
                    if (launcher.targetDropdownOpen.get()) launcher.gameDropdownOpen.set(false);
                })
                .build();

        ui.text("target.dropdown.label")
                .position(secondColX + kItemTextPaddingX, fieldRowY)
                .size(colWidth, kFieldHeight)
                .text(launcher.getCurrentGameTargets()[launcher.selectedTarget])
                .fontSize(kDropdownFontSize)
                .lineHeight(kDropdownHeight)
                .color(textPrimary)
                .verticalAlign(core::VerticalAlign::Center)
                .build();

        components::input(ui, "sensitivity.input")
                .position(thirdColX, fieldRowY)
                .size(colWidth, kFieldHeight)
                .theme(theme)
                .value(launcher.sensitivityStr)
                .placeholder("e.g. 1.0")
                .onChange([&](const std::string &value) { launcher.sensitivityStr = value; })
                .build();

        if (launcher.gameDropdownOpen.get()) {
            const float popupH = kItemHeight * static_cast<float>(launcher.getGames().size()) + kPopupPadding * 2.0f;
            const float popupY = fieldRowY + kFieldHeight + kPopupGap;
            auto onSelect = [&](const int i) {
                launcher.selectedGame = i;
                launcher.sensitivityStr = std::to_string(Games::instance().getSettings(launcher.getGames()[i]).sensitivity);
                launcher.selectedTarget = 0;
                launcher.gameDropdownOpen.set(false);
                launcher.updateStatus("Game: " + launcher.getGames()[i], false);
            };
            buildDropdownPopup(ui, "game.dropdown.popup", kMarginX, popupY, colWidth, popupH,
                               theme, borderColor, launcher.getGames(), launcher.selectedGame, onSelect);
        }

        if (launcher.targetDropdownOpen.get()) {
            const float popupH = kItemHeight * static_cast<float>(launcher.getCurrentGameTargets().size())
                                 + kPopupPadding * 2.0f;
            const float popupY = fieldRowY + kFieldHeight + kPopupGap;
            auto onSelect = [&](const int i) {
                launcher.selectedTarget = i;
                launcher.targetDropdownOpen.set(false);
                launcher.updateStatus("Target: " + launcher.getCurrentGameTargets()[i], false);
            };
            buildDropdownPopup(ui, "target.dropdown.popup",
                               secondColX, popupY, colWidth, popupH,
                               theme, borderColor,
                               launcher.getCurrentGameTargets(),
                               launcher.selectedTarget, onSelect);
        }

        currentY += kFirstRowSectionHeight + kSectionGap; {
            const float inputW = contentWidth - kBrowseButtonWidth - kBrowseButtonGap;
            const float rowY = currentY + kLabelHeight + kLabelFieldGap;

            ui.text("label.model")
                    .position(kMarginX, currentY)
                    .size(contentWidth, kLabelHeight)
                    .text("Model Path (.onnx / .xml)")
                    .fontSize(kLabelFontSize)
                    .lineHeight(kLabelHeight)
                    .color(textMuted)
                    .build();

            components::input(ui, "model.input")
                    .position(kMarginX, rowY).size(inputW, kFieldHeight)
                    .theme(theme)
                    .fontSize(kInputFontSize)
                    .value(launcher.modelPath)
                    .placeholder("Path to model file")
                    .onChange([&](const std::string &v) { launcher.modelPath = v; })
                    .build();

            components::button(ui, "model.browse")
                    .position(kMarginX + inputW + kBrowseButtonGap, rowY).size(kBrowseButtonWidth, kFieldHeight)
                    .text("Browse")
                    .colors(components::theme::withAlpha(accent, 0.12f),
                            components::theme::withAlpha(accent, 0.20f),
                            components::theme::withAlpha(accent, 0.28f))
                    .textColor(accent).radius(kComponentRadius)
                    .border(1.0f, components::theme::withAlpha(accent, 0.40f))
                    .onClick([&]() {
                        core::platform::FileDialogOptions opts;
                        opts.prompt = "Select Model File";
                        opts.allowedExtensions = {"onnx", "xml"};
                        opts.filterName = "Model Files";
                        launcher.modelPath = core::platform::chooseFile(opts);
                    })
                    .build();
        }
        currentY += kLabelHeight + kLabelFieldGap + kFieldHeight + kSectionGap; {
            const float inputW = contentWidth - kBrowseButtonWidth - kBrowseButtonGap;
            const float rowY = currentY + kLabelHeight + kLabelFieldGap;

            ui.text("label.weights")
                    .position(kMarginX, currentY)
                    .size(contentWidth, kLabelHeight)
                    .text("Weights Path (.bin, optional)")
                    .fontSize(kLabelFontSize)
                    .lineHeight(kLabelHeight)
                    .color(textMuted)
                    .build();

            components::input(ui, "weights.input")
                    .position(kMarginX, rowY)
                    .size(inputW, kFieldHeight)
                    .theme(theme)
                    .fontSize(kInputFontSize)
                    .value(launcher.weightsPath)
                    .placeholder("Path to weights file (optional)")
                    .onChange([&](const std::string &v) { launcher.weightsPath = v; })
                    .build();

            components::button(ui, "weights.browse")
                    .position(kMarginX + inputW + kBrowseButtonGap, rowY).size(kBrowseButtonWidth, kFieldHeight)
                    .text("Browse")
                    .colors(components::theme::withAlpha(accent, 0.12f),
                            components::theme::withAlpha(accent, 0.20f),
                            components::theme::withAlpha(accent, 0.28f))
                    .textColor(accent).radius(kComponentRadius)
                    .border(1.0f, components::theme::withAlpha(accent, 0.40f))
                    .onClick([&]() {
                        core::platform::FileDialogOptions opts;
                        opts.prompt = "Select Weights File";
                        opts.allowedExtensions = {"bin"};
                        opts.filterName = "Weights Files";
                        launcher.weightsPath = core::platform::chooseFile(opts);
                    })
                    .build();
        }
        currentY += kLabelHeight + kLabelFieldGap + kFieldHeight + kSectionGap;

        if (!launcher.statusMsg.empty()) {
            ui.text("status")
                    .position(kMarginX, currentY)
                    .size(contentWidth, kLabelHeight)
                    .text(launcher.statusIsError ? launcher.statusMsg : "")
                    .fontSize(kLabelFontSize)
                    .lineHeight(kLabelHeight)
                    .color(errorColor)
                    .build();
        }
        float buttonY = screen.height - kButtonHeight - 10.0f;

        eui::Color hoverAccent = eui::mixColor(accent, eui::Color{1.0f, 1.0f, 1.0f, accent.a}, 0.12f);
        eui::Color pressAccent = eui::mixColor(accent, eui::Color{0.0f, 0.0f, 0.0f, accent.a}, 0.24f);
        auto btnShadow = components::theme::popupShadow(theme);

        components::button(ui, "btn.cancel")
                .position(kMarginX, buttonY)
                .size(buttonWidth, kButtonHeight)
                .text("Cancel")
                .colors(eui::Color{0.0f, 0.0f, 0.0f, 0.0f},
                        components::theme::withAlpha(accent, 0.08f),
                        components::theme::withAlpha(accent, 0.16f))
                .textColor(textPrimary)
                .iconColor(textMuted)
                .radius(kComponentRadius)
                .border(1.0f, borderColor)
                .onClick([&]() {
                        launcher.shouldCancel = true;
                        launcher.shouldLaunch = false;
                })
                .build();

        components::button(ui, "btn.launch")
                .position(kMarginX + buttonWidth + kButtonGap, buttonY)
                .size(buttonWidth, kButtonHeight)
                .text("Launch AImer")
                .colors(accent, hoverAccent, pressAccent)
                .textColor(eui::Color{1.0f, 1.0f, 1.0f, 1.0f})
                .iconColor(eui::Color{1.0f, 1.0f, 1.0f, 1.0f})
                .radius(kComponentRadius)
                .border(1.0f, components::theme::withAlpha(accent, 0.58f))
                .shadow(btnShadow.blur, btnShadow.offset.x, btnShadow.offset.y, btnShadow.color)
                .onClick([&]() {
                        launcher.validateAndLaunch();
                })
                .build();
    }
} // namespace app
