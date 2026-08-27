#include "core/Game.h"

#include "core/Constants.h"
#include "input/GameAction.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>

namespace
{
float distanceSquared(const Vector2 first, const Vector2 second)
{
    const float x = first.x - second.x;
    const float y = first.y - second.y;
    return x * x + y * y;
}

Vector2 normalizedDirection(const Vector2 from, const Vector2 to)
{
    const float x = to.x - from.x;
    const float y = to.y - from.y;
    const float length = std::sqrt(x * x + y * y);
    return length > 0.001F ? Vector2{x / length, y / length} : Vector2{1.0F, 0.0F};
}

Color itemColor(const ItemType type)
{
    switch (type)
    {
    case ItemType::Sachet: return Color{242, 91, 131, 255};
    case ItemType::Catnip: return Color{91, 214, 120, 255};
    case ItemType::Coin: return Color{244, 199, 70, 255};
    }
    return WHITE;
}
} // namespace

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
    InitWindow(constants::kWindowWidth, constants::kWindowHeight, "Dungeons & Cats");
    initialized_ = true;
    SetExitKey(KEY_NULL);
    SetTargetFPS(constants::kTargetFps);

    renderTarget_ = LoadRenderTexture(constants::kInternalWidth, constants::kInternalHeight);
    SetTextureFilter(renderTarget_.texture, TEXTURE_FILTER_POINT);
    input_.setDeadZone(constants::kGamepadDeadZone);

    camera_.offset = Vector2{constants::kInternalWidth * 0.5F, constants::kInternalHeight * 0.5F};
    camera_.zoom = 1.0F;
    seed_ = static_cast<std::uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    random_.seed(seed_);
}

void Game::startNewRun()
{
    ++seed_;
    random_.seed(seed_);
    dungeon_ = generator_.generate(seed_);
    player_.reset(roomWorldCenter(dungeon_.startRoomId()));
    populateDungeon();
    particles_.clear();
    currentRoomId_ = dungeon_.startRoomId();
    screenShake_ = 0.0F;
    setToast("Explore as ruas. Encontre o Rei das Ruas.", 3.0F);
    state_ = GameState::Playing;
    updateCamera();
}

void Game::populateDungeon()
{
    enemies_.clear();
    worldItems_.clear();
    trashCans_.clear();
    scratchers_.clear();

    for (const Room& room : dungeon_.rooms())
    {
        if (room.type == RoomType::Start || room.type == RoomType::Boss)
        {
            continue;
        }

        const Vector2 center = roomWorldCenter(room.id);
        if (room.type == RoomType::Combat)
        {
            enemies_.emplace_back(EnemyType::Rat, Vector2{center.x - 20.0F, center.y}, room.id);
            enemies_.emplace_back(EnemyType::Rat, Vector2{center.x + 22.0F, center.y + 12.0F}, room.id);
        }
        else if (room.type == RoomType::Treasure)
        {
            trashCans_.push_back(TrashCan{center});
            enemies_.emplace_back(EnemyType::Rat, Vector2{center.x + 28.0F, center.y}, room.id);
        }
        else if (room.type == RoomType::Scratcher)
        {
            scratchers_.push_back(Scratcher{center});
            enemies_.emplace_back(EnemyType::Rat, Vector2{center.x - 25.0F, center.y + 8.0F}, room.id);
        }
        else if ((room.id & 1) == 0)
        {
            enemies_.emplace_back(EnemyType::Rat, center, room.id);
        }
    }
}

void Game::update(const float deltaTime)
{
    if (input_.isActionPressed(GameAction::ToggleDebug))
    {
        showDebug_ = !showDebug_;
    }

    switch (state_)
    {
    case GameState::Title:
        if (IsKeyPressed(KEY_ENTER) || input_.isActionPressed(GameAction::Attack))
        {
            startNewRun();
        }
        break;
    case GameState::Playing:
        if (input_.isActionPressed(GameAction::Pause))
        {
            state_ = GameState::Paused;
        }
        else
        {
            updatePlaying(deltaTime);
        }
        break;
    case GameState::Paused:
        if (input_.isActionPressed(GameAction::Pause) || input_.isActionPressed(GameAction::Dodge))
        {
            state_ = GameState::Playing;
        }
        break;
    case GameState::GameOver:
    case GameState::Victory:
        if (IsKeyPressed(KEY_ENTER) || input_.isActionPressed(GameAction::Attack))
        {
            startNewRun();
        }
        break;
    }
}

