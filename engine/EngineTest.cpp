// Unit tests for the ECS engine. Build and run the ECSTest target:
//   cmake --build build --target ECSTest && ./build/engine/ECSTest
// Exit code is 0 when every check passes, 1 otherwise.

#include <cstdio>
#include <iterator>
#include <string>
#include <vector>

#include "EntityManager.h"

namespace {

int gChecks = 0;
int gFailures = 0;

#define CHECK(cond)                                                         \
    do {                                                                    \
        ++gChecks;                                                          \
        if (!(cond)) {                                                      \
            ++gFailures;                                                    \
            std::printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                   \
    } while (0)

// Small stand-in components so the tests do not depend on game code.
struct Position {
    int x = 0;
    int y = 0;
};
struct Health {
    int hp = 0;
};
struct Tag {
    char c = 0;
};

using TestManager = ECSEngine::EntityManager<Position, Health, Tag>;

size_t CountValidEntities(TestManager& em) {
    size_t count = 0;
    for (const auto& e : em) {
        if (e.IsValid()) ++count;
    }
    return count;
}

size_t CountValidSlots(const ECSEngine::ComponentStorage<Health>& storage) {
    size_t count = 0;
    for (bool valid : storage.mValid) {
        if (valid) ++count;
    }
    return count;
}

// ---------------------------------------------------------------- entities

void TestCreateGivesDistinctIds() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    auto c = em.CreateEntity("c");
    CHECK(a != b);
    CHECK(b != c);
    CHECK(a != c);
    CHECK(em.IsValid(a));
    CHECK(em.IsValid(b));
    CHECK(em.IsValid(c));
}

void TestIterationVisitsCreatedEntities() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    std::vector<ECSEngine::EntityID> seen;
    for (const auto& e : em) {
        if (e.IsValid()) seen.push_back(e.GetID());
    }
    CHECK(seen.size() == 2);
    CHECK(seen.size() == 2 && seen[0] == a && seen[1] == b);
}

void TestRemovedEntityIsSkippedInIteration() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    auto c = em.CreateEntity("c");
    em.RemoveEntity(b);
    CHECK(!em.IsValid(b));
    CHECK(em.IsValid(a));
    CHECK(em.IsValid(c));
    CHECK(CountValidEntities(em) == 2);
    for (const auto& e : em) {
        if (e.IsValid()) CHECK(e.GetID() != b);
    }
}

void TestRemovedIdIsReusedWithNoLeftoverComponents() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Health{5});
    em.RemoveEntity(a);
    auto b = em.CreateEntity("b");
    CHECK(b == a);  // slot is recycled so the entity vector stays compact
    CHECK(em.IsValid(b));
    CHECK(!em.HasComponent<Health>(b));
}

void TestOutOfRangeIdIsNotValid() {
    TestManager em;
    CHECK(!em.IsValid(0));
    CHECK(!em.IsValid(TestManager::kNullComponent));
    CHECK(!em.HasComponent<Health>(42));
}

// -------------------------------------------------------------- components

void TestNewEntityHasNoComponents() {
    TestManager em;
    auto a = em.CreateEntity("a");
    CHECK(!em.HasComponent<Position>(a));
    CHECK(!em.HasComponent<Health>(a));
    CHECK(!em.HasComponent<Tag>(a));
}

void TestAddThenGetReturnsStoredValue() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Position{3, 7});
    CHECK(em.HasComponent<Position>(a));
    CHECK(em.GetComponent<Position>(a).x == 3);
    CHECK(em.GetComponent<Position>(a).y == 7);
}

void TestGetReturnsReferenceIntoStorage() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Health{10});
    em.GetComponent<Health>(a).hp -= 4;
    CHECK(em.GetComponent<Health>(a).hp == 6);
}

void TestComponentsBelongToTheirOwnEntity() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    em.AddComponent(a, Health{1});
    em.AddComponent(b, Health{2});
    em.AddComponent(b, Tag{'b'});
    CHECK(em.GetComponent<Health>(a).hp == 1);
    CHECK(em.GetComponent<Health>(b).hp == 2);
    CHECK(!em.HasComponent<Tag>(a));
    CHECK(em.HasComponent<Tag>(b));
}

void TestAddingTwiceReplacesValue() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Health{1});
    em.AddComponent(a, Health{9});
    CHECK(em.GetComponent<Health>(a).hp == 9);
    CHECK(CountValidSlots(em.GetComponentStorage<Health>()) == 1);
}

