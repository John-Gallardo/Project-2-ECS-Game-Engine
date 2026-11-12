#pragma once

namespace ECSEngine {

/**
 * Moves every entity by its velocity once per frame
 */
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

            // Add entity's velocity to its position
            Point2D &velocity{movements.mStorage[i].velocity};
            Point2D &location{entities.template GetComponent<LocationComponent>(owner).loc};
            location.x += velocity.x;
            location.y += velocity.y;
        }
        return kContinue;
    }
};

}  // namespace ECSEngine