void Game::updatePlaying(const float deltaTime)
{
    toastTimer_ = std::max(0.0F, toastTimer_ - deltaTime);
    screenShake_ = std::max(0.0F, screenShake_ - deltaTime);

    if (input_.isActionPressed(GameAction::UseSachet) && player_.useSachet())
    {
        setToast("Sache consumido: HP recuperado");
        spawnParticles(player_.position(), Color{242, 91, 131, 255}, 12);
    }
    if (input_.isActionPressed(GameAction::UseCatnip) && player_.useCatnip())
    {
        setToast("FURIA FELINA!");
        spawnParticles(player_.position(), Color{91, 214, 120, 255}, 20, 70.0F);
    }

    player_.update(input_, dungeon_, deltaTime);
    currentRoomId_ = dungeon_.roomAtWorld(player_.position().x, player_.position().y);

    for (Enemy& enemy : enemies_)
    {
        if (!enemy.isDead() && (enemy.roomId() == currentRoomId_ || distanceSquared(enemy.position(), player_.position()) < 240.0F * 240.0F))
        {
            enemy.update(player_.position(), dungeon_, deltaTime);
        }

        if (enemy.canDealDamage() && CheckCollisionRecs(enemy.attackCollider(), player_.collider()))
        {
            enemy.markDamageApplied();
            if (player_.takeDamage(enemy.damage()))
            {
                spawnParticles(player_.position(), Color{226, 71, 64, 255}, 12, 80.0F);
                screenShake_ = 0.18F;
                setToast(TextFormat("-%d HP", enemy.damage()), 0.9F);
            }
        }
    }

    processCombat();
    processPickups();
    processInteractions();
    updateParticles(deltaTime);
    updateCamera();

    if (player_.isDead())
    {
        state_ = GameState::GameOver;
    }
}

void Game::processCombat()
{
    if (!player_.attackStarted())
    {
        return;
    }

    bool hitSomething = false;
    for (Enemy& enemy : enemies_)
    {
        if (enemy.isDead() || !CheckCollisionRecs(player_.attackHitbox(), enemy.collider()))
        {
            continue;
        }

        bool critical = false;
        const int damage = player_.rollAttackDamage(random_, critical);
        const Vector2 knockback = normalizedDirection(player_.position(), enemy.position());
        const bool killed = enemy.takeDamage(damage, knockback, dungeon_);
        hitSomething = true;
        spawnParticles(enemy.position(), critical ? YELLOW : Color{255, 174, 79, 255}, critical ? 16 : 8, critical ? 90.0F : 55.0F);
        setToast(critical ? TextFormat("CRITICO! %d", damage) : TextFormat("Dano: %d", damage), 0.75F);
        if (killed && !enemy.dropProcessed())
        {
            enemy.markDropProcessed();
            spawnLoot(enemy.position());
        }
    }

    const Vector2 slashPosition{player_.position().x + player_.direction().x * 15.0F, player_.position().y + player_.direction().y * 15.0F};
    spawnParticles(slashPosition, Color{255, 220, 152, 255}, hitSomething ? 6 : 3, 35.0F);
}

void Game::processPickups()
{
    for (WorldItem& item : worldItems_)
    {
        if (item.active && distanceSquared(item.position, player_.position()) <= 21.0F * 21.0F)
        {
            item.active = false;
            player_.inventory().add(item.type);
            setToast(std::string(itemName(item.type)) + " coletado");
            spawnParticles(item.position, itemColor(item.type), 10);
        }
    }

    worldItems_.erase(std::remove_if(worldItems_.begin(), worldItems_.end(), [](const WorldItem& item) {
        return !item.active;
    }), worldItems_.end());
}

