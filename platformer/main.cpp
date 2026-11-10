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

// Loads a map file and creates one entity per tile. Format (see the project spec):
//   dictionary <1|2>
//   <char> <sheet> <sx sy sw sh> [<bx by bw bh>]   one line per tile type; the
//                                                  box is only there for type 2
//   map origin <left top> tile <width height> size <columns rows>
//   <rows lines of `columns` characters; characters not in the dictionary are empty>
// Every tile gets a LocationComponent and SpriteComponent. Type 2 tiles also get a
// static CollisionComponent, and 'S' tiles also get a star SpawnComponent.
// Throws std::runtime_error if the file is missing or malformed.
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
    Engine engine(1024, 768, "Test Engine");

    // Load sounds

    // Load maps. The sky goes first so its sprites are drawn behind the world.
    LoadMap(engine, gResourcePath, "sky.map");
    LoadMap(engine, gResourcePath, "world.map");

    // Place player

    // Setup score component

    // Setup Camera

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
