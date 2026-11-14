#pragma once

#include <string>

#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

// Moves every entity by its velocity once per frame.
//
// How it works
//   - Only entities with both a MovementComponent and a LocationComponent move.
//   - Velocity is in world units per frame (the game runs at a fixed 60 fps,
//     so there is no delta time).
//   - Runs after InputSystem and GravitySystem have set this frame's velocity,
//     and before CollisionSystem, which pushes entities back out of walls.
//   - It does not check for walls itself; a fast entity can end up inside one
//     until CollisionSystem fixes it.
template <typename... Components>
class MovementSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("MovementSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto &entities{engine->GetEntityManager()};

        // Go through only entities with the movement component
        auto &movements = entities.template GetComponentStorage<MovementComponent>();
        for (size_t i{0}; i < movements.mStorage.size(); i++) {
            // Edge case: invalid movement component
            if (!movements.mValid[i])
                continue;

            EntityID owner{movements.mOwner[i]};
            // Edge case: no location component to add to
            if (!entities.template HasComponent<LocationComponent>(owner))
                continue;

            // position += velocity
            Point2D &velocity{movements.mStorage[i].velocity};
            Point2D &location{entities.template GetComponent<LocationComponent>(owner).loc};
            location.x += velocity.x;
            location.y += velocity.y;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