void Game::processInteractions()
{
    if (!input_.isActionPressed(GameAction::Interact))
    {
        return;
    }

    for (TrashCan& trash : trashCans_)
    {
        if (!trash.opened && distanceSquared(trash.position, player_.position()) < 34.0F * 34.0F)
        {
            trash.opened = true;
            std::uniform_int_distribution<int> lootRoll(0, 99);
            const int roll = lootRoll(random_);
            if (roll < 55)
            {
                worldItems_.push_back(WorldItem{ItemType::Sachet, Vector2{trash.position.x + 14.0F, trash.position.y}});
                setToast("A lata tinha um sache!");
            }
            else if (roll < 80)
            {
                worldItems_.push_back(WorldItem{ItemType::Catnip, Vector2{trash.position.x + 14.0F, trash.position.y}});
                setToast("A lata tinha catnip!");
            }
            else if (roll < 94)
            {
                worldItems_.push_back(WorldItem{ItemType::Coin, Vector2{trash.position.x + 14.0F, trash.position.y}});
                setToast("Uma tampinha brilhante!");
            }
            else
            {
                setToast("A lata estava vazia...");
            }
            spawnParticles(trash.position, GRAY, 12);
            return;
        }
    }

    for (Scratcher& scratcher : scratchers_)
    {
        if (!scratcher.used && distanceSquared(scratcher.position, player_.position()) < 36.0F * 36.0F)
        {
            scratcher.used = true;
            player_.addPermanentAttack(2);
            setToast("Unhas afiadas! ATQ +2", 2.5F);
            spawnParticles(scratcher.position, Color{255, 174, 79, 255}, 18, 65.0F);
            return;
        }
    }
}

void Game::updateParticles(const float deltaTime)
{
    for (Particle& particle : particles_)
    {
        particle.life -= deltaTime;
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        particle.velocity.x *= std::pow(0.08F, deltaTime);
        particle.velocity.y *= std::pow(0.08F, deltaTime);
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(), [](const Particle& particle) {
        return particle.life <= 0.0F;
    }), particles_.end());
}

void Game::updateCamera()
{
    constexpr float halfWidth = constants::kInternalWidth * 0.5F;
    constexpr float halfHeight = constants::kInternalHeight * 0.5F;
    const float worldWidth = static_cast<float>(Dungeon::kWidth * Dungeon::kTileSize);
    const float worldHeight = static_cast<float>(Dungeon::kHeight * Dungeon::kTileSize);
    camera_.target.x = std::clamp(player_.position().x, halfWidth, worldWidth - halfWidth);
    camera_.target.y = std::clamp(player_.position().y, halfHeight, worldHeight - halfHeight);

    if (screenShake_ > 0.0F)
    {
        std::uniform_real_distribution<float> shake(-2.5F, 2.5F);
        camera_.offset.x = halfWidth + shake(random_);
        camera_.offset.y = halfHeight + shake(random_);
    }
    else
    {
        camera_.offset = Vector2{halfWidth, halfHeight};
    }
}

void Game::render()
{
    BeginTextureMode(renderTarget_);
    ClearBackground(Color{16, 18, 24, 255});
    if (state_ == GameState::Title)
    {
        renderTitle();
    }
    else
    {
        renderWorld();
        renderHud();
        if (showDebug_)
        {
            renderDebugPanel();
        }
        renderStateOverlay();
    }
    EndTextureMode();

    BeginDrawing();
    ClearBackground(BLACK);
    renderToWindow();
    EndDrawing();
}

void Game::renderTitle() const
{
    DrawRectangleGradientV(0, 0, constants::kInternalWidth, constants::kInternalHeight, Color{19, 27, 42, 255}, Color{48, 29, 42, 255});
    DrawCircle(525, 70, 48.0F, Color{239, 198, 104, 255});
    DrawRectangle(0, 235, constants::kInternalWidth, 125, Color{24, 25, 31, 255});
    for (int building = 0; building < 12; ++building)
    {
        const int x = building * 58 - 12;
        const int height = 55 + (building * 23 % 70);
        DrawRectangle(x, 235 - height, 50, height, Color{31, 35, 48, 255});
        for (int window = 0; window < 3; ++window)
        {
            DrawRectangle(x + 8 + window * 13, 246 - height, 6, 9, Color{226, 169, 77, 180});
        }
    }
    DrawRectangle(0, 292, constants::kInternalWidth, 68, Color{37, 39, 43, 255});
    DrawLine(0, 319, constants::kInternalWidth, 319, Color{205, 177, 84, 190});
    DrawText("DUNGEONS", 43, 40, 48, Color{255, 183, 69, 255});
    DrawText("& CATS", 95, 91, 44, Color{255, 221, 151, 255});
    DrawText("UM ROGUELITE PELAS RUAS", 49, 143, 13, Color{181, 195, 214, 255});

    DrawRectangle(46, 189, 270, 29, Fade(BLACK, 0.55F));
    DrawText("ENTER / QUADRADO: NOVA RUN", 61, 198, 12, RAYWHITE);
    DrawText("WASD  mover   J/ESPACO  atacar   E  interagir", 43, 250, 10, LIGHTGRAY);
    DrawText("SHIFT  esquiva   Q  sache   R  catnip   ESC  pause", 43, 266, 10, LIGHTGRAY);
    DrawText("F3  diagnostico do controle", 43, 282, 10, Color{117, 201, 255, 255});

    const float catX = 503.0F;
    const float catY = 285.0F;
    DrawCircle(static_cast<int>(catX), static_cast<int>(catY), 17.0F, Color{230, 112, 35, 255});
    DrawTriangle(Vector2{catX - 14, catY - 11}, Vector2{catX - 6, catY - 28}, Vector2{catX - 1, catY - 12}, Color{238, 133, 48, 255});
    DrawTriangle(Vector2{catX + 2, catY - 12}, Vector2{catX + 10, catY - 28}, Vector2{catX + 15, catY - 9}, Color{238, 133, 48, 255});
    DrawCircle(static_cast<int>(catX - 6), static_cast<int>(catY - 2), 2.0F, Color{49, 50, 38, 255});
    DrawCircle(static_cast<int>(catX + 6), static_cast<int>(catY - 2), 2.0F, Color{49, 50, 38, 255});
    DrawLineEx(Vector2{catX + 14, catY + 8}, Vector2{catX + 34, catY + 2}, 4.0F, Color{218, 96, 30, 255});
}

