// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include <string>
#include <memory>
#include "Role.hpp"
namespace coup {
class Player {
public:
    std::string name; std::unique_ptr<Role> role; int coins{0}; bool alive{true};
    bool sanctionActive{false}, jamActive{false};
    bool investedThisTurn{false}, didPeekThisTurn{false}, didJamThisTurn{false};
    bool forcedCoupFirstAction{false}, bribePendingCoup{false};
    int actions{0};
    Player* lastArrestTarget{nullptr};
    Player(std::string n, std::unique_ptr<Role> r): name(std::move(n)), role(std::move(r)) {}
    bool isGovernor() const; bool isJudge() const; bool isGeneral() const;
    bool isBaron() const;    bool isMerchant() const; bool isSpy() const;
}; }