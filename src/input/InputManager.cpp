#include "input/InputManager.h"

#include <algorithm>
#include <cmath>

void InputManager::update()
{
    if (activeGamepad_ >= 0 && !IsGamepadAvailable(activeGamepad_))
    {
        activeGamepad_ = -1;
    }

    if (activeGamepad_ < 0)
    {
        for (int index = 0; index < kMaxScannedGamepads; ++index)
        {
            if (IsGamepadAvailable(index))
            {
                activeGamepad_ = index;
                break;
            }
        }
    }

    if (!isGamepadConnected())
    {
        lastPressedButton_ = -1;
        return;
    }

    const int pressedButton = GetGamepadButtonPressed();
    if (pressedButton != GAMEPAD_BUTTON_UNKNOWN)
    {
        lastPressedButton_ = pressedButton;
    }
}

Vector2 InputManager::movement() const
{
    Vector2 result{};
    result.x += isActionDown(GameAction::MoveRight) ? 1.0F : 0.0F;
    result.x -= isActionDown(GameAction::MoveLeft) ? 1.0F : 0.0F;
    result.y += isActionDown(GameAction::MoveDown) ? 1.0F : 0.0F;
    result.y -= isActionDown(GameAction::MoveUp) ? 1.0F : 0.0F;

    const Vector2 analog = analogMovement();
    result.x += analog.x;
    result.y += analog.y;

    const float magnitude = std::sqrt(result.x * result.x + result.y * result.y);
    if (magnitude > 1.0F)
    {
        result.x /= magnitude;
        result.y /= magnitude;
    }

    return result;
}

bool InputManager::isActionDown(const GameAction action) const
{
    return queryAction(action, InputState::Down);
}

bool InputManager::isActionPressed(const GameAction action) const
{
    return queryAction(action, InputState::Pressed);
}

bool InputManager::isActionReleased(const GameAction action) const
{
    return queryAction(action, InputState::Released);
}

bool InputManager::isGamepadConnected() const
{
    return activeGamepad_ >= 0 && IsGamepadAvailable(activeGamepad_);
}

int InputManager::gamepadIndex() const
{
    return activeGamepad_;
}

const char* InputManager::gamepadName() const
{
    if (!isGamepadConnected())
    {
        return "Nenhum";
    }

    const char* name = GetGamepadName(activeGamepad_);
    return name != nullptr ? name : "Desconhecido";
}

int InputManager::axisCount() const
{
    return isGamepadConnected() ? GetGamepadAxisCount(activeGamepad_) : 0;
}

float InputManager::rawAxis(const int axis) const
{
    if (!isGamepadConnected() || axis < 0 || axis >= axisCount())
    {
        return 0.0F;
    }

    return GetGamepadAxisMovement(activeGamepad_, axis);
}

bool InputManager::isRawButtonDown(const int button) const
{
    return isGamepadConnected() && IsGamepadButtonDown(activeGamepad_, button);
}

bool InputManager::isRawButtonPressed(const int button) const
{
    return isGamepadConnected() && IsGamepadButtonPressed(activeGamepad_, button);
}

bool InputManager::isRawButtonReleased(const int button) const
{
    return isGamepadConnected() && IsGamepadButtonReleased(activeGamepad_, button);
}

int InputManager::lastPressedButton() const
{
    return lastPressedButton_;
}

void InputManager::setDeadZone(const float deadZone)
{
    deadZone_ = std::clamp(deadZone, 0.0F, 0.95F);
}

float InputManager::deadZone() const
{
    return deadZone_;
}

