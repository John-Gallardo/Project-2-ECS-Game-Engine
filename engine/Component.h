#pragma once

#include <vector>

#include "Entity.h"

namespace ECSEngine {

// The pool for one component type, owned by the EntityManager.
//   - Slot i holds a T in mStorage[i]. It is only in use when mValid[i] is true.
//   - Systems may read and change the T values, but add or remove slots only
//     through EntityManager::AddComponent / RemoveComponent.
template <typename T>
class ComponentStorage {
public:
    std::vector<T> mStorage;
    std::vector<bool> mValid;       // Quickly track if a component is valid
    std::vector<size_t> mFreeList;  // Avoid scanning for free slots
    std::vector<EntityID> mOwner;   // Entity that owns each slot
};

}  // namespace ECSEngine
