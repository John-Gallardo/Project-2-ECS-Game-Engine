#pragma once

#include <cmath>
#include <random>

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SpawnComponent.h"
#include "SpriteComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

// constants
const float kStarSize = 64;         // one tile
const float kStarMinSpeed = 6;
const float kStarMaxSpeed = 14;
const float kStarGravity = 0.5f;    // lighter than the player's, so stars float a little
const float kTwoPi = 6.2831853f;


/**
    * Counts down every spawner. When a spawner's timer runs out, a star is created
    * just above it, flying in a random direction. Stops after totalSpawns stars.
*/
template <typename... Components>
class SpawnSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("SpawnSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entities = engine->GetEntityManager();
        auto& spawners = entities.template GetComponentStorage<SpawnComponent>();
        
        for (size_t i = 0; i < spawners.mStorage.size(); ++i) {
            if (!spawners.mValid[i]) {
                continue;
            }

            SpawnComponent& spawner = spawners.mStorage[i];
            if (spawner.totalSpawns <= 0 || --spawner.timeToNextSpawn > 0) {
                continue;
            }

            const EntityID owner = spawners.mOwner[i];
            if (!entities.template HasComponent<LocationComponent>(owner)) {
                continue;
            }

            spawner.timeToNextSpawn = spawner.timeBetweenSpawns;
            --spawner.totalSpawns;

            // A copy, not a reference: adding the star's LocationComponent can
            // move the location pool in memory
            const Point2D spawnerLoc = entities.template GetComponent<LocationComponent>(owner).loc;
            SpawnStar(entities, {spawnerLoc.x, spawnerLoc.y - kStarSize}, spawner.spawnTextureID);
        }

        return kContinue; 
    }

private:

    // Creates one moving star at "at" with a random direction and speed
    void SpawnStar(EntityManager<Components...>& entities, Point2D at, SpriteID texture) {
        const float angle = mAngle(mRandom);
        const float speed = mSpeed(mRandom);

        const EntityID star = entities.CreateEntity("star");
        entities.AddComponent(star, LocationComponent{at});
        entities.AddComponent(star, MovementComponent{{speed * std::cos(angle), speed * std::sin(angle)}});
        entities.AddComponent(star, GravityComponent{{0.0f, kStarGravity}});
        entities.AddComponent(star, SpriteComponent{texture, {{0.0f, 0.0f}, kStarSize, kStarSize}, true});

        CollisionComponent collision{}; // staticEntity = 0, so the CollisionSystem moves it
        collision.collisionBox = {{0.0f, 0.0f}, kStarSize, kStarSize};
        entities.AddComponent(star, collision);
    }

    std::mt19937 mRandom{std::random_device{}()};
    std::uniform_real_distribution<float> mAngle{0.0f, kTwoPi};
    std::uniform_real_distribution<float> mSpeed{kStarMinSpeed, kStarMaxSpeed};
};

}  // namespace ECSEngine
