#pragma once

namespace ECSEngine
{

/**
 * Adds each entity's gravity acceleration to its velocity once per frame
 */
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

            // Add gravity to current velocity
            Point2D &velocity = entities.template GetComponent<MovementComponent>(owner).velocity;
            velocity.x += gravity.acceleration.x;
            velocity.y += gravity.acceleration.y;
        }
        return kContinue;
	}
	
};


}



