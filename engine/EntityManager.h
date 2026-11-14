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

// Owns every entity and component in the game.
//
// How it works
//   - Entities live in a vector. An EntityID is just an index into it.
//   - Each component type has its own pool (ComponentStorage<T>).
//   - mComponentSlots records which slot in each pool belongs to an entity.
//   - Removed entities and components leave gaps that are reused later.
//
// Lifetimes
//   - The EntityManager frees everything; callers never delete anything.
//   - An ID can be reused after RemoveEntity, so check IsValid() on stored IDs.
//   - Component references last only until the next AddComponent or
//     RemoveEntity (the pool may grow and move). Don't store them.
//   - CreateEntity invalidates entity iterators. Loops must skip invalid entities.
template <typename... Components>
class EntityManager {
public:
    static constexpr size_t kNullComponent = std::numeric_limits<size_t>::max();

    // Makes an entity with no components and returns its ID. `name` is its type, e.g. "player".
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

    // Deletes the entity and all its components. Its ID may be reused later.
    void RemoveEntity(EntityID entity) {
        assert(IsValid(entity));
        (RemoveComponent<Components>(entity), ...);
        mEntities[entity] = Entity();  // a default Entity counts as invalid
        mFreeEntities.push_back(entity);
    }

    // Adds a T to the entity, or replaces the one it already has.
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

    // Removes the entity's T, if it has one.
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

    // True if the entity exists and has a T. Safe with any ID.
    template <typename T>
    bool HasComponent(EntityID entity) const {
        return IsValid(entity) && mComponentSlots[entity][IndexOf<T>()] != kNullComponent;
    }

    // Returns the entity's T. Check HasComponent first.
    template <typename T>
    T& GetComponent(EntityID entity) {
        assert(HasComponent<T>(entity));
        return GetComponentStorage<T>().mStorage[mComponentSlots[entity][IndexOf<T>()]];
    }

    // The whole pool of T, for systems that loop over one component type.
    // Skip slots where mValid[i] is false; mOwner[i] is the slot's entity.
    template <typename T>
    ComponentStorage<T>& GetComponentStorage() {
        return std::get<ComponentStorage<T>>(mStorages);
    }

    // True if the entity exists right now.
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
    // Position of T in the Components list (column in mComponentSlots).
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
    // [entity][IndexOf<T>()] = slot of the entity's T in its pool, or kNullComponent
    std::vector<std::array<size_t, sizeof...(Components)>> mComponentSlots;
    std::tuple<ComponentStorage<Components>...> mStorages;
};

}  // namespace ECSEngine
