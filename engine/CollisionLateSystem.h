#pragma once

#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

template <typename... Components>
class CollisionLateSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionLateSystem");
        return name;
    }

    /*
    Runs first each frame. Saves where every collision box is before anything
    moves, so the CollisionSystem can compare the old and new positions and
    tell which side an entity hit from
    */

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        
        auto& entities = engine->GetEntityManager();

        auto& collisions = entities.template GetComponentStorage<CollisionComponent>();

        for (size_t i = 0; i < collisions.mStorage.size(); ++i) {
            if (!collisions.mValid[i]) {
                continue;
            }

            const EntityID owner = collisions.mOwner[i];
            if (!entities.template HasComponent<LocationComponent>(owner)) {
                continue;
            }

            CollisionComponent& c = collisions.mStorage[i];
            const Point2D& loc = entities.template GetComponent<LocationComponent>(owner).loc;

            // collisionBox is relative to the location, but previousCollisionBox
            // is saved in world space so it still holds last frame's position
            c.previousCollisionBox = c.collisionBox;
            c.previousCollisionBox.topLeft = Point2D{loc.x + c.collisionBox.topLeft.x, loc.y + c.collisionBox.topLeft.y};
        }
        return kContinue; 
    }
};

}  // namespace ECSEngine