void Game::renderWorld() const
{
    BeginMode2D(camera_);
    renderDungeon();
    renderObjects();
    renderEnemies();
    renderPlayer();
    renderParticles();
    EndMode2D();
}

void Game::renderDungeon() const
{
    for (int y = 0; y < Dungeon::kHeight; ++y)
    {
        for (int x = 0; x < Dungeon::kWidth; ++x)
        {
            const TileType tile = dungeon_.tileAt(x, y);
            const int worldX = x * Dungeon::kTileSize;
            const int worldY = y * Dungeon::kTileSize;
            const unsigned int pattern = static_cast<unsigned int>(x * 73856093) ^ static_cast<unsigned int>(y * 19349663) ^ seed_;

            if (tile == TileType::Floor)
            {
                const Color base = (pattern & 1U) == 0U ? Color{61, 64, 69, 255} : Color{65, 68, 73, 255};
                DrawRectangle(worldX, worldY, Dungeon::kTileSize, Dungeon::kTileSize, base);
                if (pattern % 17U == 0U)
                {
                    DrawLine(worldX + 3, worldY + 8, worldX + 10, worldY + 11, Color{45, 48, 52, 255});
                }
                if (pattern % 41U == 0U)
                {
                    DrawRectangle(worldX + 2, worldY + 2, 3, 2, Color{79, 96, 74, 255});
                }
            }
            else if (tile == TileType::Wall)
            {
                DrawRectangle(worldX, worldY, Dungeon::kTileSize, Dungeon::kTileSize, Color{69, 53, 57, 255});
                DrawRectangle(worldX, worldY, Dungeon::kTileSize, 4, Color{103, 75, 71, 255});
                DrawLine(worldX, worldY + 8, worldX + Dungeon::kTileSize, worldY + 8, Color{45, 39, 44, 255});
                DrawLine(worldX + (((y & 1) != 0) ? 4 : 11), worldY + 8, worldX + (((y & 1) != 0) ? 4 : 11), worldY + 15, Color{45, 39, 44, 255});
                if (pattern % 53U == 0U)
                {
                    DrawRectangle(worldX + 2, worldY + 5, 11, 5, Color{42, 142, 139, 255});
                    DrawLine(worldX + 3, worldY + 9, worldX + 12, worldY + 6, Color{226, 89, 123, 255});
                }
            }
        }
    }

    if (showDebug_)
    {
        for (const Room& room : dungeon_.rooms())
        {
            const Vector2 world = roomWorldCenter(room.id);
            DrawText(TextFormat("%d:%s", room.id, roomTypeName(room.type)), static_cast<int>(world.x - 28), static_cast<int>(world.y - 35), 8, Fade(SKYBLUE, 0.75F));
            for (const int neighbor : room.neighbors)
            {
                if (room.id < neighbor)
                {
                    DrawLineV(world, roomWorldCenter(neighbor), Fade(SKYBLUE, 0.18F));
                }
            }
        }
    }
}

