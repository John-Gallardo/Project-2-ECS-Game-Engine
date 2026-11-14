#pragma once

#include <algorithm>

#include "EntityManager.h"
#include "GravityComponent.h"
#include "InputComponent.h"
#include "Keyboard.h"
#include "LocationComponent.h"
#include "MovementComponent.h"
#include "SystemManager.h"

namespace ECSEngine
{

// Constants
const float kFallGravityScale = 2.0f;       // falling pulls harder than rising
const float kFastFallGravityScale = 3.0f;   // holding down pulls harder still
const float kMaxFallVelocity = 20.0f;
const float kMaxFastFallVelocity = 35.0f;   // kept under a tile's height so nothing falls through one

// Adds each entity's gravity to its velocity once per frame.
// Falling uses stronger gravity than rising, and holding down (player only)
// makes it stronger again. Fall speed is capped.
template <typename... Components>
class GravitySystem : public System<Components...> {
public:
	const std::string & GetName() const override
	{ static std::string name("GravitySystem"); return name; }
	
	tSystemReturn Run(EngineView<Components...> *engine) override
	{
        auto &entities{engine->GetEntityManager()};

        // Go through only entities with the gravity component
        auto &gravities{entities.template GetComponentStorage<GravityComponent>()};
        for (size_t i{0}; i < gravities.mStorage.size(); i++) {
            // Edge case: gravity component is invalid
            if (!gravities.mValid[i])
                continue;

            EntityID owner{gravities.mOwner[i]};
            // Edge Case: must have movement component for gravity to act
            if (!entities.template HasComponent<MovementComponent>(owner))
                continue;

            GravityComponent &gravity = gravities.mStorage[i];
            // Edge Case: timer is disabled -> just decerement timer & move onto next iteration
            if (gravity.disabledTimer > 0) {
                --gravity.disabledTimer;
                continue;
            }

            Point2D &velocity = entities.template GetComponent<MovementComponent>(owner).velocity;
            const bool holdingDown{entities.template HasComponent<InputComponent>(owner) &&
                                   (entities.template GetComponent<InputComponent>(owner).keydown[Key::Down] ||
                                    entities.template GetComponent<InputComponent>(owner).keydown[Key::S])};

            float scale{1.0f};
            float maxFall{kMaxFallVelocity};
            
            if (holdingDown) {
                scale = kFastFallGravityScale;
                maxFall = kMaxFastFallVelocity;
            } 
            
            else if (velocity.y > 0) {
                scale = kFallGravityScale;
            }

            // Add gravity to current velocity
            velocity.x += gravity.acceleration.x;
            velocity.y += gravity.acceleration.y * scale;
            velocity.y = std::min(velocity.y, maxFall);
        }
        return kContinue;
	}
	
};


}



