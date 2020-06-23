#include "catch2/catch.hpp"
#include "entity/hierarchy.h"
#include "entt/entt.hpp"

using namespace erwin;

class HierarchyFixture00
{
protected:
    entt::registry registry;
};

TEST_CASE_METHOD(HierarchyFixture00, "Attaching one node to another", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    entity::attach(A, B, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == B);

    REQUIRE(B_h.parent == A);
    REQUIRE(B_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Attaching a subtree to a node", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    auto D = registry.create();
    entity::attach(B, C, registry);
    entity::attach(B, D, registry);
    entity::attach(A, B, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);
    const auto& C_h = registry.get<HierarchyComponent>(C);
    const auto& D_h = registry.get<HierarchyComponent>(D);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == B);

    REQUIRE(B_h.parent == A);
    REQUIRE(B_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == D);

    REQUIRE(C_h.parent == B);
    REQUIRE(C_h.previous_sibling == D);
    REQUIRE(C_h.next_sibling == k_invalid_entity_id);
    REQUIRE(C_h.first_child == k_invalid_entity_id);

    REQUIRE(D_h.parent == B);
    REQUIRE(D_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(D_h.next_sibling == C);
    REQUIRE(D_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Attaching one node to another with a child", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    entity::attach(A, B, registry);
    entity::attach(A, C, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);
    const auto& C_h = registry.get<HierarchyComponent>(C);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == C);

    REQUIRE(B_h.parent == A);
    REQUIRE(B_h.previous_sibling == C);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == k_invalid_entity_id);

    REQUIRE(C_h.parent == A);
    REQUIRE(C_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(C_h.next_sibling == B);
    REQUIRE(C_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Detaching first child", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    entity::attach(A, B, registry);
    entity::attach(A, C, registry);
    entity::detach(C, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);
    const auto& C_h = registry.get<HierarchyComponent>(C);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == B);

    REQUIRE(B_h.parent == A);
    REQUIRE(B_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == k_invalid_entity_id);

    REQUIRE(C_h.parent == k_invalid_entity_id);
    REQUIRE(C_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(C_h.next_sibling == k_invalid_entity_id);
    REQUIRE(C_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Detaching middle child", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    auto D = registry.create();
    entity::attach(A, B, registry);
    entity::attach(A, C, registry);
    entity::attach(A, D, registry);
    entity::detach(C, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);
    const auto& D_h = registry.get<HierarchyComponent>(D);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == D);

    REQUIRE(B_h.parent == A);
    REQUIRE(B_h.previous_sibling == D);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == k_invalid_entity_id);

    REQUIRE(D_h.parent == A);
    REQUIRE(D_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(D_h.next_sibling == B);
    REQUIRE(D_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Detaching last child", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    entity::attach(A, B, registry);
    entity::attach(A, C, registry);
    entity::detach(B, registry);

    const auto& A_h = registry.get<HierarchyComponent>(A);
    const auto& B_h = registry.get<HierarchyComponent>(B);
    const auto& C_h = registry.get<HierarchyComponent>(C);

    REQUIRE(A_h.parent == k_invalid_entity_id);
    REQUIRE(A_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(A_h.next_sibling == k_invalid_entity_id);
    REQUIRE(A_h.first_child == C);

    REQUIRE(B_h.parent == k_invalid_entity_id);
    REQUIRE(B_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(B_h.next_sibling == k_invalid_entity_id);
    REQUIRE(B_h.first_child == k_invalid_entity_id);

    REQUIRE(C_h.parent == A);
    REQUIRE(C_h.previous_sibling == k_invalid_entity_id);
    REQUIRE(C_h.next_sibling == k_invalid_entity_id);
    REQUIRE(C_h.first_child == k_invalid_entity_id);
}

TEST_CASE_METHOD(HierarchyFixture00, "Child check", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    entity::attach(A, B, registry);

    bool B_is_child_A = entity::is_child(A, B, registry);
    bool A_is_child_B = entity::is_child(B, A, registry);

    REQUIRE(B_is_child_A);
    REQUIRE(!A_is_child_B);
}

TEST_CASE_METHOD(HierarchyFixture00, "Child check, split trees", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    auto D = registry.create();
    entity::attach(A, B, registry);
    entity::attach(C, D, registry);

    bool D_is_child_A = entity::is_child(A, D, registry);
    bool B_is_child_C = entity::is_child(C, B, registry);

    REQUIRE(!D_is_child_A);
    REQUIRE(!B_is_child_C);
}

TEST_CASE_METHOD(HierarchyFixture00, "Sibling check", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    entity::attach(A, B, registry);
    entity::attach(A, C, registry);

    bool B_is_sibling_C = entity::is_sibling(B, C, registry);
    bool C_is_sibling_B = entity::is_sibling(C, B, registry);
    bool A_is_sibling_B = entity::is_sibling(A, B, registry);
    bool B_is_sibling_A = entity::is_sibling(B, A, registry);

    REQUIRE(B_is_sibling_C);
    REQUIRE(C_is_sibling_B);
    REQUIRE(!A_is_sibling_B);
    REQUIRE(!B_is_sibling_A);
}

TEST_CASE_METHOD(HierarchyFixture00, "Sibling check, split trees", "[hier]")
{
    auto A = registry.create();
    auto B = registry.create();
    auto C = registry.create();
    auto D = registry.create();
    entity::attach(A, B, registry);
    entity::attach(C, D, registry);

    bool C_is_sibling_D = entity::is_sibling(C, D, registry);
    bool D_is_sibling_C = entity::is_sibling(D, C, registry);

    REQUIRE(!C_is_sibling_D);
    REQUIRE(!D_is_sibling_C);
}

struct NameComponent
{
    NameComponent(const std::string& name_) : name(name_) {}
    std::string name;
};

inline std::string to_string(EntityID entity)
{
    return (entity != k_invalid_entity_id) ? std::to_string(size_t(entity)) : "NULL";
}

std::ostream& operator<<(std::ostream& stream, const HierarchyComponent& rhs)
{
    stream << "nc: " << rhs.children << " pa: " << to_string(rhs.parent) << " pr: " << to_string(rhs.previous_sibling)
           << " ne: " << to_string(rhs.next_sibling) << " fc: " << to_string(rhs.first_child);
    return stream;
}

void print_hierarchy(EntityID node, entt::registry& registry)
{
    entity::depth_first(node, registry, [&registry](EntityID ent, const auto& hier, size_t depth) {
        const auto& cname = registry.get<NameComponent>(ent);
        std::string indent(4 * depth, ' ');
        DLOGN("entity") << indent << cname.name << " [" << size_t(ent) << "]" << std::endl;
        DLOG("entity", 1) << indent << hier << std::endl;
        return false;
    });
}

class HierarchyFixture01
{
public:
    HierarchyFixture01()
    {
        A = registry.create();
        B = registry.create();
        C = registry.create();
        D = registry.create();
        E = registry.create();
        F = registry.create();

        registry.emplace<NameComponent>(A, "A");
        registry.emplace<NameComponent>(B, "B");
        registry.emplace<NameComponent>(C, "C");
        registry.emplace<NameComponent>(D, "D");
        registry.emplace<NameComponent>(E, "E");
        registry.emplace<NameComponent>(F, "F");

        entity::attach(A, B, registry);
        entity::attach(A, C, registry);
        entity::attach(A, D, registry);
        entity::attach(D, E, registry);
        entity::attach(D, F, registry);
    }

protected:
    entt::registry registry;
    EntityID A;
    EntityID B;
    EntityID C;
    EntityID D;
    EntityID E;
    EntityID F;
};

TEST_CASE_METHOD(HierarchyFixture01, "Base case requirements", "[hier]")
{
    // entity::sort_hierarchy(registry);
    // print_hierarchy(A, registry);

    REQUIRE(entity::is_child(A, B, registry));
    REQUIRE(entity::is_child(A, C, registry));
    REQUIRE(entity::is_child(A, D, registry));
    REQUIRE(entity::is_child(D, E, registry));
    REQUIRE(entity::is_child(D, F, registry));
    REQUIRE(!entity::is_child(A, E, registry));
    REQUIRE(!entity::is_child(A, F, registry));

    REQUIRE(entity::subtree_contains(A, D, registry));
    REQUIRE(entity::subtree_contains(A, E, registry));
    REQUIRE(entity::subtree_contains(A, F, registry));

    REQUIRE(entity::is_sibling(B, C, registry));
    REQUIRE(entity::is_sibling(C, D, registry));
    REQUIRE(entity::is_sibling(E, F, registry));
    REQUIRE(!entity::is_sibling(A, B, registry));
    REQUIRE(!entity::is_sibling(D, E, registry));
}

TEST_CASE_METHOD(HierarchyFixture01, "Detaching subtree", "[hier]")
{
    entity::detach(D, registry);

    REQUIRE(entity::is_child(A, B, registry));
    REQUIRE(entity::is_child(A, C, registry));
    REQUIRE(!entity::is_child(A, D, registry));
    REQUIRE(entity::is_child(D, E, registry));
    REQUIRE(entity::is_child(D, F, registry));

    REQUIRE(!entity::subtree_contains(A, D, registry));
    REQUIRE(!entity::subtree_contains(A, E, registry));
    REQUIRE(!entity::subtree_contains(A, F, registry));

    REQUIRE(!entity::is_sibling(C, D, registry));
}

TEST_CASE_METHOD(HierarchyFixture01, "Reattaching subtree", "[hier]")
{
    entity::attach(C, D, registry);

    REQUIRE(entity::is_child(A, B, registry));
    REQUIRE(entity::is_child(A, C, registry));
    REQUIRE(!entity::is_child(A, D, registry));
    REQUIRE(entity::is_child(C, D, registry));
    REQUIRE(entity::is_child(D, E, registry));
    REQUIRE(entity::is_child(D, F, registry));

    REQUIRE(entity::subtree_contains(A, D, registry));
    REQUIRE(entity::subtree_contains(A, E, registry));
    REQUIRE(entity::subtree_contains(A, F, registry));

    REQUIRE(!entity::is_sibling(C, D, registry));
}