bool InputManager::queryAction(const GameAction action, const InputState state) const
{
    switch (action)
    {
    case GameAction::MoveUp:
        return queryKeyboard(KEY_W, state) || queryKeyboard(KEY_UP, state)
            || queryGamepadButton(GAMEPAD_BUTTON_LEFT_FACE_UP, state);
    case GameAction::MoveDown:
        return queryKeyboard(KEY_S, state) || queryKeyboard(KEY_DOWN, state)
            || queryGamepadButton(GAMEPAD_BUTTON_LEFT_FACE_DOWN, state);
    case GameAction::MoveLeft:
        return queryKeyboard(KEY_A, state) || queryKeyboard(KEY_LEFT, state)
            || queryGamepadButton(GAMEPAD_BUTTON_LEFT_FACE_LEFT, state);
    case GameAction::MoveRight:
        return queryKeyboard(KEY_D, state) || queryKeyboard(KEY_RIGHT, state)
            || queryGamepadButton(GAMEPAD_BUTTON_LEFT_FACE_RIGHT, state);
    case GameAction::Attack:
        return queryKeyboard(KEY_SPACE, state) || queryKeyboard(KEY_J, state)
            || queryGamepadButton(GAMEPAD_BUTTON_RIGHT_FACE_LEFT, state);
    case GameAction::Dodge:
        return queryKeyboard(KEY_LEFT_SHIFT, state) || queryKeyboard(KEY_RIGHT_SHIFT, state)
            || queryGamepadButton(GAMEPAD_BUTTON_RIGHT_FACE_DOWN, state);
    case GameAction::Interact:
        return queryKeyboard(KEY_E, state) || queryGamepadButton(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, state);
    case GameAction::UseSachet:
        return queryKeyboard(KEY_Q, state) || queryGamepadButton(GAMEPAD_BUTTON_LEFT_TRIGGER_1, state);
    case GameAction::UseCatnip:
        return queryKeyboard(KEY_R, state) || queryGamepadButton(GAMEPAD_BUTTON_RIGHT_TRIGGER_1, state);
    case GameAction::Inventory:
        return queryKeyboard(KEY_TAB, state) || queryKeyboard(KEY_I, state)
            || queryGamepadButton(GAMEPAD_BUTTON_RIGHT_FACE_UP, state);
    case GameAction::Pause:
        return queryKeyboard(KEY_ESCAPE, state) || queryGamepadButton(GAMEPAD_BUTTON_MIDDLE_RIGHT, state);
    case GameAction::ToggleDebug:
        return queryKeyboard(KEY_F3, state) || queryGamepadButton(GAMEPAD_BUTTON_MIDDLE_LEFT, state);
    }

    return false;
}

bool InputManager::queryKeyboard(const int key, const InputState state) const
{
    switch (state)
    {
    case InputState::Down:
        return IsKeyDown(key);
    case InputState::Pressed:
        return IsKeyPressed(key);
    case InputState::Released:
        return IsKeyReleased(key);
    }

    return false;
}

bool InputManager::queryGamepadButton(const int button, const InputState state) const
{
    if (!isGamepadConnected())
    {
        return false;
    }

    switch (state)
    {
    case InputState::Down:
        return IsGamepadButtonDown(activeGamepad_, button);
    case InputState::Pressed:
        return IsGamepadButtonPressed(activeGamepad_, button);
    case InputState::Released:
        return IsGamepadButtonReleased(activeGamepad_, button);
    }

    return false;
}

Vector2 InputManager::analogMovement() const
{
    if (!isGamepadConnected())
    {
        return Vector2{};
    }

    Vector2 analog{
        GetGamepadAxisMovement(activeGamepad_, GAMEPAD_AXIS_LEFT_X),
        GetGamepadAxisMovement(activeGamepad_, GAMEPAD_AXIS_LEFT_Y),
    };

    const float magnitude = std::sqrt(analog.x * analog.x + analog.y * analog.y);
    if (magnitude <= deadZone_)
    {
        return Vector2{};
    }

    const float normalizedX = analog.x / magnitude;
    const float normalizedY = analog.y / magnitude;
    const float remappedMagnitude = std::min((magnitude - deadZone_) / (1.0F - deadZone_), 1.0F);
    return Vector2{normalizedX * remappedMagnitude, normalizedY * remappedMagnitude};
}
