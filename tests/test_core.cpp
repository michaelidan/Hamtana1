// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "Game.hpp"
#include "Player.hpp"
#include "roles/Judge.hpp"
#include "roles/General.hpp"
#include "roles/Merchant.hpp"
using namespace coup;

TEST_CASE("Bribe -> +1 action; coup first") {
    Game g;
    auto& m = g.addPlayer("M", std::make_unique<Merchant>());
    auto& j = g.addPlayer("J", std::make_unique<Judge>());
    auto& x = g.addPlayer("X", std::make_unique<General>());
    (void)j;(void)x;
    m.coins = 11;
    g.startTurn();
    CHECK(m.forcedCoupFirstAction == true);
    g.bribe();
    CHECK(m.actions == 2);
    CHECK(m.forcedCoupFirstAction == true);
    for (auto* t : g.alivePlayers()) if (t != &m) { g.coup(*t); break; }
    CHECK(m.forcedCoupFirstAction == false);
}

TEST_CASE("Judge blocks bribe; turn continues; coup still required") {
    Game g;
    auto& m = g.addPlayer("M", std::make_unique<Merchant>());
    auto& j = g.addPlayer("J", std::make_unique<Judge>());
    auto& x = g.addPlayer("X", std::make_unique<General>());
    (void)x;
    m.coins = 11;
    g.startTurn();
    g.queueBlock("J", Action::Bribe);
    g.bribe();
    CHECK(m.actions == 1);
    for (auto* t : g.alivePlayers()) if (t != &m) { g.coup(*t); break; }
    CHECK(m.forcedCoupFirstAction == false);
}

TEST_CASE("General needs 5 to block") {
    Game g;
    auto& a = g.addPlayer("A", std::make_unique<Merchant>());
    auto& g1 = g.addPlayer("G1", std::make_unique<General>());
    auto& g2 = g.addPlayer("G2", std::make_unique<General>());
    a.coins = 10; g1.coins = 4; g2.coins = 5;
    g.startTurn();
    g.queueBlock("G1", Action::Coup);
    g.coup(g1);
    CHECK(g1.alive == false);
}
