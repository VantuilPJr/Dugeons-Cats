#include "core/Game.h"

#include "core/Constants.h"
#include "input/GameAction.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
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

std::string findAssetPath(const std::filesystem::path& relativePath)
{
    const std::filesystem::path executableDirectory = GetApplicationDirectory();
    const std::array candidates{
        executableDirectory / relativePath,
        executableDirectory / ".." / relativePath,
        relativePath,
        std::filesystem::path{".."} / relativePath,
    };

    for (const std::filesystem::path& candidate : candidates)
    {
        const std::string path = candidate.lexically_normal().string();
        if (FileExists(path.c_str()))
        {
            return path;
        }
    }

    return {};
}

Rectangle atlasSource(const Texture2D texture, const int column, const int row, const bool flipX = false)
{
    constexpr float columns = 6.0F;
    constexpr float rows = 4.0F;
    const float width = static_cast<float>(texture.width) / columns;
    const float height = static_cast<float>(texture.height) / rows;
    if (flipX)
    {
        return Rectangle{(static_cast<float>(column) + 1.0F) * width, static_cast<float>(row) * height, -width, height};
    }
    return Rectangle{static_cast<float>(column) * width, static_cast<float>(row) * height, width, height};
}

void drawAtlasSprite(const Texture2D texture, const int column, const int row, const Vector2 center,
                     const float width, const float height, const bool flipX = false, const Color tint = WHITE)
{
    if (texture.id == 0U)
    {
        return;
    }
    const Rectangle destination{center.x - width * 0.5F, center.y - height * 0.5F, width, height};
    DrawTexturePro(texture, atlasSource(texture, column, row, flipX), destination, Vector2{}, 0.0F, tint);
}
} // namespace

