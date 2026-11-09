#pragma once

#include <vector>

#include "Entity.h"

namespace ECSEngine {

template <typename T>
class ComponentStorage {
public:
    std::vector<T> mStorage;
    std::vector<bool> mValid;       // Quickly track if a component is valid
    std::vector<size_t> mFreeList;  // Avoid scanning for free slots
    std::vector<EntityID> mOwner;   // Entity that owns each slot
};

}  // namespace ECSEngine