void Game::renderObjects() const
{
    for (const WorldItem& item : worldItems_)
    {
        const Color color = itemColor(item.type);
        DrawCircleV(Vector2{item.position.x, item.position.y + 5.0F}, 7.0F, Fade(BLACK, 0.35F));
        if (item.type == ItemType::Sachet)
        {
            DrawRectangle(static_cast<int>(item.position.x - 6), static_cast<int>(item.position.y - 7), 12, 13, color);
            DrawRectangle(static_cast<int>(item.position.x - 4), static_cast<int>(item.position.y - 5), 8, 3, Color{255, 211, 224, 255});
        }
        else if (item.type == ItemType::Catnip)
        {
            DrawTriangle(Vector2{item.position.x, item.position.y - 8}, Vector2{item.position.x - 7, item.position.y + 5}, Vector2{item.position.x + 2, item.position.y + 2}, color);
            DrawTriangle(Vector2{item.position.x + 1, item.position.y - 3}, Vector2{item.position.x + 7, item.position.y + 6}, Vector2{item.position.x - 2, item.position.y + 4}, Color{52, 168, 89, 255});
        }
        else
        {
            DrawCircleV(item.position, 6.0F, color);
            DrawCircleLines(static_cast<int>(item.position.x), static_cast<int>(item.position.y), 4.0F, Color{139, 94, 35, 255});
        }
    }

    for (const TrashCan& trash : trashCans_)
    {
        DrawRectangle(static_cast<int>(trash.position.x - 9), static_cast<int>(trash.position.y - 9), 18, 20, trash.opened ? Color{75, 82, 88, 255} : Color{91, 101, 105, 255});
        DrawRectangle(static_cast<int>(trash.position.x - 11), static_cast<int>(trash.position.y - (trash.opened ? 15 : 12)), 22, 5, Color{124, 132, 132, 255});
        DrawLine(static_cast<int>(trash.position.x - 4), static_cast<int>(trash.position.y - 5), static_cast<int>(trash.position.x - 4), static_cast<int>(trash.position.y + 7), Color{54, 60, 65, 255});
        DrawLine(static_cast<int>(trash.position.x + 4), static_cast<int>(trash.position.y - 5), static_cast<int>(trash.position.x + 4), static_cast<int>(trash.position.y + 7), Color{54, 60, 65, 255});
    }

    for (const Scratcher& scratcher : scratchers_)
    {
        DrawRectangle(static_cast<int>(scratcher.position.x - 10), static_cast<int>(scratcher.position.y + 8), 20, 5, Color{74, 46, 32, 255});
        DrawRectangle(static_cast<int>(scratcher.position.x - 4), static_cast<int>(scratcher.position.y - 14), 8, 23, scratcher.used ? Color{111, 78, 48, 255} : Color{177, 122, 61, 255});
        for (int scratch = -8; scratch <= 4; scratch += 4)
        {
            DrawLine(static_cast<int>(scratcher.position.x - 4), static_cast<int>(scratcher.position.y + scratch), static_cast<int>(scratcher.position.x + 4), static_cast<int>(scratcher.position.y + scratch - 3), Color{234, 189, 106, 255});
        }
    }
}

void Game::renderEnemies() const
{
    for (const Enemy& enemy : enemies_)
    {
        if (enemy.isDead())
        {
            continue;
        }

        const Vector2 position = enemy.position();
        if (enemy.state() == EnemyState::Telegraph)
        {
            DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 18.0F + enemy.stateTimer() * 8.0F, Color{244, 72, 74, 255});
            DrawLineEx(position, Vector2{position.x + enemy.facing().x * 28.0F, position.y + enemy.facing().y * 28.0F}, 2.0F, Color{244, 72, 74, 210});
        }

        DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y + 7), 11.0F, 5.0F, Fade(BLACK, 0.35F));
        const Color body = enemy.state() == EnemyState::Hurt ? WHITE : Color{133, 137, 146, 255};
        DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y), 10.0F, 7.0F, body);
        DrawCircle(static_cast<int>(position.x + enemy.facing().x * 7.0F), static_cast<int>(position.y + enemy.facing().y * 5.0F - 1.0F), 5.0F, Color{154, 157, 166, 255});
        DrawCircle(static_cast<int>(position.x - 4), static_cast<int>(position.y - 6), 3.0F, Color{190, 142, 151, 255});
        DrawLineEx(Vector2{position.x - 9.0F, position.y + 2.0F}, Vector2{position.x - 20.0F, position.y + 8.0F}, 2.0F, Color{190, 142, 151, 255});
        DrawCircle(static_cast<int>(position.x + enemy.facing().x * 10.0F), static_cast<int>(position.y - 2), 1.5F, Color{31, 28, 33, 255});

        if (enemy.hp() < enemy.maxHp())
        {
            DrawRectangle(static_cast<int>(position.x - 11), static_cast<int>(position.y - 14), 22, 3, Color{49, 34, 38, 255});
            DrawRectangle(static_cast<int>(position.x - 11), static_cast<int>(position.y - 14), static_cast<int>(22.0F * enemy.hp() / enemy.maxHp()), 3, Color{224, 73, 76, 255});
        }

        if (showDebug_)
        {
            DrawRectangleLinesEx(enemy.collider(), 1.0F, RED);
            if (enemy.canDealDamage())
            {
                DrawRectangleLinesEx(enemy.attackCollider(), 1.0F, ORANGE);
            }
        }
    }
}

