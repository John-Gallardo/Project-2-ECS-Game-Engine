#pragma once

#include <array>
#include <cassert>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "Component.h"
#include "Entity.h"

namespace ECSEngine {

// Owns every entity and every component in the game.
//
// Memory layout
//   - Entities live in one vector. An EntityID is an index into it.
//   - Each component type T has its own ComponentStorage<T> (a pool). An
//     entity's T, if it has one, sits at some slot in that pool. A per-entity
//     table maps (entity, component type) to the slot, or kNullComponent.
//   - Removing an entity or component leaves a gap. Gaps go on a free list and
//     are reused by the next create/add, so the vectors stay compact.
//
// Rules for callers (who may hold what, and for how long)
//   - The EntityManager owns all entities and components; callers never delete
//     or free anything themselves.
//   - An EntityID stays valid until RemoveEntity is called on it. After that the
//     ID may be handed out again to a brand-new entity, so code that stores IDs
//     (e.g. a camera follower) must stop using an ID once its entity is removed.
//     Use IsValid() to check an ID before using it.
//   - References from GetComponent<T>() and iterators/references into
//     GetComponentStorage<T>() are only valid until the next AddComponent<T>
//     (any entity) or RemoveEntity, because the storage vector may reallocate.
//     Re-fetch them each frame; do not store them.
//   - Entity iterators are invalidated by CreateEntity. Iteration visits removed
//     slots too: always skip entities where IsValid() is false.
template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // Creates an entity with no components. `name` describes the entity's type
    // (e.g. "player", "star"). Returns its ID, which may be a recycled ID.
    [[nodiscard]] EntityID CreateEntity(const std::string& name) {
        EntityID id;
        if (!mFreeEntities.empty()) {
            id = mFreeEntities.back();
            mFreeEntities.pop_back();
            mEntities[id] = Entity(id, name);
        } else {
            id = mEntities.size();
            mEntities.push_back(Entity(id, name));
            mComponentSlots.emplace_back();
        }
        mComponentSlots[id].fill(kNullComponent);
        return id;
    }

    // Removes the entity and all of its components. `entity` must be valid.
    // The ID becomes invalid and may be reused by a later CreateEntity.
    void RemoveEntity(EntityID entity) {
        assert(IsValid(entity));
        (RemoveComponent<Components>(entity), ...);
        mEntities[entity] = Entity();  // default Entity has kNullEntity as its ID
        mFreeEntities.push_back(entity);
    }

    // Gives `entity` a copy of `component`. If the entity already has a T, the
    // old value is replaced. `entity` must be valid.
    template <typename T>
    void AddComponent(EntityID entity, T component) {
        assert(IsValid(entity));
        ComponentStorage<T>& storage = GetComponentStorage<T>();
        size_t& slot = mComponentSlots[entity][IndexOf<T>()];

        if (slot != kNullComponent) {
            storage.mStorage[slot] = std::move(component);
            return;
        }

        if (!storage.mFreeList.empty()) {
            slot = storage.mFreeList.back();
            storage.mFreeList.pop_back();
            storage.mStorage[slot] = std::move(component);
            storage.mValid[slot] = true;
            storage.mOwner[slot] = entity;
        } else {
            slot = storage.mStorage.size();
            storage.mStorage.push_back(std::move(component));
            storage.mValid.push_back(true);
            storage.mOwner.push_back(entity);
        }
    }

    // Removes the entity's T. Does nothing if the entity has no T or is invalid.
    template <typename T>
    void RemoveComponent(EntityID entity) {
        if (!HasComponent<T>(entity)) return;
        ComponentStorage<T>& storage = GetComponentStorage<T>();
        size_t& slot = mComponentSlots[entity][IndexOf<T>()];
        storage.mValid[slot] = false;
        storage.mOwner[slot] = kNullEntity;
        storage.mFreeList.push_back(slot);
        slot = kNullComponent;
    }

    // True if `entity` is valid and has a T. Safe to call with any ID.
    template <typename T>
    bool HasComponent(EntityID entity) const {
        return IsValid(entity) && mComponentSlots[entity][IndexOf<T>()] != kNullComponent;
    }

    // Returns the entity's T. The entity must have one (check HasComponent).
    // See the class comment for how long the reference stays valid.
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        return GetComponentStorage<T>().mStorage[mComponentSlots[entity][IndexOf<T>()]];
    }

    // Direct access to every T, for systems that only need to walk one
    // component type. Skip slots where mValid[i] is false; mOwner[i] is the
    // entity that owns slot i. Do not add or remove slots through this
    // reference; use AddComponent/RemoveComponent.
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    // True if `entity` refers to an entity that exists right now.
    bool IsValid(EntityID entity) const {
        return entity < mEntities.size() && mEntities[entity].IsValid();
    }

    using tEntity = std::vector<Entity>;
    using const_iterator = tEntity::const_iterator;
    const_iterator cbegin() const { return mEntities.begin(); }
    const_iterator cend() const { return mEntities.end(); }
    const_iterator begin() { return mEntities.begin(); }
    const_iterator end() { return mEntities.end(); }

private:
    // Position of T in the Components... list, used to index mComponentSlots.
    template <typename T>
    static constexpr size_t IndexOf() {
        static_assert((std::is_same_v<T, Components> || ...),
                      "T is not one of this EntityManager's component types");
        constexpr bool matches[] = {std::is_same_v<T, Components>...};
        for (size_t i = 0; i < sizeof...(Components); ++i) {
            if (matches[i]) return i;
        }
        return kNullComponent;
    }

    tEntity mEntities;
    std::vector<EntityID> mFreeEntities;  // removed IDs waiting to be reused
    // mComponentSlots[entity][IndexOf<T>()] = slot of the entity's T, or kNullComponent
    std::vector<std::array<size_t, sizeof...(Components)>> mComponentSlots;
    std::tuple<ComponentStorage<Components>...> mStorages;
};

}  // namespace ECSEngine
