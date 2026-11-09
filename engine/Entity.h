#pragma once

#include <cstddef>
#include <limits>
#include <string>

// An entity can have any component it is templated with,
// but does not have these installed by default.
// This is a base class for all entities in the ECS system.

namespace ECSEngine {

using EntityID = size_t;

// ID stored in an Entity whose slot is currently unused (the entity was removed).
inline constexpr EntityID kNullEntity = std::numeric_limits<EntityID>::max();

class Entity {
public:
    EntityID GetID() const { return mID; }

    // False for a slot whose entity has been removed. Code that iterates the
    // EntityManager must skip invalid entities.
    bool IsValid() const { return mID != kNullEntity; }

private:
    Entity() = default;
    Entity(EntityID eid, const std::string& name) : mID(eid), mType(name) {}
    EntityID mID = kNullEntity;
    std::string mType;

    template <typename... T>
    friend class EntityManager;
};

}  // namespace ECSEngine
