#pragma once

#include <algorithm>
#include <cmath>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "SystemManager.h"

namespace ECSEngine {

// Constants
const float kCameraSnapDistance = 1.0f;
const float kShakeDecay = 0.7f; // each frame the shake keeps 70% of its size

/**
    * Moves the camera along x to follow the CameraFollower's entity. The y stays fixed
    * The target is never allowed outside hardExclusion (the camera follows it directly there).
    * Outside softExclusion the camera tweens toward it at camera.speed until it is back inside
    * Camera shake is added on top for the frame only, so it never moves the real camera
*/
template <typename... Components>
class CameraSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name = "CameraSystem";
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override { 
        auto& entities = engine->GetEntityManager();
        auto& window = engine->GetWindowManager();
        auto& cameras = entities.template GetComponentStorage<CameraComponent>();
        const CameraFollower* follower = FindFirst<CameraFollower>(entities);

        for (size_t i = 0; i < cameras.mStorage.size(); ++i) {
            if (!cameras.mValid[i]) {
                continue;
            }

            CameraComponent& camera = cameras.mStorage[i];
            if (follower && entities.template HasComponent<LocationComponent>(follower->entityID)) {
                Follow(camera, TargetX(entities, follower->entityID), window.GetWindowBounds().width);
            }

            Point2D center = camera.position;
            const EntityID owner = cameras.mOwner[i];
            if (entities.template HasComponent<CameraShake>(owner)) {
                CameraShake& shake = entities.template GetComponent<CameraShake>(owner);

                if (shake.frameLimit > 0) {
                    // Flip side every frame and shrink, so the view rattles and then settles
                    const float side = (shake.frameLimit % 2 == 0) ? 1.0f : -1.0f;

                    center.x += side * shake.offset.x * camera.worldUnitsPerPixel;
                    center.y += side * shake.offset.y * camera.worldUnitsPerPixel;
                    shake.offset = kShakeDecay * shake.offset;

                    --shake.frameLimit;
                }
            }

            window.SetWorldScale(camera.worldUnitsPerPixel);
            window.SetWorldCenter(center);
            return kContinue;   // only one camera can drive the window
        }
        return kContinue; 
    }

private:

    // Moves camera.position.x so the target ends up inside the soft zone, never outside the hard zone
    static void Follow(CameraComponent& camera, float targetX, float windowWidth) {
        const float scale = camera.worldUnitsPerPixel;
        const float screenX = (targetX - camera.position.x) / scale + windowWidth / 2;  // target on screen now

        const float softLeft = camera.softExclusion.topLeft.x;
        const float softRight = softLeft + camera.softExclusion.width;
        const float hardLeft = camera.hardExclusion.topLeft.x;
        const float hardRight = hardLeft + camera.hardExclusion.width;

        // How far (in pixels) the target is past the soft zone, 0 while inside it
        float outside = 0;
        if (screenX < softLeft) {
            outside = screenX - softLeft;
        } else if (screenX > softRight) {
            outside = screenX - softRight;
        }

        float move = (std::abs(outside) < kCameraSnapDistance) ? outside : outside * camera.speed;

        // Moving the camera right by "move" moves the target left on the screen by "move"
        move = std::clamp(move, screenX - hardRight, screenX - hardLeft);
        camera.position.x += move * scale;
    }

    // World x the camera follows: the middle of the collision box, or the location if there is none
    static float TargetX(EntityManager<Components...>& entities, EntityID id) {
        float x = entities.template GetComponent<LocationComponent>(id).loc.x;

        if (entities.template HasComponent<CollisionComponent>(id)) {
            const Rect& box = entities.template GetComponent<CollisionComponent>(id).collisionBox;
            x += box.topLeft.x + box.width / 2;
        }

        return x;
    }

    // Returns the first valid T, or nullptr if there is none
    template <typename T>
    static T* FindFirst(EntityManager<Components...>& entities) {
        auto& storage = entities.template GetComponentStorage<T>();

        for (size_t i = 0; i < storage.mStorage.size(); ++i) {
            if (storage.mValid[i]) {
                return &storage.mStorage[i];
            }
        }

        return nullptr;
    }

};

}  // namespace ECSEngine