void Game::renderPlayer() const
{
    const Vector2 position = player_.position();
    if (player_.isRaging())
    {
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 19.0F + std::sin(static_cast<float>(GetTime()) * 9.0F) * 2.0F, Color{89, 235, 123, 220});
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 23.0F, Fade(Color{89, 235, 123, 255}, 0.30F));
    }

    if (player_.isInvulnerable() && (static_cast<int>(GetTime() * 18.0) & 1) == 0)
    {
        return;
    }

    DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y + 8), 12.0F, 5.0F, Fade(BLACK, 0.38F));
    DrawLineEx(Vector2{position.x - player_.direction().x * 7.0F, position.y - player_.direction().y * 4.0F}, Vector2{position.x - player_.direction().x * 18.0F - player_.direction().y * 5.0F, position.y - player_.direction().y * 13.0F + player_.direction().x * 5.0F}, 4.0F, Color{204, 80, 26, 255});
    DrawCircleV(position, 10.0F, Color{230, 112, 35, 255});
    const Vector2 head{position.x + player_.direction().x * 6.0F, position.y + player_.direction().y * 5.0F - 3.0F};
    DrawCircleV(head, 7.0F, Color{242, 133, 44, 255});
    DrawTriangle(Vector2{head.x - 6, head.y - 4}, Vector2{head.x - 3, head.y - 12}, Vector2{head.x, head.y - 5}, Color{242, 133, 44, 255});
    DrawTriangle(Vector2{head.x + 1, head.y - 5}, Vector2{head.x + 5, head.y - 12}, Vector2{head.x + 7, head.y - 3}, Color{242, 133, 44, 255});
    DrawCircle(static_cast<int>(head.x + player_.direction().x * 4.0F - player_.direction().y * 2.0F), static_cast<int>(head.y + player_.direction().y * 3.0F - 1.0F), 1.5F, Color{43, 48, 35, 255});
    DrawCircle(static_cast<int>(head.x + player_.direction().x * 4.0F + player_.direction().y * 2.0F), static_cast<int>(head.y + player_.direction().y * 3.0F + 1.0F), 1.5F, Color{43, 48, 35, 255});
    DrawLineEx(Vector2{position.x - 6, position.y - 7}, Vector2{position.x + 7, position.y - 7}, 2.0F, Color{51, 193, 210, 255});

    if (player_.attackActive())
    {
        const Rectangle hitbox = player_.attackHitbox();
        const Vector2 center{hitbox.x + hitbox.width * 0.5F, hitbox.y + hitbox.height * 0.5F};
        for (int claw = -1; claw <= 1; ++claw)
        {
            const Vector2 perpendicular{-player_.direction().y * claw * 4.0F, player_.direction().x * claw * 4.0F};
            DrawLineEx(Vector2{center.x - player_.direction().x * 9.0F + perpendicular.x, center.y - player_.direction().y * 9.0F + perpendicular.y}, Vector2{center.x + player_.direction().x * 9.0F + perpendicular.x, center.y + player_.direction().y * 9.0F + perpendicular.y}, 2.0F, Color{255, 231, 171, 255});
        }
    }

    if (showDebug_)
    {
        DrawRectangleLinesEx(player_.collider(), 1.0F, GREEN);
        if (player_.attackActive())
        {
            DrawRectangleLinesEx(player_.attackHitbox(), 1.0F, YELLOW);
        }
    }
}

void Game::renderParticles() const
{
    for (const Particle& particle : particles_)
    {
        const float alpha = std::clamp(particle.life / particle.maxLife, 0.0F, 1.0F);
        DrawRectangle(static_cast<int>(particle.position.x), static_cast<int>(particle.position.y), static_cast<int>(particle.size), static_cast<int>(particle.size), Fade(particle.color, alpha));
    }
}

