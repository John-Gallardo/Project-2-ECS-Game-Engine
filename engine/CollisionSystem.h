#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "ScoreComponent.h"
#include "SystemManager.h"

namespace ECSEngine {

const float kGridCellSize = 64;     // One map tile
const float kStarBounce = 0.7f;     // stars keep 70% of their speed when they hit a wall
const float kMinShakeSpeed = 4;     // softer hits
const float kShakePerSpeed = 0.3f;  // shake distance in pixels per unit of impact speed
const int kShakeFrames = 6;
const float kTouchEpsilon = 0.01f;  // slack for float rounding when comparing edges

/*
    Finds overlapping collision boxes and resolves them:
    Moving boxes (player, stars) are pushed out of static boxes. The
    side that was hit sets onGround/onCeiling/onLeftWall/onRightWall
    A static box with doesDamage (spikes) sends the player to its RespawnComponent

    The player collects any star it touches. Two stars bounce off each other

    Static boxes never move, so they are put into a grid once. Each moving entity
    then only checks the tiles in the grid cells it covers, not the whole map
    There are only a few moving entities, so those are checked pair by pair
*/

template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    // Runs once per frame: sorts boxes, pushes movers out of the map,
    // handles player/star and star/star hits, then removes collected stars
    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();
        auto& collisions = entities.template GetComponentStorage<CollisionComponent>();

        // Sort the collision slots into moving and static ones
        mDynamic.clear();
        size_t staticCount = 0;
        for (size_t i = 0; i < collisions.mStorage.size(); ++i) {
            if (!collisions.mValid[i]) {
                continue;
            }

            if (!entities.template HasComponent<LocationComponent>(collisions.mOwner[i])) {
                continue;
            }

            if (collisions.mStorage[i].staticEntity) {
                ++staticCount;
            }
            else {
                mDynamic.push_back(i);
            }
        }

        // Rebuild the grid only when the number of static boxes changes
        if (staticCount != mGridStaticCount) {
            BuildGrid(entities);
        }

        CameraShake* shake = FindFirst<CameraShake>(entities);
        for (size_t slot : mDynamic) {
            ResolveAgainstStatic(engine, slot, shake);
        }
        ResolveDynamicPairs(engine);

        // Removed after the loops so not slot disappears while it is being checked
        for (EntityID star : mToRemove) {
            entities.RemoveEntity(star);
        }
        mToRemove.clear();
        return kContinue; 
    }