Game::~Game()
{
    audio_.shutdown();
    if (titleBackground_.id != 0U)
    {
        UnloadTexture(titleBackground_);
    }
    for (Texture2D* texture : {&playerAtlas_, &enemyAtlas_, &urbanAtlas_, &propsAtlas_})
    {
        if (texture->id != 0U)
        {
            UnloadTexture(*texture);
        }
    }
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
    if (const char* captureDirectory = std::getenv("DUNGEONS_CATS_CAPTURE_DIR"))
    {
        const std::filesystem::path directory = captureDirectory;
        std::filesystem::create_directories(directory);
        const auto saveInternalCapture = [this, &directory](const char* fileName) {
            Image image = LoadImageFromTexture(renderTarget_.texture);
            ImageFlipVertical(&image);
            ImageResizeNN(&image, constants::kWindowWidth, constants::kWindowHeight);
            const std::string output = (directory / fileName).string();
            ExportImage(image, output.c_str());
            UnloadImage(image);
        };
        startNewRun();
        render();
        render();
        saveInternalCapture("world.png");
        inventoryOpen_ = true;
        render();
        render();
        saveInternalCapture("bag.png");
        auto enemyToCapture = std::find_if(enemies_.begin(), enemies_.end(), [](const Enemy& enemy) {
            return enemy.type() == EnemyType::StrayDog;
        });
        if (enemyToCapture == enemies_.end() && !enemies_.empty())
        {
            enemyToCapture = enemies_.begin();
        }
        inventoryOpen_ = false;
        if (enemyToCapture != enemies_.end())
        {
            player_.reset(enemyToCapture->position());
            currentRoomId_ = enemyToCapture->roomId();
            updateCamera();
            render();
            render();
            saveInternalCapture("enemies.png");
        }
        inventoryOpen_ = false;
        player_.reset(roomWorldCenter(dungeon_.bossRoomId()));
        currentRoomId_ = dungeon_.bossRoomId();
        boss_.engage();
        updateCamera();
        render();
        render();
        saveInternalCapture("boss.png");
        return 0;
    }
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
    audio_.initialize();
    const std::string titlePath = findAssetPath("assets/ui/title-background.png");
    if (!titlePath.empty())
    {
        titleBackground_ = LoadTexture(titlePath.c_str());
        if (titleBackground_.id != 0U)
        {
            SetTextureFilter(titleBackground_, TEXTURE_FILTER_POINT);
        }
    }

    const auto loadAtlas = [](Texture2D& destination, const char* path) {
        const std::string resolved = findAssetPath(path);
        if (!resolved.empty())
        {
            destination = LoadTexture(resolved.c_str());
            if (destination.id != 0U)
            {
                SetTextureFilter(destination, TEXTURE_FILTER_POINT);
            }
        }
    };
    loadAtlas(playerAtlas_, "assets/sprites/ginger-cat-atlas.png");
    loadAtlas(enemyAtlas_, "assets/sprites/street-enemies-atlas.png");
    loadAtlas(urbanAtlas_, "assets/tiles/brazilian-urban-atlas.png");
    loadAtlas(propsAtlas_, "assets/sprites/urban-props-items-atlas.png");

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
    inventoryOpen_ = false;
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
    boss_.reset(roomWorldCenter(dungeon_.bossRoomId()), dungeon_.bossRoomId());

    const auto distances = dungeon_.graph().breadthFirstDistances(dungeon_.startRoomId());
    for (const Room& room : dungeon_.rooms())
    {
        if (room.type == RoomType::Start || room.type == RoomType::Boss)
        {
            continue;
        }

        const Vector2 center = roomWorldCenter(room.id);
        if (room.type == RoomType::Combat)
        {
            const int depth = distances[static_cast<std::size_t>(room.id)];
            if (depth >= 4)
            {
                enemies_.emplace_back(EnemyType::StrayDog, center, room.id);
                enemies_.emplace_back(EnemyType::Rat, Vector2{center.x + 30.0F, center.y + 12.0F}, room.id);
            }
            else if (depth >= 2)
            {
                enemies_.emplace_back(EnemyType::Bird, Vector2{center.x - 22.0F, center.y}, room.id);
                enemies_.emplace_back(EnemyType::Rat, Vector2{center.x + 24.0F, center.y + 12.0F}, room.id);
            }
            else
            {
                enemies_.emplace_back(EnemyType::Rat, Vector2{center.x - 20.0F, center.y}, room.id);
                enemies_.emplace_back(EnemyType::Rat, Vector2{center.x + 22.0F, center.y + 12.0F}, room.id);
            }
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
    audio_.updateAmbient(state_ != GameState::Paused && state_ != GameState::GameOver && state_ != GameState::Victory);
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
        if (inventoryOpen_)
        {
            if (input_.isActionPressed(GameAction::Inventory)
                || input_.isActionPressed(GameAction::Pause)
                || input_.isActionPressed(GameAction::Dodge))
            {
                inventoryOpen_ = false;
            }
        }
        else if (input_.isActionPressed(GameAction::Inventory))
        {
            inventoryOpen_ = true;
        }
        else if (input_.isActionPressed(GameAction::Pause))
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
        audio_.playPickup();
        setToast("Sache consumido: HP recuperado");
        spawnParticles(player_.position(), Color{242, 91, 131, 255}, 12);
    }
    if (input_.isActionPressed(GameAction::UseCatnip) && player_.useCatnip())
    {
        audio_.playRage();
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
                audio_.playHit();
                spawnParticles(player_.position(), Color{226, 71, 64, 255}, 12, 80.0F);
                screenShake_ = 0.18F;
                setToast(TextFormat("-%d HP", enemy.damage()), 0.9F);
            }
        }
    }

    if (currentRoomId_ == boss_.roomId() && !boss_.engaged())
    {
        boss_.engage();
        audio_.playBossCue();
        setToast("CARAMELO, O REI DAS RUAS", 3.0F);
        screenShake_ = 0.35F;
    }
    boss_.update(player_.position(), dungeon_, deltaTime);
    if (boss_.attackHits(player_.collider()))
    {
        boss_.markDamageApplied();
        if (player_.takeDamage(boss_.damage()))
        {
            audio_.playHit();
            spawnParticles(player_.position(), Color{244, 68, 61, 255}, 20, 105.0F);
            screenShake_ = 0.30F;
            setToast(TextFormat("Caramelo: -%d HP", boss_.damage()), 1.0F);
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

    audio_.playAttack();

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


    if (boss_.engaged() && !boss_.isDead() && CheckCollisionRecs(player_.attackHitbox(), boss_.collider()))
    {
        bool critical = false;
        const int damage = player_.rollAttackDamage(random_, critical);
        const bool killed = boss_.takeDamage(damage);
        hitSomething = true;
        spawnParticles(boss_.position(), critical ? YELLOW : Color{255, 174, 79, 255}, critical ? 20 : 10, 80.0F);
        setToast(critical ? TextFormat("CRITICO NO CHEFE! %d", damage) : TextFormat("Caramelo: -%d", damage), 0.85F);
        if (killed)
        {
            audio_.playVictory();
            spawnParticles(boss_.position(), Color{255, 202, 89, 255}, 70, 130.0F);
            screenShake_ = 0.8F;
            state_ = GameState::Victory;
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
            audio_.playPickup();
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
            audio_.playPickup();
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
            audio_.playRage();
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
        if (inventoryOpen_)
        {
            renderInventoryBag();
        }
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
    if (titleBackground_.id != 0U)
    {
        const Rectangle source{0.0F, 0.0F, static_cast<float>(titleBackground_.width), static_cast<float>(titleBackground_.height)};
        const Rectangle destination{0.0F, 0.0F, static_cast<float>(constants::kInternalWidth), static_cast<float>(constants::kInternalHeight)};
        DrawTexturePro(titleBackground_, source, destination, Vector2{}, 0.0F, WHITE);
        DrawRectangleGradientH(0, 0, 430, constants::kInternalHeight, Fade(BLACK, 0.78F), BLANK);
        DrawRectangle(0, 238, constants::kInternalWidth, 68, Fade(BLACK, 0.58F));
    }
    DrawText("DUNGEONS", 43, 40, 48, Color{255, 183, 69, 255});
    DrawText("& CATS", 95, 91, 44, Color{255, 221, 151, 255});
    DrawText("UM ROGUELITE PELAS RUAS", 49, 143, 13, Color{181, 195, 214, 255});

    DrawRectangle(46, 189, 270, 29, Fade(BLACK, 0.55F));
    DrawText("ENTER / QUADRADO: NOVA RUN", 61, 198, 12, RAYWHITE);
    DrawText("WASD  mover   J/ESPACO  atacar   E  interagir", 43, 250, 10, LIGHTGRAY);
    DrawText("SHIFT esquiva   Q sache   R catnip   TAB/I bolsa", 43, 266, 10, LIGHTGRAY);
    DrawText("ESC pause   F3 diagnostico do controle", 43, 282, 10, Color{117, 201, 255, 255});

    if (titleBackground_.id == 0U)
    {
        const float catX = 503.0F;
        const float catY = 285.0F;
        DrawCircle(static_cast<int>(catX), static_cast<int>(catY), 17.0F, Color{230, 112, 35, 255});
        DrawTriangle(Vector2{catX - 14, catY - 11}, Vector2{catX - 6, catY - 28}, Vector2{catX - 1, catY - 12}, Color{238, 133, 48, 255});
        DrawTriangle(Vector2{catX + 2, catY - 12}, Vector2{catX + 10, catY - 28}, Vector2{catX + 15, catY - 9}, Color{238, 133, 48, 255});
        DrawCircle(static_cast<int>(catX - 6), static_cast<int>(catY - 2), 2.0F, Color{49, 50, 38, 255});
        DrawCircle(static_cast<int>(catX + 6), static_cast<int>(catY - 2), 2.0F, Color{49, 50, 38, 255});
        DrawLineEx(Vector2{catX + 14, catY + 8}, Vector2{catX + 34, catY + 2}, 4.0F, Color{218, 96, 30, 255});
    }
}

void Game::renderWorld() const
{
    BeginMode2D(camera_);
    renderDungeon();
    renderObjects();
    renderEnemies();
    renderBoss();
    renderPlayer();
    renderParticles();
    EndMode2D();
}

void Game::renderDungeon() const
{
    const float halfWidth = constants::kInternalWidth * 0.5F / camera_.zoom;
    const float halfHeight = constants::kInternalHeight * 0.5F / camera_.zoom;
    const int firstX = std::max(0, static_cast<int>((camera_.target.x - halfWidth) / Dungeon::kTileSize) - 2);
    const int lastX = std::min(Dungeon::kWidth - 1, static_cast<int>((camera_.target.x + halfWidth) / Dungeon::kTileSize) + 2);
    const int firstY = std::max(0, static_cast<int>((camera_.target.y - halfHeight) / Dungeon::kTileSize) - 2);
    const int lastY = std::min(Dungeon::kHeight - 1, static_cast<int>((camera_.target.y + halfHeight) / Dungeon::kTileSize) + 2);

    for (int y = firstY; y <= lastY; ++y)
    {
        for (int x = firstX; x <= lastX; ++x)
        {
            const TileType tile = dungeon_.tileAt(x, y);
            const int worldX = x * Dungeon::kTileSize;
            const int worldY = y * Dungeon::kTileSize;
            const unsigned int pattern = static_cast<unsigned int>(x * 73856093) ^ static_cast<unsigned int>(y * 19349663) ^ seed_;

            if (tile == TileType::Floor && urbanAtlas_.id != 0U)
            {
                const int roomId = dungeon_.roomAtWorld(worldX + Dungeon::kTileSize * 0.5F,
                                                        worldY + Dungeon::kTileSize * 0.5F);
                int tileColumn = 2;
                if (roomId >= 0)
                {
                    const Room& room = dungeon_.rooms()[static_cast<std::size_t>(roomId)];
                    switch (room.type)
                    {
                    case RoomType::Start: tileColumn = 1; break;
                    case RoomType::Combat: tileColumn = (room.id & 1) == 0 ? 0 : 1; break;
                    case RoomType::Treasure: tileColumn = 3; break;
                    case RoomType::Scratcher: tileColumn = 2; break;
                    case RoomType::Normal: tileColumn = room.id % 3 == 0 ? 4 : 2; break;
                    case RoomType::Boss: tileColumn = 5; break;
                    }
                }
                else if (x < Dungeon::kWidth / 3)
                {
                    tileColumn = 1;
                }
                else if (x >= Dungeon::kWidth * 2 / 3)
                {
                    tileColumn = 3;
                }
                const Rectangle destination{static_cast<float>(worldX), static_cast<float>(worldY),
                                            static_cast<float>(Dungeon::kTileSize), static_cast<float>(Dungeon::kTileSize)};
                DrawTexturePro(urbanAtlas_, atlasSource(urbanAtlas_, tileColumn, 0), destination, Vector2{}, 0.0F, WHITE);
                if (pattern % 43U == 0U)
                {
                    DrawTexturePro(urbanAtlas_, atlasSource(urbanAtlas_, static_cast<int>(pattern % 6U), 2),
                                   destination, Vector2{}, 0.0F, Fade(WHITE, 0.82F));
                }
            }
            else if (tile == TileType::Wall && urbanAtlas_.id != 0U)
            {
                const int column = (x / 12 + y / 10 + static_cast<int>(seed_ % 6U)) % 6;
                const int row = pattern % 79U == 0U ? 3 : 1;
                const Rectangle destination{static_cast<float>(worldX), static_cast<float>(worldY),
                                            static_cast<float>(Dungeon::kTileSize), static_cast<float>(Dungeon::kTileSize)};
                DrawTexturePro(urbanAtlas_, atlasSource(urbanAtlas_, column, row), destination, Vector2{}, 0.0F, WHITE);
            }
            else if (tile == TileType::Empty)
            {
                const Color block = (pattern & 1U) == 0U ? Color{25, 29, 34, 255} : Color{28, 33, 38, 255};
                DrawRectangle(worldX, worldY, Dungeon::kTileSize, Dungeon::kTileSize, block);
                if (urbanAtlas_.id != 0U && pattern % 127U == 0U)
                {
                    const Rectangle destination{static_cast<float>(worldX), static_cast<float>(worldY),
                                                static_cast<float>(Dungeon::kTileSize), static_cast<float>(Dungeon::kTileSize)};
                    DrawTexturePro(urbanAtlas_, atlasSource(urbanAtlas_, 5, 3), destination, Vector2{}, 0.0F, Fade(WHITE, 0.45F));
                }
            }
            else if (tile == TileType::Floor)
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
        if (propsAtlas_.id != 0U)
        {
            int column = 0;
            if (item.type == ItemType::Catnip)
            {
                column = 1;
            }
            else if (item.type == ItemType::Coin)
            {
                column = 2;
            }
            drawAtlasSprite(propsAtlas_, column, 1, Vector2{item.position.x, item.position.y - 2.0F}, 25.0F, 25.0F);
        }
        else if (item.type == ItemType::Sachet)
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
        if (propsAtlas_.id != 0U)
        {
            drawAtlasSprite(propsAtlas_, trash.opened ? 1 : 0, 0,
                            Vector2{trash.position.x, trash.position.y - 5.0F}, 42.0F, 42.0F);
        }
        else
        {
            DrawRectangle(static_cast<int>(trash.position.x - 9), static_cast<int>(trash.position.y - 9), 18, 20, trash.opened ? Color{75, 82, 88, 255} : Color{91, 101, 105, 255});
            DrawRectangle(static_cast<int>(trash.position.x - 11), static_cast<int>(trash.position.y - (trash.opened ? 15 : 12)), 22, 5, Color{124, 132, 132, 255});
        }
    }

    for (const Scratcher& scratcher : scratchers_)
    {
        if (propsAtlas_.id != 0U)
        {
            drawAtlasSprite(propsAtlas_, 2, 0, Vector2{scratcher.position.x, scratcher.position.y - 6.0F},
                            44.0F, 44.0F, false, scratcher.used ? Fade(GRAY, 0.78F) : WHITE);
        }
        else
        {
            DrawRectangle(static_cast<int>(scratcher.position.x - 10), static_cast<int>(scratcher.position.y + 8), 20, 5, Color{74, 46, 32, 255});
            DrawRectangle(static_cast<int>(scratcher.position.x - 4), static_cast<int>(scratcher.position.y - 14), 8, 23, scratcher.used ? Color{111, 78, 48, 255} : Color{177, 122, 61, 255});
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

        DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y + 7), enemy.type() == EnemyType::StrayDog ? 18.0F : 11.0F, 5.0F, Fade(BLACK, 0.35F));
        const bool hurt = enemy.state() == EnemyState::Hurt;
        if (enemyAtlas_.id != 0U)
        {
            int row = 0;
            float spriteSize = 36.0F;
            if (enemy.type() == EnemyType::Bird)
            {
                row = 1;
                spriteSize = 42.0F;
            }
            else if (enemy.type() == EnemyType::StrayDog)
            {
                row = 2;
                spriteSize = 62.0F;
            }

            int frame = 0;
            if (enemy.state() == EnemyState::Chase)
            {
                frame = 1 + static_cast<int>(GetTime() * 8.0) % 2;
            }
            else if (enemy.state() == EnemyState::Telegraph)
            {
                frame = 3;
            }
            else if (enemy.state() == EnemyState::Attack)
            {
                frame = 4;
            }
            else if (hurt)
            {
                frame = 5;
            }
            const float hover = enemy.type() == EnemyType::Bird
                ? std::sin(static_cast<float>(GetTime()) * 8.0F + position.x) * 2.0F
                : 0.0F;
            drawAtlasSprite(enemyAtlas_, frame, row, Vector2{position.x, position.y - spriteSize * 0.20F + hover},
                            spriteSize, spriteSize, enemy.facing().x < -0.05F, hurt ? Color{255, 205, 205, 255} : WHITE);
        }
        else if (enemy.type() == EnemyType::Rat)
        {
            const Color body = hurt ? WHITE : Color{133, 137, 146, 255};
            DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y), 10.0F, 7.0F, body);
            DrawCircle(static_cast<int>(position.x + enemy.facing().x * 7.0F), static_cast<int>(position.y + enemy.facing().y * 5.0F - 1.0F), 5.0F, Color{154, 157, 166, 255});
            DrawCircle(static_cast<int>(position.x - 4), static_cast<int>(position.y - 6), 3.0F, Color{190, 142, 151, 255});
            DrawLineEx(Vector2{position.x - 9.0F, position.y + 2.0F}, Vector2{position.x - 20.0F, position.y + 8.0F}, 2.0F, Color{190, 142, 151, 255});
            DrawCircle(static_cast<int>(position.x + enemy.facing().x * 10.0F), static_cast<int>(position.y - 2), 1.5F, Color{31, 28, 33, 255});
        }
        else if (enemy.type() == EnemyType::Bird)
        {
            const float hover = std::sin(static_cast<float>(GetTime()) * 8.0F + position.x) * 2.0F;
            const Color body = hurt ? WHITE : Color{61, 135, 171, 255};
            DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y - 3.0F + hover), 9.0F, 7.0F, body);
            DrawTriangle(Vector2{position.x - 3, position.y - 2 + hover}, Vector2{position.x - 15, position.y - 9 + hover}, Vector2{position.x - 8, position.y + 5 + hover}, Color{80, 163, 194, 255});
            DrawTriangle(Vector2{position.x + 2, position.y - 2 + hover}, Vector2{position.x + 14, position.y - 9 + hover}, Vector2{position.x + 8, position.y + 5 + hover}, Color{80, 163, 194, 255});
            DrawTriangle(Vector2{position.x + enemy.facing().x * 8.0F, position.y - 3 + hover}, Vector2{position.x + enemy.facing().x * 14.0F, position.y + hover}, Vector2{position.x + enemy.facing().x * 8.0F, position.y + 2 + hover}, Color{247, 191, 60, 255});
        }
        else
        {
            const Color body = hurt ? WHITE : Color{162, 111, 63, 255};
            DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y), 17.0F, 11.0F, body);
            const Vector2 head{position.x + enemy.facing().x * 13.0F, position.y + enemy.facing().y * 7.0F - 4.0F};
            DrawCircleV(head, 9.0F, Color{181, 126, 70, 255});
            DrawTriangle(Vector2{head.x - 7, head.y - 5}, Vector2{head.x - 5, head.y - 14}, Vector2{head.x, head.y - 7}, Color{91, 61, 45, 255});
            DrawTriangle(Vector2{head.x + 1, head.y - 7}, Vector2{head.x + 7, head.y - 14}, Vector2{head.x + 8, head.y - 4}, Color{91, 61, 45, 255});
            DrawCircle(static_cast<int>(head.x + enemy.facing().x * 8.0F), static_cast<int>(head.y), 2.0F, Color{33, 28, 25, 255});
            DrawLineEx(Vector2{position.x - enemy.facing().x * 15.0F, position.y}, Vector2{position.x - enemy.facing().x * 25.0F, position.y - 6.0F}, 4.0F, Color{137, 89, 56, 255});
        }

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
    if (playerAtlas_.id != 0U)
    {
        int row = 2;
        const Vector2 direction = player_.direction();
        if (std::abs(direction.y) >= std::abs(direction.x))
        {
            row = direction.y >= 0.0F ? 0 : 3;
        }
        else
        {
            row = direction.x < 0.0F ? 1 : 2;
        }

        int frame = 0;
        if (player_.attackActive())
        {
            frame = 4 + static_cast<int>(GetTime() * 14.0) % 2;
        }
        else if (player_.isMoving())
        {
            frame = 1 + static_cast<int>(GetTime() * 9.0) % 3;
        }
        drawAtlasSprite(playerAtlas_, frame, row, Vector2{position.x, position.y - 12.0F}, 54.0F, 54.0F);
    }
    else
    {
        DrawLineEx(Vector2{position.x - player_.direction().x * 7.0F, position.y - player_.direction().y * 4.0F}, Vector2{position.x - player_.direction().x * 18.0F - player_.direction().y * 5.0F, position.y - player_.direction().y * 13.0F + player_.direction().x * 5.0F}, 4.0F, Color{204, 80, 26, 255});
        DrawCircleV(position, 10.0F, Color{230, 112, 35, 255});
        const Vector2 head{position.x + player_.direction().x * 6.0F, position.y + player_.direction().y * 5.0F - 3.0F};
        DrawCircleV(head, 7.0F, Color{242, 133, 44, 255});
        DrawTriangle(Vector2{head.x - 6, head.y - 4}, Vector2{head.x - 3, head.y - 12}, Vector2{head.x, head.y - 5}, Color{242, 133, 44, 255});
        DrawTriangle(Vector2{head.x + 1, head.y - 5}, Vector2{head.x + 5, head.y - 12}, Vector2{head.x + 7, head.y - 3}, Color{242, 133, 44, 255});
        DrawCircle(static_cast<int>(head.x + player_.direction().x * 4.0F - player_.direction().y * 2.0F), static_cast<int>(head.y + player_.direction().y * 3.0F - 1.0F), 1.5F, Color{43, 48, 35, 255});
        DrawCircle(static_cast<int>(head.x + player_.direction().x * 4.0F + player_.direction().y * 2.0F), static_cast<int>(head.y + player_.direction().y * 3.0F + 1.0F), 1.5F, Color{43, 48, 35, 255});
        DrawLineEx(Vector2{position.x - 6, position.y - 7}, Vector2{position.x + 7, position.y - 7}, 2.0F, Color{51, 193, 210, 255});
    }

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