void Game::renderHud() const
{
    DrawRectangle(8, 8, 196, 48, Fade(Color{13, 16, 22, 255}, 0.88F));
    DrawRectangleLines(8, 8, 196, 48, Color{82, 91, 105, 255});
    DrawText("HP", 16, 16, 10, RAYWHITE);
    DrawRectangle(36, 16, 112, 10, Color{62, 34, 41, 255});
    DrawRectangle(36, 16, static_cast<int>(112.0F * player_.hp() / player_.maxHp()), 10, Color{220, 70, 73, 255});
    DrawText(TextFormat("%d/%d", player_.hp(), player_.maxHp()), 153, 16, 9, LIGHTGRAY);
    DrawText(TextFormat("ATQ %d  CRIT %d%%", player_.attack(), static_cast<int>(player_.criticalChance() * 100.0F)), 16, 35, 9, Color{255, 190, 92, 255});

    DrawRectangle(432, 8, 200, 48, Fade(Color{13, 16, 22, 255}, 0.88F));
    DrawRectangleLines(432, 8, 200, 48, Color{82, 91, 105, 255});
    DrawText(TextFormat("Q SACHE x%zu", player_.inventory().count(ItemType::Sachet)), 442, 16, 9, Color{242, 121, 151, 255});
    DrawText(TextFormat("R CATNIP x%zu", player_.inventory().count(ItemType::Catnip)), 526, 16, 9, Color{101, 224, 130, 255});
    DrawText(TextFormat("TAMPINHAS %zu", player_.inventory().count(ItemType::Coin)), 442, 34, 9, Color{244, 199, 70, 255});

    if (currentRoomId_ >= 0 && currentRoomId_ < static_cast<int>(dungeon_.rooms().size()))
    {
        const char* name = roomTypeName(dungeon_.rooms()[static_cast<std::size_t>(currentRoomId_)].type);
        const int width = MeasureText(name, 10) + 18;
        DrawRectangle(constants::kInternalWidth / 2 - width / 2, 8, width, 18, Fade(BLACK, 0.72F));
        DrawText(name, constants::kInternalWidth / 2 - width / 2 + 9, 13, 10, Color{186, 205, 224, 255});
    }

    if (player_.isRaging())
    {
        DrawRectangle(220, 32, 198, 19, Fade(Color{21, 65, 35, 255}, 0.90F));
        DrawRectangle(222, 34, static_cast<int>(194.0F * player_.rageRemaining() / 8.0F), 15, Color{57, 190, 88, 255});
        DrawText("FURIA FELINA", 276, 38, 9, RAYWHITE);
    }

    if (toastTimer_ > 0.0F)
    {
        const int width = MeasureText(toast_.c_str(), 11) + 22;
        DrawRectangle(constants::kInternalWidth / 2 - width / 2, 326, width, 22, Fade(BLACK, 0.82F));
        DrawText(toast_.c_str(), constants::kInternalWidth / 2 - width / 2 + 11, 332, 11, RAYWHITE);
    }
    DrawText(input_.isGamepadConnected() ? "PAD" : "KEY", 608, 344, 8, input_.isGamepadConnected() ? GREEN : GRAY);
}

