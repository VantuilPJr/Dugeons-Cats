#pragma once

#include "dungeon/Dungeon.h"
#include "dungeon/DungeonGenerator.h"
#include "entities/CarameloBoss.h"
#include "entities/Enemy.h"
#include "entities/Player.h"
#include "input/InputManager.h"
#include "systems/AudioSystem.h"
#include "systems/WorldObjects.h"

#include <raylib.h>

#include <cstdint>
#include <random>
#include <string>
#include <vector>

enum class GameState
{
    Title,
    Playing,
    Paused,
    GameOver,
    Victory,
};

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
    void startNewRun();
    void populateDungeon();
    void update(float deltaTime);
    void updatePlaying(float deltaTime);
    void processCombat();
    void processPickups();
    void processInteractions();
    void updateParticles(float deltaTime);
    void updateCamera();

    void render();
    void renderTitle() const;
    void renderWorld() const;
    void renderDungeon() const;
    void renderObjects() const;
    void renderEnemies() const;
    void renderBoss() const;
    void renderPlayer() const;
    void renderParticles() const;
    void renderHud() const;
    void renderDebugPanel() const;
    void renderStateOverlay() const;
    void renderToWindow() const;

    void spawnParticles(Vector2 position, Color color, int count, float speed = 45.0F);
    void spawnLoot(Vector2 position);
    void setToast(std::string text, float duration = 2.0F);
    [[nodiscard]] Vector2 roomWorldCenter(int roomId) const;

    InputManager input_;
    AudioSystem audio_;
    RenderTexture2D renderTarget_{};
    Texture2D titleBackground_{};
    Camera2D camera_{};
    DungeonGenerator generator_;
    Dungeon dungeon_;
    Player player_;
    CarameloBoss boss_;
    std::vector<Enemy> enemies_;
    std::vector<WorldItem> worldItems_;
    std::vector<TrashCan> trashCans_;
    std::vector<Scratcher> scratchers_;
    std::vector<Particle> particles_;
    std::mt19937 random_;

    GameState state_ = GameState::Title;
    std::uint32_t seed_ = 0;
    int currentRoomId_ = -1;
    std::string toast_;
    float toastTimer_ = 0.0F;
    float screenShake_ = 0.0F;
    bool initialized_ = false;
    bool showDebug_ = false;
};