private:
    struct StaticBox {
        Rect box; // world space
        bool doesDamage;
    };

    enum class HitSide { kTop, kBottom, kLeft, kRight };    // side of static box that was hit

    // Turns a box relative to an entity into a box in world space
    static Rect WorldBox(const Rect& box, const Point2D& loc) {
        return {{loc.x + box.topLeft.x, loc.y + box.topLeft.y}, box.width, box.height};
    }

    // True if the two boxes overlap. Touching edges do not count
    static bool Overlaps(const Rect& a, const Rect& b) {
        return a.topLeft.x < b.topLeft.x + b.width &&
               b.topLeft.x < a.topLeft.x + a.width &&
               a.topLeft.y < b.topLeft.y + b.height &&
               b.topLeft.y < a.topLeft.y + a.height; 
    }

    // Returns the grid column (or row) that a world x (or y) falls in
    static int Cell(float worldCoord) {
        return static_cast<int>(std::floor(worldCoord / kGridCellSize));
    }

    // Packs a cell's column and row into one number to use as the grid key
    static int64_t CellKey(int column, int row) {
        return (static_cast<int64_t>(column) << 32 | static_cast<uint32_t>(row));
    }

    // Returns the first valid T (e.g. the one CameraShake), or nullptr if there is none
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

    // Puts every static box into each grid cell it covers, so movers
    // only have to check the tiles near them
    void BuildGrid(EntityManager<Components...>& entities) {
        auto& collisions = entities.template GetComponentStorage<CollisionComponent>();
        mGrid.clear();
        mGridStaticCount = 0;
        for (size_t i = 0; i < collisions.mStorage.size(); ++i) {
            if (!collisions.mValid[i] || !collisions.mStorage[i].staticEntity) {
                continue;
            }
            const EntityID owner = collisions.mOwner[i];
            if (!entities.template HasComponent<LocationComponent>(owner)) {
                continue;
            }

            const CollisionComponent& c = collisions.mStorage[i];
            const Rect box = WorldBox(c.collisionBox, entities.template GetComponent<LocationComponent>(owner).loc);
            // A box that crosses a cell edge goes into every cell it covers
            for (int col = Cell(box.topLeft.x); col <= Cell(box.topLeft.x + box.width - kTouchEpsilon); ++col) {
                for (int row = Cell(box.topLeft.y); row <= Cell(box.topLeft.y + box.height - kTouchEpsilon); ++row) {
                    mGrid[CellKey(col, row)].push_back({box, c.doesDamage == 1});
                }
            }
            ++mGridStaticCount;
        }
    }

    // Pushes one moving entity out of any static boxes it overlaps and sets its
    // onGround/onCeiling/wall flags. Spikes respawn it. Also plays the land sound
    // and starts the camera shake on hard hits
    void ResolveAgainstStatic(EngineView<Components...>* engine, size_t slot, CameraShake* shake) {
        auto& entities = engine->GetEntityManager();
        auto& collisions = entities.template GetComponentStorage<CollisionComponent>();

        CollisionComponent& c = collisions.mStorage[slot];
        const EntityID owner = collisions.mOwner[slot];

        // The flags describe this frame only. The InputSystem reads them next frame
        const bool wasOnGround = c.onGround;
        c.onGround = c.onCeiling = c.onLeftWall = c.onRightWall = 0;

        if (!entities.template HasComponent<MovementComponent>(owner)) {
            return;
        }

        Point2D& loc = entities.template GetComponent<LocationComponent>(owner).loc;
        Point2D& vel = entities.template GetComponent<MovementComponent>(owner).velocity;
        const bool isPlayer = entities.template HasComponent<InputComponent>(owner);
        const float bounce = isPlayer ? 0.0f : kStarBounce;
        const Rect& prev = c.previousCollisionBox;  // world space, saved by CollisionLateSystem
        const Rect start = WorldBox(c.collisionBox, loc);

        for (int col = Cell(start.topLeft.x); col <= Cell(start.topLeft.x + start.width - kTouchEpsilon); ++col) {
            for (int row = Cell(start.topLeft.y); row <= Cell(start.topLeft.y + start.height - kTouchEpsilon); ++row) {
                auto cell = mGrid.find(CellKey(col, row));
                if (cell == mGrid.end()) {
                    continue;
                }

                for (const StaticBox& solid : cell->second) {
                    // Recomputed each time: an earlier tile may already have pushed us
                    const Rect box = WorldBox(c.collisionBox, loc);
                    if (!Overlaps(box, solid.box)) {
                        continue;
                    }

                    if (solid.doesDamage && entities.template HasComponent<RespawnComponent>(owner)) {
                        loc = entities.template GetComponent<RespawnComponent>(owner).loc;
                        vel = {0.0f, 0.0f};
                        engine->GetSoundManager().GetSound("hurt").play();
                        return;
                    }

                    const HitSide side = FindHitSide(box, prev, solid.box);
                    float impact = 0; //speed going into the wall, for the screen shake
                    Point2D direction;
                    switch (side) {
                        case HitSide::kTop: // landed on it
                            loc.y -= box.topLeft.y + box.height - solid.box.topLeft.y;
                            c.onGround = 1;
                            impact = vel.y;
                            direction = {0.0f, 1.0f};
                            vel.y = -vel.y * bounce;
                            break;
                        case HitSide::kBottom:  // hit it with out head
                            loc.y += solid.box.topLeft.y + solid.box.height - box.topLeft.y;
                            c.onCeiling = 1;
                            impact = -vel.y;
                            direction = {0.0f, -1.0f};
                            vel.y = -vel.y * bounce;
                            break;
                        case HitSide::kLeft:    // ran into it moving right, so the wall is on our right
                            loc.x -= box.topLeft.x + box.width - solid.box.topLeft.x;
                            c.onRightWall = 1;
                            impact = vel.x;
                            direction = {1.f, 0.f};
                            vel.x = -vel.x * bounce;
                            break;
                        case HitSide::kRight:   // ran into it moving left
                            loc.x += solid.box.topLeft.x + solid.box.width - box.topLeft.x;
                            c.onLeftWall = 1;
                            impact = -vel.x;
                            direction = {-1.f, 0.f};
                            vel.x = -vel.x * bounce;
                            break;
                    }

                    if (isPlayer && shake && impact > kMinShakeSpeed) {
                        shake->frameLimit = kShakeFrames;
                        shake->offset = (impact * kShakePerSpeed) * direction;
                    }
                }
            }
        }

        if (isPlayer && c.onGround && !wasOnGround) {
            engine->GetSoundManager().GetSound("land").play();
        }
    }

    // Uses where the box was last frame to tell which side it came in from.
    // If it was already inside (e.g. it spawned there), takes the shortest way out
    static HitSide FindHitSide(const Rect& box, const Rect& prev, const Rect& solid) {
        const float solidRight = solid.topLeft.x + solid.width;
        const float solidBottom = solid.topLeft.y + solid.height;

        if (prev.topLeft.y + prev.height <= solid.topLeft.y + kTouchEpsilon) {
            return HitSide::kTop;
        }
        if (prev.topLeft.y >= solidBottom - kTouchEpsilon) {
            return HitSide::kBottom;
        }
        if (prev.topLeft.x + prev.width <= solid.topLeft.x + kTouchEpsilon) {
            return HitSide::kLeft;
        }
        if (prev.topLeft.x >= solidRight - kTouchEpsilon) {
            return HitSide::kRight;
        }

        const float outTop = box.topLeft.y + box.height - solid.topLeft.y;
        const float outBottom = solidBottom - box.topLeft.y;
        const float outLeft = box.topLeft.x + box.width - solid.topLeft.x;
        const float outRight = solidRight - box.topLeft.x;
        const float shortest = std::min({outTop, outBottom, outLeft, outRight});
        if (shortest == outTop) return HitSide::kTop;
        if (shortest == outBottom) return HitSide::kBottom;
        if (shortest == outLeft) return HitSide::kLeft;
        return HitSide::kRight;

    }

    // Checks every pair of moving entities. Player + star: collect the star.
    // Star + star: push them apart and bounce them off each other
    void ResolveDynamicPairs(EngineView<Components...>* engine) {
        auto& entities = engine->GetEntityManager();
        auto& collisions = entities.template GetComponentStorage<CollisionComponent>();
        mCollected.assign(mDynamic.size(), false);

        for (size_t a = 0; a < mDynamic.size(); ++a) {
            for (size_t b = a + 1; b < mDynamic.size(); ++b) {
                if (mCollected[a] || mCollected[b]) continue;
                const EntityID ownerA = collisions.mOwner[mDynamic[a]];
                const EntityID ownerB = collisions.mOwner[mDynamic[b]];
                Point2D& locA = entities.template GetComponent<LocationComponent>(ownerA).loc;
                Point2D& locB = entities.template GetComponent<LocationComponent>(ownerB).loc;
                const Rect boxA = WorldBox(collisions.mStorage[mDynamic[a]].collisionBox, locA);
                const Rect boxB = WorldBox(collisions.mStorage[mDynamic[b]].collisionBox, locB);
                if (!Overlaps(boxA, boxB)) continue;

                const bool aIsPlayer = entities.template HasComponent<InputComponent>(ownerA);
                const bool bIsPlayer = entities.template HasComponent<InputComponent>(ownerB);
                if (aIsPlayer || bIsPlayer) {
                    // The player collects the star.
                    const size_t star = aIsPlayer ? b : a;
                    mCollected[star] = true;
                    mToRemove.push_back(aIsPlayer ? ownerB : ownerA);
                    if (ScoreComponent* score = FindFirst<ScoreComponent>(entities)) ++score->score;
                    engine->GetSoundManager().GetSound("star").play();
                    continue;
                }

                // Two stars: push them apart along the axis they overlap least,
                // and swap their speeds on that axis like two equal balls.
                if (!entities.template HasComponent<MovementComponent>(ownerA) ||
                    !entities.template HasComponent<MovementComponent>(ownerB)) {
                    continue;
                }
                Point2D& velA = entities.template GetComponent<MovementComponent>(ownerA).velocity;
                Point2D& velB = entities.template GetComponent<MovementComponent>(ownerB).velocity;
                const float overlapX = std::min(boxA.topLeft.x + boxA.width, boxB.topLeft.x + boxB.width) -
                                       std::max(boxA.topLeft.x, boxB.topLeft.x);
                const float overlapY = std::min(boxA.topLeft.y + boxA.height, boxB.topLeft.y + boxB.height) -
                                       std::max(boxA.topLeft.y, boxB.topLeft.y);
                if (overlapX < overlapY) {
                    const float push = (boxA.topLeft.x < boxB.topLeft.x ? overlapX : -overlapX) / 2;
                    locA.x -= push;
                    locB.x += push;
                    std::swap(velA.x, velB.x);
                } else {
                    const float push = (boxA.topLeft.y < boxB.topLeft.y ? overlapY : -overlapY) / 2;
                    locA.y -= push;
                    locB.y += push;
                    std::swap(velA.y, velB.y);
                }
            }
        }
    }


    std::unordered_map<int64_t, std::vector<StaticBox>> mGrid;  // static boxes by grid cell
    size_t mGridStaticCount = 0;                                // static boxes in mGrid, to notice when a map is loaded
    std::vector<size_t> mDynamic;                               // collision slots of moving entities, refilled each frame
    std::vector<bool> mCollected;                               // per mDynamic entry: star already collected this frame
    std::vector<EntityID> mToRemove;                            // stars to remove at the end of the frame
};

}  // namespace ECSEngine