void Game::renderDebugPanel() const
{
    DrawRectangle(8, 64, 246, 250, Fade(BLACK, 0.88F));
    DrawRectangleLines(8, 64, 246, 250, Color{75, 158, 194, 255});
    DrawText("DEBUG / ESTRUTURAS", 16, 72, 11, SKYBLUE);
    DrawText(TextFormat("Seed: %u  Sala: %d", seed_, currentRoomId_), 16, 90, 9, LIGHTGRAY);
    DrawText(TextFormat("Player: %.0f, %.0f  HP: %d", player_.position().x, player_.position().y, player_.hp()), 16, 104, 9, LIGHTGRAY);
    DrawText(TextFormat("ATQ base+run: %d (+%d)", player_.attack(), player_.permanentAttackBonus()), 16, 118, 9, LIGHTGRAY);
    DrawText(TextFormat("Inimigos: %zu  Itens: %zu", enemies_.size(), worldItems_.size()), 16, 132, 9, LIGHTGRAY);
    const auto path = dungeon_.graph().breadthFirstShortestPath(dungeon_.startRoomId(), dungeon_.bossRoomId());
    DrawText(TextFormat("MATRIZ: %dx%d", Dungeon::kWidth, Dungeon::kHeight), 16, 151, 9, Color{255, 190, 92, 255});
    DrawText(TextFormat("GRAFO: %d vertices", dungeon_.graph().vertexCount()), 16, 165, 9, Color{255, 190, 92, 255});
    DrawText(TextFormat("DFS/PILHA: conectado %s", dungeon_.graph().isConnected() ? "SIM" : "NAO"), 16, 179, 9, Color{255, 190, 92, 255});
    DrawText(TextFormat("BFS/FILA: %zu salas ate boss", path.size()), 16, 193, 9, Color{255, 190, 92, 255});
    DrawText(TextFormat("LISTA: %zu itens", player_.inventory().size()), 16, 207, 9, Color{255, 190, 92, 255});
    DrawText(TextFormat("Gamepad: %s #%d", input_.isGamepadConnected() ? "SIM" : "NAO", input_.gamepadIndex()), 16, 228, 9, LIGHTGRAY);
    DrawText(TextFormat("Nome: %.30s", input_.gamepadName()), 16, 242, 8, GRAY);
    DrawText(TextFormat("Eixos: %d  Ultimo botao: %d", input_.axisCount(), input_.lastPressedButton()), 16, 256, 9, LIGHTGRAY);

    for (int axis = 0; axis < std::min(input_.axisCount(), 4); ++axis)
    {
        DrawText(TextFormat("A%d:% .2f", axis, input_.rawAxis(axis)), 16 + (axis % 2) * 74, 272 + (axis / 2) * 13, 8, GRAY);
    }
    for (int button = 0; button < InputManager::kDiagnosticButtonCount; ++button)
    {
        if (input_.isRawButtonDown(button))
        {
            DrawText(TextFormat("B%d DOWN", button), 164, 272 + (button % 3) * 12, 8, GREEN);
        }
    }
    DrawText("F3 fecha debug", 164, 300, 8, SKYBLUE);
}

void Game::renderStateOverlay() const
{
    if (state_ == GameState::Playing)
    {
        return;
    }

    DrawRectangle(0, 0, constants::kInternalWidth, constants::kInternalHeight, Fade(BLACK, 0.72F));
    const char* title = state_ == GameState::Paused ? "PAUSADO" : (state_ == GameState::GameOver ? "A RUA VENCEU" : "VITORIA!");
    const Color color = state_ == GameState::GameOver ? Color{237, 87, 82, 255} : Color{255, 190, 92, 255};
    DrawText(title, constants::kInternalWidth / 2 - MeasureText(title, 34) / 2, 126, 34, color);

    const char* prompt = state_ == GameState::Paused ? "ESC / X: continuar" : "ENTER / QUADRADO: nova run";
    DrawText(prompt, constants::kInternalWidth / 2 - MeasureText(prompt, 12) / 2, 181, 12, RAYWHITE);
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

void Game::spawnParticles(const Vector2 position, const Color color, const int count, const float speed)
{
    std::uniform_real_distribution<float> direction(-1.0F, 1.0F);
    std::uniform_real_distribution<float> life(0.20F, 0.48F);
    std::uniform_real_distribution<float> particleSize(1.0F, 3.0F);
    for (int index = 0; index < count; ++index)
    {
        Vector2 velocity{direction(random_), direction(random_)};
        const float magnitude = std::max(0.1F, std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y));
        velocity.x = velocity.x / magnitude * speed;
        velocity.y = velocity.y / magnitude * speed;
        const float duration = life(random_);
        particles_.push_back(Particle{position, velocity, color, duration, duration, particleSize(random_)});
    }
}

void Game::spawnLoot(const Vector2 position)
{
    std::uniform_int_distribution<int> roll(0, 99);
    const int value = roll(random_);
    if (value < 58)
    {
        worldItems_.push_back(WorldItem{ItemType::Sachet, position});
    }
    else if (value < 76)
    {
        worldItems_.push_back(WorldItem{ItemType::Catnip, position});
    }
    else if (value < 91)
    {
        worldItems_.push_back(WorldItem{ItemType::Coin, position});
    }
}

void Game::setToast(std::string text, const float duration)
{
    toast_ = std::move(text);
    toastTimer_ = duration;
}

Vector2 Game::roomWorldCenter(const int roomId) const
{
    const GridPoint center = dungeon_.roomCenter(roomId);
    return Vector2{static_cast<float>(center.x * Dungeon::kTileSize + Dungeon::kTileSize / 2), static_cast<float>(center.y * Dungeon::kTileSize + Dungeon::kTileSize / 2)};
}