void TestRemoveComponentLeavesOthers() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Health{1});
    em.AddComponent(a, Tag{'x'});
    em.RemoveComponent<Health>(a);
    CHECK(!em.HasComponent<Health>(a));
    CHECK(em.HasComponent<Tag>(a));
    CHECK(em.GetComponent<Tag>(a).c == 'x');
}

void TestRemoveMissingComponentIsNoOp() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.RemoveComponent<Health>(a);
    CHECK(!em.HasComponent<Health>(a));
    CHECK(em.IsValid(a));
}

void TestRemoveEntityFreesItsComponents() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    em.AddComponent(a, Health{1});
    em.AddComponent(b, Health{2});
    em.RemoveEntity(a);
    CHECK(!em.HasComponent<Health>(a));
    CHECK(CountValidSlots(em.GetComponentStorage<Health>()) == 1);
    CHECK(em.GetComponent<Health>(b).hp == 2);
}

void TestFreedComponentSlotIsReused() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    em.AddComponent(a, Health{1});
    em.AddComponent(b, Health{2});
    em.RemoveComponent<Health>(a);
    auto c = em.CreateEntity("c");
    em.AddComponent(c, Health{3});
    CHECK(em.GetComponentStorage<Health>().mStorage.size() == 2);
    CHECK(em.GetComponent<Health>(b).hp == 2);
    CHECK(em.GetComponent<Health>(c).hp == 3);
}

void TestStorageRecordsOwnerOfEachSlot() {
    TestManager em;
    auto a = em.CreateEntity("a");
    auto b = em.CreateEntity("b");
    em.AddComponent(b, Health{20});
    em.AddComponent(a, Health{10});
    // A system can walk one storage and find each component's entity.
    auto& storage = em.GetComponentStorage<Health>();
    int sum = 0;
    for (size_t i = 0; i < storage.mStorage.size(); ++i) {
        if (!storage.mValid[i]) continue;
        auto owner = storage.mOwner[i];
        CHECK(em.GetComponent<Health>(owner).hp == storage.mStorage[i].hp);
        sum += storage.mStorage[i].hp;
    }
    CHECK(sum == 30);
}

void TestHasComponentWorksOnConstManager() {
    TestManager em;
    auto a = em.CreateEntity("a");
    em.AddComponent(a, Tag{'t'});
    const TestManager& view = em;
    CHECK(view.HasComponent<Tag>(a));
    CHECK(!view.HasComponent<Health>(a));
    CHECK(view.IsValid(a));
}

struct NamedTest {
    const char* name;
    void (*fn)();
};

}  // namespace

int main() {
    const NamedTest tests[] = {
        {"CreateGivesDistinctIds", TestCreateGivesDistinctIds},
        {"IterationVisitsCreatedEntities", TestIterationVisitsCreatedEntities},
        {"RemovedEntityIsSkippedInIteration", TestRemovedEntityIsSkippedInIteration},
        {"RemovedIdIsReusedWithNoLeftoverComponents", TestRemovedIdIsReusedWithNoLeftoverComponents},
        {"OutOfRangeIdIsNotValid", TestOutOfRangeIdIsNotValid},
        {"NewEntityHasNoComponents", TestNewEntityHasNoComponents},
        {"AddThenGetReturnsStoredValue", TestAddThenGetReturnsStoredValue},
        {"GetReturnsReferenceIntoStorage", TestGetReturnsReferenceIntoStorage},
        {"ComponentsBelongToTheirOwnEntity", TestComponentsBelongToTheirOwnEntity},
        {"AddingTwiceReplacesValue", TestAddingTwiceReplacesValue},
        {"RemoveComponentLeavesOthers", TestRemoveComponentLeavesOthers},
        {"RemoveMissingComponentIsNoOp", TestRemoveMissingComponentIsNoOp},
        {"RemoveEntityFreesItsComponents", TestRemoveEntityFreesItsComponents},
        {"FreedComponentSlotIsReused", TestFreedComponentSlotIsReused},
        {"StorageRecordsOwnerOfEachSlot", TestStorageRecordsOwnerOfEachSlot},
        {"HasComponentWorksOnConstManager", TestHasComponentWorksOnConstManager},
    };

    int failedTests = 0;
    for (const auto& test : tests) {
        int before = gFailures;
        test.fn();
        bool passed = gFailures == before;
        if (!passed) ++failedTests;
        std::printf("%s %s\n", passed ? "[PASS]" : "[FAIL]", test.name);
    }
    std::printf("\n%d/%zu tests passed (%d/%d checks)\n",
                static_cast<int>(std::size(tests)) - failedTests, std::size(tests),
                gChecks - gFailures, gChecks);
    return gFailures == 0 ? 0 : 1;
}