void Game::renderBoss() const
{
    if (!boss_.engaged() || boss_.isDead())
    {
        return;
    }

    const Vector2 position = boss_.position();
    const Color warning = boss_.phaseTwo() ? Color{255, 72, 62, 255} : Color{255, 174, 70, 255};
    if (boss_.state() == BossState::Telegraph)
    {
        if (boss_.attack() == BossAttack::Charge)
        {
            DrawLineEx(position, Vector2{position.x + boss_.attackDirection().x * 180.0F, position.y + boss_.attackDirection().y * 180.0F}, 5.0F, Fade(warning, 0.62F));
        }
        else if (boss_.attack() == BossAttack::Bark)
        {
            DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 48.0F + boss_.stateTimer() * 12.0F, warning);
            DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 58.0F + boss_.stateTimer() * 12.0F, Fade(warning, 0.45F));
        }
        else
        {
            DrawRectangleLinesEx(boss_.attackCollider(), 3.0F, warning);
        }
        DrawText(bossAttackName(boss_.attack()), static_cast<int>(position.x) - MeasureText(bossAttackName(boss_.attack()), 9) / 2, static_cast<int>(position.y - 46), 9, warning);
    }
    else if (boss_.state() == BossState::Attack && boss_.attack() == BossAttack::Bark)
    {
        DrawRing(position, std::max(0.0F, boss_.barkRadius() - 7.0F), boss_.barkRadius(), 0.0F, 360.0F, 48, Fade(warning, 0.80F));
    }

    if (boss_.phaseTwo())
    {
        DrawCircleLines(static_cast<int>(position.x), static_cast<int>(position.y), 34.0F + std::sin(static_cast<float>(GetTime()) * 11.0F) * 3.0F, Fade(RED, 0.72F));
    }
    if (boss_.state() == BossState::PhaseTransition)
    {
        DrawCircleGradient(static_cast<int>(position.x), static_cast<int>(position.y), 50.0F + boss_.stateTimer() * 25.0F, Fade(RED, 0.52F), BLANK);
    }

    DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y + 17), 31.0F, 10.0F, Fade(BLACK, 0.42F));
    if (enemyAtlas_.id != 0U)
    {
        int frame = 0;
        if (boss_.state() == BossState::ChooseAttack || boss_.state() == BossState::Telegraph)
        {
            frame = 3;
        }
        else if (boss_.state() == BossState::Attack)
        {
            frame = 4;
        }
        else if (boss_.state() == BossState::Recover || boss_.state() == BossState::PhaseTransition)
        {
            frame = 5;
        }
        else
        {
            frame = 1 + static_cast<int>(GetTime() * 4.0) % 2;
        }
        const Color tint = boss_.phaseTwo() ? Color{255, 205, 195, 255} : WHITE;
        drawAtlasSprite(enemyAtlas_, frame, 3, Vector2{position.x, position.y - 26.0F},
                        104.0F, 104.0F, boss_.attackDirection().x < -0.05F, tint);
    }
    else
    {
        DrawEllipse(static_cast<int>(position.x), static_cast<int>(position.y), 27.0F, 19.0F, Color{205, 135, 60, 255});
        const Vector2 head{position.x + boss_.attackDirection().x * 20.0F, position.y + boss_.attackDirection().y * 11.0F - 6.0F};
        DrawCircleV(head, 15.0F, Color{224, 151, 68, 255});
        DrawTriangle(Vector2{head.x - 12, head.y - 8}, Vector2{head.x - 9, head.y - 24}, Vector2{head.x - 1, head.y - 11}, Color{116, 74, 45, 255});
        DrawTriangle(Vector2{head.x + 2, head.y - 11}, Vector2{head.x + 11, head.y - 24}, Vector2{head.x + 13, head.y - 7}, Color{116, 74, 45, 255});
        DrawCircle(static_cast<int>(head.x + boss_.attackDirection().x * 12.0F), static_cast<int>(head.y + boss_.attackDirection().y * 5.0F), 4.0F, Color{48, 35, 29, 255});
        DrawCircle(static_cast<int>(head.x - 5), static_cast<int>(head.y - 4), 2.0F, Color{31, 30, 25, 255});
        DrawCircle(static_cast<int>(head.x + 5), static_cast<int>(head.y - 4), 2.0F, Color{31, 30, 25, 255});
        DrawLineEx(Vector2{position.x - 18, position.y - 12}, Vector2{position.x + 20, position.y - 12}, 4.0F, boss_.phaseTwo() ? RED : Color{49, 182, 196, 255});
        DrawLineEx(Vector2{position.x - boss_.attackDirection().x * 24.0F, position.y}, Vector2{position.x - boss_.attackDirection().x * 39.0F, position.y - 12.0F}, 7.0F, Color{184, 112, 54, 255});
    }

    if (showDebug_)
    {
        DrawRectangleLinesEx(boss_.collider(), 1.0F, MAGENTA);
        DrawText(bossStateName(boss_.state()), static_cast<int>(position.x - 22), static_cast<int>(position.y + 29), 8, MAGENTA);
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

    if (propsAtlas_.id != 0U)
    {
        drawAtlasSprite(propsAtlas_, inventoryOpen_ ? 1 : 0, 2, Vector2{416.0F, 23.0F}, 34.0F, 34.0F);
    }
    DrawText("TAB", 405, 43, 8, Color{201, 211, 223, 255});

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

    if (boss_.engaged() && !boss_.isDead())
    {
        DrawRectangle(170, 60, 300, 25, Fade(BLACK, 0.86F));
        DrawRectangle(177, 76, 286, 5, Color{66, 31, 34, 255});
        DrawRectangle(177, 76, static_cast<int>(286.0F * boss_.hp() / boss_.maxHp()), 5, boss_.phaseTwo() ? Color{240, 67, 58, 255} : Color{219, 127, 51, 255});
        const char* bossName = boss_.phaseTwo() ? "CARAMELO - FASE 2" : "CARAMELO, O REI DAS RUAS";
        DrawText(bossName, constants::kInternalWidth / 2 - MeasureText(bossName, 10) / 2, 64, 10, RAYWHITE);
    }
}

