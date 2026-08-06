/*
 * Copyright 2026 Sukanle(https://github.com/Sukanle)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// =============================================================================
// Reflect Library — Catch2 Test Suite
// =============================================================================
// Covers:
//   - Static class reflection (fields, methods, custom template names)
//   - Static enum reflection (scoped & unscoped)
//   - Dynamic class reflection (SKL_RFD_CLASS / SKL_RFD_PROPERTY / SKL_RFD_METHOD)
//   - Dynamic enum reflection (SKL_RFD_ENUM_BEGIN / SKL_RFD_ENUM_VALUE)
// =============================================================================

#include "reflect.h"

#include <cstdio>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

// =============================================================================
// Test Types
// =============================================================================

class Player {
public:
    std::string name{"Unknown"};
    int health{100};
    int mana{50};

    void heal(int amount) {
        health += amount;
        std::printf("    [Player::heal] +%d health -> %d\n", amount, health);
    }

    [[nodiscard]] bool isAlive() const { return health > 0; }

    void takeDamage(int amount) {
        health -= amount;
        if (health < 0) health = 0;
        std::printf("    [Player::takeDamage] -%d health -> %d\n", amount, health);
    }
};

class Weapon {
public:
    std::string name{"Fists"};
    int damage{5};
    float weight{1.0f};
};

enum Fruit : uint8_t {
    APPLE,
    BANANA,
    ORANGE,
    GRAPE,
};

enum class Direction : uint8_t {
    NORTH = 0,
    SOUTH = 1,
    EAST = 2,
    WEST = 3,
};

// =============================================================================
// Register type hashes for dynamic reflection (REQUIRED by static_assert)
// =============================================================================

STATIC_TYPE_TAG(Player, "Player");
STATIC_TYPE_TAG(Weapon, "Weapon");
STATIC_TYPE_TAG(Fruit, "Fruit");
STATIC_TYPE_TAG(Direction, "Direction");

// =============================================================================
// Custom template names for SKL_RFS_PROPERTY's optional second parameter
// =============================================================================

constexpr const char _TMPL_health[]{"Health"};
constexpr const char _TMPL_mana[]{"Mana"};
constexpr const char _TMPL_isAlive[]{"IsAlive"};

// =============================================================================
// Static Reflection Registration
// =============================================================================

// --- Player: mix of default-name and custom-name properties ---
SKL_RFS_REGISTER_BEGIN(Player)
SKL_RFS_PROPERTY(name)                     // default template name
SKL_RFS_PROPERTY(health, _TMPL_health)     // custom template name
SKL_RFS_PROPERTY(mana, _TMPL_mana)         // custom template name
SKL_RFS_PROPERTY(heal)                     // member function
SKL_RFS_PROPERTY(isAlive, _TMPL_isAlive)   // const member function, custom name
SKL_RFS_PROPERTY(takeDamage)               // member function
SKL_RFS_REGISTER_END()

// --- Weapon: all default names ---
SKL_RFS_CLASS(Weapon)
SKL_RFS_PROPERTY(name)
SKL_RFS_PROPERTY(damage)
SKL_RFS_PROPERTY(weight)
SKL_RFS_CLASS()

// --- Fruit: unscoped enum ---
SKL_RFS_ENUM_BEGIN(Fruit)
SKL_RFS_ENUM_VALUE(Fruit::APPLE, "APPLE")
SKL_RFS_ENUM_VALUE(Fruit::BANANA, "BANANA")
SKL_RFS_ENUM_VALUE(Fruit::ORANGE, "ORANGE")
SKL_RFS_ENUM_VALUE(Fruit::GRAPE, "GRAPE")
SKL_RFS_ENUM_END()

// --- Direction: scoped enum ---
SKL_RFS_ENUM_BEGIN(Direction)
SKL_RFS_ENUM_VALUE(Direction::NORTH, "NORTH")
SKL_RFS_ENUM_VALUE(Direction::SOUTH, "SOUTH")
SKL_RFS_ENUM_VALUE(Direction::EAST, "EAST")
SKL_RFS_ENUM_VALUE(Direction::WEST, "WEST")
SKL_RFS_ENUM_END()

// =============================================================================
// Dynamic Reflection Registration
// =============================================================================

// use normal registration macros (BEGIN/END)
SKL_RFD_REGISTER_BEGIN(Player)
SKL_RFD_PROPERTY(name)
SKL_RFD_PROPERTY(health)
SKL_RFD_PROPERTY(mana)
SKL_RFD_METHOD(heal)
SKL_RFD_METHOD(isAlive)
SKL_RFD_METHOD(takeDamage)
SKL_RFD_REGISTER_END()

// use simple registration macros (CLASS)
SKL_RFD_CLASS(Weapon)
SKL_RFD_PROPERTY(name)
SKL_RFD_PROPERTY(damage)
SKL_RFD_PROPERTY(weight)
SKL_RFD_CLASS()

// use normal registration macros (BEGIN/END)
SKL_RFD_ENUM_BEGIN(Fruit)
SKL_RFD_ENUM_VALUE(Fruit::APPLE, "APPLE")
SKL_RFD_ENUM_VALUE(Fruit::BANANA, "BANANA")
SKL_RFD_ENUM_VALUE(Fruit::ORANGE, "ORANGE")
SKL_RFD_ENUM_VALUE(Fruit::GRAPE, "GRAPE")
SKL_RFD_ENUM_END()

// use simple registration macros (ENUM)
SKL_RFD_ENUM(Direction)
SKL_RFD_ENUM_VALUE(Direction::NORTH, "NORTH")
SKL_RFD_ENUM_VALUE(Direction::SOUTH, "SOUTH")
SKL_RFD_ENUM_VALUE(Direction::EAST, "EAST")
SKL_RFD_ENUM_VALUE(Direction::WEST, "WEST")
SKL_RFD_ENUM()

// =============================================================================
// Static Class Reflection Tests
// =============================================================================

TEST_CASE("Static class reflection — Player", "[static][class]") {
    using Info = SRefl::TypeInfo<Player>;
    Player player;

    SECTION("type metadata") {
        REQUIRE(Info::_name == "Player [class]");
        INFO("Type name: " << Info::_name);
    }

    SECTION("field: name (default template name)") {
        REQUIRE(Info::Registry::_name.getName() == "name");
        INFO("Field 'name' = " << (player.*Info::Registry::_name._ptr));

        player.*Info::Registry::_name._ptr = "Arthur";
        REQUIRE((player.*Info::Registry::_name._ptr) == "Arthur");
    }

    SECTION("field: health (custom template name)") {
        REQUIRE(Info::Registry::_health.getName() == "health");
        REQUIRE(Info::Registry::_health.from_TempName() == "Health");
        REQUIRE((player.*Info::Registry::_health._ptr) == 100);
        INFO("Field 'health' (template: '"
             << Info::Registry::_health.from_TempName()
             << "') = "
             << (player.*Info::Registry::_health._ptr));
    }

    SECTION("field: mana (custom template name)") {
        REQUIRE(Info::Registry::_mana.getName() == "mana");
        REQUIRE(Info::Registry::_mana.from_TempName() == "Mana");
        REQUIRE((player.*Info::Registry::_mana._ptr) == 50);
    }

    SECTION("method: heal") {
        REQUIRE(Info::Registry::_heal.is_function());
        REQUIRE(Info::Registry::_heal.is_member());
        REQUIRE(Info::Registry::_heal.getName() == "heal");

        (player.*Info::Registry::_heal._ptr)(20);
        REQUIRE((player.*Info::Registry::_health._ptr) == 120);
    }

    SECTION("method: isAlive (const)") {
        REQUIRE(Info::Registry::_isAlive.is_function());
        REQUIRE(Info::Registry::_isAlive.is_const());
        REQUIRE(Info::Registry::_isAlive.getName() == "isAlive");
        REQUIRE(Info::Registry::_isAlive.from_TempName() == "IsAlive");

        REQUIRE((player.*Info::Registry::_isAlive._ptr)() == true);
        INFO("isAlive() = " << std::boolalpha << (player.*Info::Registry::_isAlive._ptr)());
    }

    SECTION("method: takeDamage") {
        REQUIRE(Info::Registry::_takeDamage.is_function());
        REQUIRE(Info::Registry::_takeDamage.getName() == "takeDamage");

        (player.*Info::Registry::_takeDamage._ptr)(200);
        REQUIRE((player.*Info::Registry::_health._ptr) == 0);
        REQUIRE((player.*Info::Registry::_isAlive._ptr)() == false);
    }
}

TEST_CASE("Static class reflection — Weapon", "[static][class]") {
    using Info = SRefl::TypeInfo<Weapon>;
    Weapon weapon;

    SECTION("type metadata") { REQUIRE(Info::_name == "Weapon [class]"); }

    SECTION("field names") {
        REQUIRE(Info::Registry::_name.getName() == "name");
        REQUIRE(Info::Registry::_damage.getName() == "damage");
        REQUIRE(Info::Registry::_weight.getName() == "weight");
    }

    SECTION("field defaults") {
        REQUIRE((weapon.*Info::Registry::_damage._ptr) == 5);
        REQUIRE((weapon.*Info::Registry::_weight._ptr) == 1.0f);
    }

    SECTION("field read/write") {
        weapon.*Info::Registry::_name._ptr = "Excalibur";
        weapon.*Info::Registry::_damage._ptr = 99;
        weapon.*Info::Registry::_weight._ptr = 3.5f;

        REQUIRE((weapon.*Info::Registry::_name._ptr) == "Excalibur");
        REQUIRE((weapon.*Info::Registry::_damage._ptr) == 99);
        REQUIRE((weapon.*Info::Registry::_weight._ptr) == 3.5f);

        INFO("Weapon: "
             << (weapon.*Info::Registry::_name._ptr)
             << ", damage="
             << (weapon.*Info::Registry::_damage._ptr)
             << ", weight="
             << (weapon.*Info::Registry::_weight._ptr));
    }
}

// =============================================================================
// Static Enum Reflection Tests
// =============================================================================

TEST_CASE("Static enum reflection — Fruit (unscoped)", "[static][enum]") {
    using Info = SRefl::TypeInfo<Fruit>;

    SECTION("type metadata") {
        REQUIRE(Info::_name == "Fruit [enum]");
        REQUIRE(Info::is_scoped == false);
    }

    SECTION("enum values") {
        REQUIRE(std::size(Info::list) == 4u);

        for (const auto &[value, name] : Info::list) {
            REQUIRE(name.size() > 0u);
            INFO("  " << name << " = " << static_cast<int>(value));
        }
    }
}

TEST_CASE("Static enum reflection — Direction (scoped)", "[static][enum]") {
    using Info = SRefl::TypeInfo<Direction>;

    SECTION("type metadata") {
        REQUIRE(Info::_name == "Direction [enum class]");
        REQUIRE(Info::is_scoped == true);
    }

    SECTION("enum values") {
        REQUIRE(std::size(Info::list) == 4u);

        for (const auto &[value, name] : Info::list) {
            INFO("  " << name << " = " << static_cast<int>(value));
        }
    }
}

// =============================================================================
// Dynamic Class Reflection Tests
// =============================================================================

TEST_CASE("Dynamic class reflection — Player", "[dynamic][class]") {
    auto *ti = DRefl::Registry::instance().find_by_name("Player");
    REQUIRE(ti != nullptr);

    SECTION("type metadata") {
        REQUIRE(ti->kind == DRefl::Kind::Class);
        REQUIRE(ti->size == sizeof(Player));
        INFO("Type: " << ti->name << ", size=" << ti->size);
    }

    SECTION("find fields") {
        auto *f_name = ti->find_field("name");
        auto *f_health = ti->find_field("health");
        auto *f_mana = ti->find_field("mana");

        REQUIRE(f_name != nullptr);
        REQUIRE(f_health != nullptr);
        REQUIRE(f_mana != nullptr);

        Player player;
        player.name = "Merlin";
        player.health = 80;
        player.mana = 200;

        auto *name_ptr = static_cast<std::string *>(f_name->getter(&player));
        auto *health_ptr = static_cast<int *>(f_health->getter(&player));
        auto *mana_ptr = static_cast<int *>(f_mana->getter(&player));

        REQUIRE(*name_ptr == "Merlin");
        REQUIRE(*health_ptr == 80);
        REQUIRE(*mana_ptr == 200);

        INFO("Fields: name=" << *name_ptr << ", health=" << *health_ptr << ", mana=" << *mana_ptr);
    }

    SECTION("set field via dynamic setter") {
        Player player;
        auto *f_health = ti->find_field("health");
        REQUIRE(f_health != nullptr);

        int new_health = 999;
        f_health->setter(&player, &new_health);
        REQUIRE(player.health == 999);
    }

    SECTION("find methods") {
        auto *m_heal = ti->find_method("heal");
        auto *m_isAlive = ti->find_method("isAlive");
        auto *m_takeDamage = ti->find_method("takeDamage");

        REQUIRE(m_heal != nullptr);
        REQUIRE(m_isAlive != nullptr);
        REQUIRE(m_takeDamage != nullptr);
    }
}

TEST_CASE("Dynamic class reflection — Weapon", "[dynamic][class]") {
    auto *ti = DRefl::Registry::instance().find_by_name("Weapon");
    REQUIRE(ti != nullptr);

    SECTION("type metadata") {
        REQUIRE(ti->kind == DRefl::Kind::Class);
        REQUIRE(ti->fields.size() == 3u);
    }

    SECTION("field getter") {
        Weapon weapon;
        weapon.name = "Longbow";
        weapon.damage = 25;
        weapon.weight = 2.5f;

        auto *f_name = ti->find_field("name");
        auto *f_damage = ti->find_field("damage");
        auto *f_weight = ti->find_field("weight");

        REQUIRE(f_name != nullptr);
        REQUIRE(f_damage != nullptr);
        REQUIRE(f_weight != nullptr);

        REQUIRE(*static_cast<std::string *>(f_name->getter(&weapon)) == "Longbow");
        REQUIRE(*static_cast<int *>(f_damage->getter(&weapon)) == 25);
        REQUIRE(*static_cast<float *>(f_weight->getter(&weapon)) == 2.5f);

        INFO("Weapon: "
             << *static_cast<std::string *>(f_name->getter(&weapon))
             << ", damage="
             << *static_cast<int *>(f_damage->getter(&weapon))
             << ", weight="
             << *static_cast<float *>(f_weight->getter(&weapon)));
    }
}

// =============================================================================
// Dynamic Enum Reflection Tests
// =============================================================================

TEST_CASE("Dynamic enum reflection — Fruit (unscoped)", "[dynamic][enum]") {
    auto *ti = DRefl::Registry::instance().find_by_name("Fruit");
    REQUIRE(ti != nullptr);

    SECTION("type metadata") {
        REQUIRE(ti->kind == DRefl::Kind::Enum);
        REQUIRE(ti->enum_info != nullptr);
        REQUIRE(ti->enum_info->is_scoped == false);
    }

    SECTION("enum entries") {
        REQUIRE(ti->enum_info->entries.size() == 4u);

        for (const auto &entry : ti->enum_info->entries) {
            INFO("  " << entry.name << " = " << entry.value);
        }
    }
}

TEST_CASE("Dynamic enum reflection — Direction (scoped)", "[dynamic][enum]") {
    auto *ti = DRefl::Registry::instance().find_by_name("Direction");
    REQUIRE(ti != nullptr);

    SECTION("type metadata") {
        REQUIRE(ti->kind == DRefl::Kind::Enum);
        REQUIRE(ti->enum_info != nullptr);
        REQUIRE(ti->enum_info->is_scoped == true);
    }

    SECTION("enum entries") {
        REQUIRE(ti->enum_info->entries.size() == 4u);

        for (const auto &entry : ti->enum_info->entries) {
            INFO("  " << entry.name << " = " << entry.value);
        }
    }
}

// =============================================================================
// Registry Tests
// =============================================================================

TEST_CASE("Dynamic registry — lookup", "[dynamic][registry]") {
    auto &reg = DRefl::Registry::instance();

    SECTION("find_by_name") {
        REQUIRE(reg.find_by_name("Player") != nullptr);
        REQUIRE(reg.find_by_name("Weapon") != nullptr);
        REQUIRE(reg.find_by_name("Fruit") != nullptr);
        REQUIRE(reg.find_by_name("Direction") != nullptr);
        REQUIRE(reg.find_by_name("NonExistent") == nullptr);
    }

    SECTION("type_count") { REQUIRE(reg.type_count() >= 4u); }

    SECTION("type_at iteration") {
        size_t count = reg.type_count();
        INFO("Registered types: " << count);

        for (size_t i = 0; i < count; ++i) {
            auto *ti = reg.type_at(i);
            REQUIRE(ti != nullptr);
            REQUIRE(ti->name != nullptr);
            INFO("  [" << i << "] " << ti->name);
        }
    }
}

// =============================================================================
// Integration: Static + Dynamic consistency
// =============================================================================

TEST_CASE("Integration — static & dynamic reflection consistency", "[integration]") {
    auto *ti = DRefl::Registry::instance().find_by_name("Player");
    REQUIRE(ti != nullptr);

    using Info = SRefl::TypeInfo<Player>;

    SECTION("field count matches") {
        // Static: check individual fields are accessible
        REQUIRE(Info::Registry::_name.getName() == "name");
        REQUIRE(Info::Registry::_health.getName() == "health");
        REQUIRE(Info::Registry::_mana.getName() == "mana");

        // Dynamic: same fields found
        REQUIRE(ti->find_field("name") != nullptr);
        REQUIRE(ti->find_field("health") != nullptr);
        REQUIRE(ti->find_field("mana") != nullptr);
    }

    SECTION("method count matches") {
        REQUIRE(Info::Registry::_heal.getName() == "heal");
        REQUIRE(Info::Registry::_isAlive.getName() == "isAlive");
        REQUIRE(Info::Registry::_takeDamage.getName() == "takeDamage");

        REQUIRE(ti->find_method("heal") != nullptr);
        REQUIRE(ti->find_method("isAlive") != nullptr);
        REQUIRE(ti->find_method("takeDamage") != nullptr);
    }

    SECTION("type name consistency") {
        REQUIRE(Info::_name == "Player [class]");
        REQUIRE(URefl::string_view(ti->name) == "Player");
    }
}