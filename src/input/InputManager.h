#pragma once

#include "input/GameAction.h"

#include <raylib.h>

class InputManager
{
public:
    static constexpr int kDiagnosticButtonCount = 18;

    void update();

    [[nodiscard]] Vector2 movement() const;
    [[nodiscard]] bool isActionDown(GameAction action) const;
    [[nodiscard]] bool isActionPressed(GameAction action) const;
    [[nodiscard]] bool isActionReleased(GameAction action) const;

    [[nodiscard]] bool isGamepadConnected() const;
    [[nodiscard]] int gamepadIndex() const;
    [[nodiscard]] const char* gamepadName() const;
    [[nodiscard]] int axisCount() const;
    [[nodiscard]] float rawAxis(int axis) const;
    [[nodiscard]] bool isRawButtonDown(int button) const;
    [[nodiscard]] bool isRawButtonPressed(int button) const;
    [[nodiscard]] bool isRawButtonReleased(int button) const;
    [[nodiscard]] int lastPressedButton() const;

    void setDeadZone(float deadZone);
    [[nodiscard]] float deadZone() const;

private:
    enum class InputState
    {
        Down,
        Pressed,
        Released,
    };

    [[nodiscard]] bool queryAction(GameAction action, InputState state) const;
    [[nodiscard]] bool queryKeyboard(int key, InputState state) const;
    [[nodiscard]] bool queryGamepadButton(int button, InputState state) const;
    [[nodiscard]] Vector2 analogMovement() const;

    static constexpr int kMaxScannedGamepads = 4;

    int activeGamepad_ = -1;
    int lastPressedButton_ = -1;
    float deadZone_ = 0.20F;
};