void Game::renderInventoryBag() const
{
    DrawRectangle(0, 0, constants::kInternalWidth, constants::kInternalHeight, Fade(Color{8, 10, 14, 255}, 0.76F));
    const Rectangle panel{74.0F, 38.0F, 492.0F, 284.0F};
    DrawRectangleRounded(panel, 0.06F, 8, Color{35, 29, 28, 248});
    DrawRectangleRoundedLinesEx(panel, 0.06F, 8, 2.0F, Color{223, 157, 72, 255});
    DrawText("BOLSA DO AVENTUREIRO", 101, 56, 20, Color{255, 205, 120, 255});
    DrawText("TAB / I / TRIANGULO: fechar", 382, 78, 8, Color{174, 184, 198, 255});
    DrawLine(96, 86, 543, 86, Color{92, 72, 60, 255});

    if (propsAtlas_.id != 0U)
    {
        drawAtlasSprite(propsAtlas_, 1, 2, Vector2{188.0F, 150.0F}, 132.0F, 132.0F);
    }
    else
    {
        DrawRectangleRounded(Rectangle{113, 107, 150, 143}, 0.12F, 6, Color{109, 77, 46, 255});
    }
    DrawText(TextFormat("%zu itens carregados", player_.inventory().size()), 132, 205, 10, LIGHTGRAY);

    const Rectangle mapArea{105.0F, 226.0F, 166.0F, 76.0F};
    DrawRectangleRounded(mapArea, 0.08F, 4, Color{24, 31, 35, 255});
    DrawRectangleRoundedLinesEx(mapArea, 0.08F, 4, 1.0F, Color{67, 106, 116, 255});
    const auto mapPoint = [&mapArea](const GridPoint point) {
        return Vector2{mapArea.x + 5.0F + static_cast<float>(point.x) / Dungeon::kWidth * (mapArea.width - 10.0F),
                       mapArea.y + 5.0F + static_cast<float>(point.y) / Dungeon::kHeight * (mapArea.height - 10.0F)};
    };
    for (const Room& room : dungeon_.rooms())
    {
        const Vector2 from = mapPoint(room.center());
        for (const int neighbor : room.neighbors)
        {
            if (room.id < neighbor)
            {
                DrawLineEx(from, mapPoint(dungeon_.rooms()[static_cast<std::size_t>(neighbor)].center()), 1.0F, Color{61, 78, 82, 255});
            }
        }
    }
    for (const Room& room : dungeon_.rooms())
    {
        Color marker = Color{112, 130, 134, 255};
        float radius = 2.0F;
        if (room.id == dungeon_.bossRoomId())
        {
            marker = Color{238, 91, 71, 255};
            radius = 3.0F;
        }
        if (room.id == currentRoomId_)
        {
            marker = Color{255, 207, 84, 255};
            radius = 4.0F;
        }
        DrawCircleV(mapPoint(room.center()), radius, marker);
    }

    const auto drawSlot = [this](const int y, const int iconColumn, const ItemType type,
                                 const char* key, const char* description, const Color color) {
        const Rectangle slot{296.0F, static_cast<float>(y), 238.0F, 48.0F};
        DrawRectangleRounded(slot, 0.12F, 5, Color{52, 47, 47, 255});
        DrawRectangleRoundedLinesEx(slot, 0.12F, 5, 1.0F, Color{91, 84, 83, 255});
        drawAtlasSprite(propsAtlas_, iconColumn, 1, Vector2{321.0F, static_cast<float>(y + 24)}, 40.0F, 40.0F);
        DrawText(TextFormat("%s  x%zu", itemName(type), player_.inventory().count(type)), 347, y + 8, 12, color);
        DrawText(TextFormat("[%s] %s", key, description), 347, y + 27, 8, Color{185, 190, 198, 255});
    };
    drawSlot(101, 0, ItemType::Sachet, "Q", "recupera 32 HP", Color{247, 129, 161, 255});
    drawSlot(157, 1, ItemType::Catnip, "R", "ativa Furia Felina", Color{112, 225, 132, 255});
    drawSlot(213, 2, ItemType::Coin, "-", "moeda das ruas", Color{248, 205, 84, 255});

    DrawText(TextFormat("HP %d/%d", player_.hp(), player_.maxHp()), 296, 279, 10, Color{244, 102, 106, 255});
    DrawText(TextFormat("ATQ %d (+%d run)", player_.attack(), player_.permanentAttackBonus()), 373, 279, 10, Color{255, 190, 92, 255});
    DrawText(TextFormat("Distrito %dx%d tiles", Dungeon::kWidth, Dungeon::kHeight), 296, 298, 9, Color{137, 188, 218, 255});
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
    if (boss_.engaged())
    {
        DrawText(TextFormat("Boss: %s / %s", bossStateName(boss_.state()), bossAttackName(boss_.attack())), 16, 144, 8, MAGENTA);
    }
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
