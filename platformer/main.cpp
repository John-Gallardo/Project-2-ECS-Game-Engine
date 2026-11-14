#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"

std::string gResourcePath = "../assets/";
const unsigned int kWindowWidth = 1024;
const unsigned int kWindowHeight = 768;

#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

// The star the spawners create, in spritesheet-tiles-default.png.
const std::string kStarSheet = "spritesheet-tiles-default.png";
const ECSEngine::Rect kStarSource{{640.f, 320.f}, 64.f, 64.f};
// Spawner tuning, in frames (60 per second).
const int kFirstSpawnDelay = 60;
const int kFramesBetweenSpawns = 180;
const int kTotalSpawns = 20;

// One dictionary line of a map file.
struct MapTile {
    ECSEngine::SpriteID sprite;
    bool solid;               // dictionary 2: the tile has a collision box
    ECSEngine::Rect collide;  // relative to the tile's top-left corner
};

// Loads a map file and creates one entity per tile.
//
// File format (from the project spec)
//   dictionary <1|2>
//   <char> <sheet> <sprite rect> [<collision box>]   one line per tile type
//   map origin <left top> tile <width height> size <columns rows>
//   <the grid: one character per tile, unknown characters are empty>
//
// Every tile gets a location and a sprite. With "dictionary 2" tiles are also
// solid, and 'S' tiles also spawn stars. Throws if the file is missing or broken.
void LoadMap(Engine& engine, const std::string& path, const std::string& file) {
    std::ifstream in(path + file);
    if (!in) throw std::runtime_error("LoadMap: could not open '" + path + file + "'");
    auto fail = [&](const std::string& why) {
        throw std::runtime_error("LoadMap: " + file + ": " + why);
    };

    auto& sprites = engine.GetSpriteManager();
    auto& entities = engine.GetEntityManager();

    // 1. Dictionary: map each character to a sprite (and collision box).
    std::string word;
    int dictionaryType = 0;
    if (!(in >> word >> dictionaryType) || word != "dictionary" ||
        (dictionaryType != 1 && dictionaryType != 2)) {
        fail("expected 'dictionary 1' or 'dictionary 2'");
    }
    std::unordered_map<char, MapTile> dictionary;
    while (in >> word && word != "map") {
        if (word.size() != 1) fail("tile name '" + word + "' must be one character");
        std::string sheet;
        ECSEngine::Rect source{};
        in >> sheet >> source.topLeft.x >> source.topLeft.y >> source.width >> source.height;
        MapTile tile{0, dictionaryType == 2, {}};
        if (tile.solid) {
            in >> tile.collide.topLeft.x >> tile.collide.topLeft.y >> tile.collide.width >>
                tile.collide.height;
        }
        if (!in) fail("bad dictionary line for '" + word + "'");
        tile.sprite = sprites.RegisterTexture(path + sheet, source);
        dictionary[word[0]] = tile;
    }

    // 2. Header: "map origin <left top> tile <width height> size <columns rows>".
    std::string originWord, tileWord, sizeWord;
    ECSEngine::Point2D origin;
    float tileWidth = 0, tileHeight = 0;
    int columns = 0, rows = 0;
    in >> originWord >> origin.x >> origin.y >> tileWord >> tileWidth >> tileHeight >> sizeWord >>
        columns >> rows;
    if (!in || word != "map" || originWord != "origin" || tileWord != "tile" ||
        sizeWord != "size") {
        fail("bad 'map origin ... tile ... size ...' line");
    }

    // Only registered when the map has a spawner, so sky.map adds no extra sprite.
    std::optional<ECSEngine::SpriteID> starSprite;

    // 3. Grid: one entity per dictionary character.
    for (int row = 0; row < rows; ++row) {
        std::string line;
        if (!(in >> line) || static_cast<int>(line.size()) < columns) {
            fail("row " + std::to_string(row) + " is missing or too short");
        }
        for (int col = 0; col < columns; ++col) {
            const char symbol = line[col];
            auto found = dictionary.find(symbol);
            if (found == dictionary.end()) continue;  // e.g. '.', empty space
            const MapTile& tile = found->second;

            ECSEngine::EntityID id = entities.CreateEntity(file + " " + symbol);
            const ECSEngine::Point2D loc{origin.x + col * tileWidth, origin.y + row * tileHeight};
            entities.AddComponent(id, ECSEngine::LocationComponent{loc});
            // Sprite and collision boxes are relative to the location (see LocationComponent).
            entities.AddComponent(
                id, ECSEngine::SpriteComponent{tile.sprite, {{0.f, 0.f}, tileWidth, tileHeight},
                                               true});
            if (tile.solid) {
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = 1;
                collision.doesDamage = (symbol == 'x');
                collision.collisionBox = tile.collide;
                collision.previousCollisionBox = tile.collide;
                entities.AddComponent(id, collision);
            }
            if (symbol == 'S') {
                if (!starSprite) starSprite = sprites.RegisterTexture(path + kStarSheet, kStarSource);
                entities.AddComponent(
                    id, ECSEngine::SpawnComponent{id, "star", *starSprite, kFirstSpawnDelay,
                                                  kFramesBetweenSpawns, kTotalSpawns});
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(kWindowWidth, kWindowHeight, "Test Engine");
    auto& entities = engine.GetEntityManager();
    auto& sprites = engine.GetSpriteManager();

    // Load sounds
    auto& sounds = engine.GetSoundManager();
    sounds.RegisterSound(gResourcePath + "sfx_jump.ogg", "jump");
    sounds.RegisterSound(gResourcePath + "sfx_gem.ogg", "star");
    sounds.RegisterSound(gResourcePath + "footstep_snow_001.ogg", "wallslide");
    sounds.RegisterSound(gResourcePath + "laserLarge_000.ogg", "dash");
    sounds.RegisterSound(gResourcePath + "error_008.ogg", "hurt");
    sounds.RegisterSound(gResourcePath + "footstep_grass_003.ogg", "land");

    // Load maps. The sky goes first so its sprites are drawn behind the world.
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // One extra star past the first wall, to test wall jumping
    // It has no MovementComponent, so it floats in place until collected
    const ECSEngine::EntityID bonusStar = entities.CreateEntity("star");
    const ECSEngine::SpriteID starSprite = sprites.RegisterTexture(gResourcePath + kStarSheet, kStarSource);
    entities.AddComponent(bonusStar, ECSEngine::LocationComponent{{33 * 64.0f, 2 * 64.0f}});
    entities.AddComponent(bonusStar, ECSEngine::SpriteComponent{starSprite, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});
    ECSEngine::CollisionComponent bonusCollision{};
    bonusCollision.collisionBox ={{0.0f, 0.0f}, 64.0f, 64.0f};
    entities.AddComponent(bonusStar, bonusCollision);

    // Place player
    const ECSEngine::Point2D kPlayerStart{832.f, 512.f};
    const ECSEngine::SpriteID playerSprite = sprites.RegisterTexture(gResourcePath + "spritesheet-characters-default.png", {{0.0f, 0.0f}, 128.0f, 128.0f});
    const ECSEngine::EntityID player = entities.CreateEntity("player");
    
    // player components
    entities.AddComponent(player, ECSEngine::LocationComponent{kPlayerStart});
    entities.AddComponent(player, ECSEngine::MovementComponent{});
    entities.AddComponent(player, ECSEngine::GravityComponent{{0.0f, 2.0f}});
    entities.AddComponent(player, ECSEngine::InputComponent{player});
    entities.AddComponent(player, ECSEngine::RespawnComponent{kPlayerStart});
    entities.AddComponent(player, ECSEngine::SpriteComponent{playerSprite, {{0.0f, 0.0f}, 64.0f, 64.0f}, true});

    ECSEngine::CollisionComponent playerCollision{};
    playerCollision.collisionBox = {{12.0f, 12.0f}, 40.0f, 52.0f};
    playerCollision.previousCollisionBox = playerCollision.collisionBox;
    entities.AddComponent(player, playerCollision);
    entities.AddComponent(player, ECSEngine::CameraFollower{player});


    // Setup score component
    ECSEngine::ScoreComponent score{};
    for (int d = 0; d < 10; ++d) {
        // Digits are stacked 9 (top) to 0 (bottom) in one column of the tile sheet
        score.digitTextureID[d] = sprites.RegisterTexture(gResourcePath + "spritesheet-tiles-default.png", {{832.0f, 832.0f - 64.0f * d}, 64.0f, 64.0f});
    }

    for (int i = 0; i < 3; ++i) {
        const ECSEngine::EntityID digit = entities.CreateEntity("score digit");
        entities.AddComponent(digit, ECSEngine::LocationComponent{{16.f + 40.f * i, 16.f}});
        entities.AddComponent(digit,
        ECSEngine::SpriteComponent{score.digitTextureID[0], {{0.f, 0.f}, 48.f, 48.f}, false});
        score.digitID[i] = digit;
    }

    const ECSEngine::EntityID scoreEntity = entities.CreateEntity("score");
    entities.AddComponent(scoreEntity, score);

    // Setup Camera
    const float kWorldHeight = 14 * 64.0f;
    const float w = static_cast<float>(kWindowWidth);
    const float h = static_cast<float>(kWindowHeight);

    ECSEngine::CameraComponent camera{};
    camera.position = {kPlayerStart.x, kWorldHeight / 2};
    camera.worldUnitsPerPixel = kWorldHeight / h;
    camera.hardExclusion = {{0.1f * w, 0.0f}, 0.8f * w, h};
    camera.softExclusion = {{0.3f * w, 0.0f}, 0.4f * w, h};
    camera.speed = 0.05f;

    const ECSEngine::EntityID cameraEntity = entities.CreateEntity("camera");
    entities.AddComponent(cameraEntity, camera);
    entities.AddComponent(cameraEntity, ECSEngine::CameraShake{0, {0.0f, 0.0f}});

    engine.GetWindowManager().SetWorldScale(camera.worldUnitsPerPixel);
    engine.GetWindowManager().SetWorldCenter(camera.position);

    // Add all systems
    auto& sm = engine.GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.Run();
    return 0;
}
