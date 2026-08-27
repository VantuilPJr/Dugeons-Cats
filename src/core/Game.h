#pragma once

#include "input/InputManager.h"

#include <raylib.h>

class Game
{
public:
    Game() = default;
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    int run();

private:
    void initialize();
    void update(float deltaTime);
    void render();
    void renderWorld() const;
    void renderDebugPanel() const;
    void renderToWindow() const;

    InputManager input_;
    RenderTexture2D renderTarget_{};
    Rectangle player_{};
    bool initialized_ = false;
    bool showDebug_ = true;
};

