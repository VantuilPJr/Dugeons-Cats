#include "core/Game.h"

#include "core/Constants.h"
#include "input/GameAction.h"

#include <algorithm>
#include <cmath>

Game::~Game()
{
    if (renderTarget_.id != 0U)
    {
        UnloadRenderTexture(renderTarget_);
    }

    if (initialized_)
    {
        CloseWindow();
    }
}

int Game::run()
{
    initialize();

    while (!WindowShouldClose())
    {
        input_.update();
        update(std::min(GetFrameTime(), 1.0F / 15.0F));
        render();
    }

    return 0;
}

void Game::initialize()
{
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(constants::kWindowWidth, constants::kWindowHeight, "Dungeons & Cats - Input Milestone");
    initialized_ = true;
    SetTargetFPS(constants::kTargetFps);

    renderTarget_ = LoadRenderTexture(constants::kInternalWidth, constants::kInternalHeight);
    SetTextureFilter(renderTarget_.texture, TEXTURE_FILTER_POINT);

    player_ = Rectangle{
        (constants::kInternalWidth - constants::kPlayerSize) * 0.5F,
        (constants::kInternalHeight - constants::kPlayerSize) * 0.5F,
        constants::kPlayerSize,
        constants::kPlayerSize,
    };

    input_.setDeadZone(constants::kGamepadDeadZone);
}

void Game::update(const float deltaTime)
{
    if (input_.isActionPressed(GameAction::ToggleDebug))
    {
        showDebug_ = !showDebug_;
    }

    const Vector2 movement = input_.movement();
    player_.x += movement.x * constants::kPlayerSpeed * deltaTime;
    player_.y += movement.y * constants::kPlayerSpeed * deltaTime;

    player_.x = std::clamp(player_.x, 0.0F, constants::kInternalWidth - player_.width);
    player_.y = std::clamp(player_.y, 0.0F, constants::kInternalHeight - player_.height);
}

void Game::render()
{
    BeginTextureMode(renderTarget_);
    ClearBackground(Color{25, 28, 33, 255});
    renderWorld();
    if (showDebug_)
    {
        renderDebugPanel();
    }
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    renderToWindow();
    EndDrawing();
}

void Game::renderWorld() const
{
    constexpr int gridSize = 32;
    const Color gridColor{42, 46, 53, 255};

    for (int x = 0; x <= constants::kInternalWidth; x += gridSize)
    {
        DrawLine(x, 0, x, constants::kInternalHeight, gridColor);
    }

    for (int y = 0; y <= constants::kInternalHeight; y += gridSize)
    {
        DrawLine(0, y, constants::kInternalWidth, y, gridColor);
    }

    DrawRectangleRec(player_, Color{236, 121, 39, 255});
    DrawRectangleLinesEx(player_, 1.0F, Color{255, 190, 92, 255});

    const char* connectionStatus = input_.isGamepadConnected() ? "GAMEPAD: CONECTADO" : "GAMEPAD: NAO CONECTADO";
    const Color statusColor = input_.isGamepadConnected() ? Color{94, 221, 134, 255} : Color{255, 184, 76, 255};
    DrawRectangle(6, constants::kInternalHeight - 22, 158, 16, Fade(BLACK, 0.78F));
    DrawText(connectionStatus, 11, constants::kInternalHeight - 18, 8, statusColor);
}

void Game::renderDebugPanel() const
{
    DrawRectangle(6, 6, constants::kInternalWidth - 12, constants::kInternalHeight - 34, Fade(BLACK, 0.88F));
    DrawRectangleLines(6, 6, constants::kInternalWidth - 12, constants::kInternalHeight - 34, Color{89, 104, 120, 255});

    DrawText("DIAGNOSTICO DE INPUT", 16, 14, 12, Color{255, 190, 92, 255});
    DrawText(TextFormat("Gamepad conectado: %s", input_.isGamepadConnected() ? "SIM" : "NAO"), 16, 34, 10, RAYWHITE);
    DrawText(TextFormat("Indice ativo: %d", input_.gamepadIndex()), 16, 50, 10, LIGHTGRAY);
    DrawText(TextFormat("Nome: %.45s", input_.gamepadName()), 16, 66, 10, LIGHTGRAY);
    DrawText(TextFormat("Dead zone: %.2f", input_.deadZone()), 16, 82, 10, LIGHTGRAY);

    const Vector2 movement = input_.movement();
    DrawText(TextFormat("Movimento: X % .2f  Y % .2f", movement.x, movement.y), 16, 98, 10, LIGHTGRAY);
    DrawText(TextFormat("Player: X %3.0f  Y %3.0f", player_.x, player_.y), 16, 114, 10, LIGHTGRAY);

    DrawText(TextFormat("EIXOS (%d)", input_.axisCount()), 16, 138, 10, Color{126, 200, 255, 255});
    const int displayedAxes = std::min(input_.axisCount(), 10);
    for (int axis = 0; axis < displayedAxes; ++axis)
    {
        DrawText(TextFormat("Axis %02d: % .3f", axis, input_.rawAxis(axis)), 16, 154 + axis * 14, 10, LIGHTGRAY);
    }

    DrawText("BOTOES FISICOS RAYLIB", 326, 14, 10, Color{126, 200, 255, 255});
    DrawText("P=pressionado H=mantido R=solto", 326, 28, 8, GRAY);

    for (int button = 0; button < InputManager::kDiagnosticButtonCount; ++button)
    {
        const int column = button / 9;
        const int row = button % 9;
        const int x = 326 + column * 148;
        const int y = 48 + row * 20;

        const bool pressed = input_.isRawButtonPressed(button);
        const bool down = input_.isRawButtonDown(button);
        const bool released = input_.isRawButtonReleased(button);
        const char state = pressed ? 'P' : (down ? 'H' : (released ? 'R' : '-'));
        const Color stateColor = down ? Color{94, 221, 134, 255} : LIGHTGRAY;
        DrawText(TextFormat("Button %02d: %c", button, state), x, y, 10, stateColor);
    }

    DrawText(TextFormat("Ultimo pressionado: %d", input_.lastPressedButton()), 326, 238, 10, LIGHTGRAY);
    DrawText("F3 ou SELECT: mostrar/ocultar painel", 326, 258, 9, Color{255, 190, 92, 255});
    DrawText("WASD / SETAS / ANALOGICO ESQ. / D-PAD", 326, 276, 9, LIGHTGRAY);
    DrawText("ESC: sair desta versao", 326, 294, 9, GRAY);
}

void Game::renderToWindow() const
{
    const float scaleX = static_cast<float>(GetScreenWidth()) / constants::kInternalWidth;
    const float scaleY = static_cast<float>(GetScreenHeight()) / constants::kInternalHeight;
    const float scale = std::min(scaleX, scaleY);
    const float destinationWidth = constants::kInternalWidth * scale;
    const float destinationHeight = constants::kInternalHeight * scale;
    const float offsetX = (GetScreenWidth() - destinationWidth) * 0.5F;
    const float offsetY = (GetScreenHeight() - destinationHeight) * 0.5F;

    const Rectangle source{0.0F, 0.0F, static_cast<float>(constants::kInternalWidth), -static_cast<float>(constants::kInternalHeight)};
    const Rectangle destination{offsetX, offsetY, destinationWidth, destinationHeight};
    DrawTexturePro(renderTarget_.texture, source, destination, Vector2{}, 0.0F, WHITE);
}

