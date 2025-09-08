// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include "types.hpp"
#include "AuthorityLists.hpp"
#include "log/EventLog.hpp"
#include "errors.hpp"
#include <optional>
#include <unordered_map>
namespace coup {
class Game {
public:
    std::vector<std::unique_ptr<Player>> players_;
    size_t turnIndex{0}, roundAnchorIndex{0};
    bool inRound{false}; int skipRoundStreak{0}; bool draw{false};
    std::optional<std::string> singleWinner;
    std::optional<std::string> queuedBlockerName; std::optional<Action> queuedBlockerAction;
    AuthorityLists authorities; EventLog log;
    std::unordered_map<Player*, bool> skippedThisRound;
    Game() = default;
    std::string turn() const;
    std::vector<std::string> players() const;
    std::string winner() const;
    bool isDraw() const { return draw; }
    std::vector<std::string> winners() const;
    Player& addPlayer(const std::string& name, std::unique_ptr<Role> r);
    Player& current(); const Player& current() const;
    size_t aliveCount() const; std::vector<Player*> alivePlayers();
    void startTurn(); void endTurn(); void skip();
    void queueBlock(const std::string& n, Action a){ queuedBlockerName=n; queuedBlockerAction=a; }
    void gather(); void tax(); void bribe(); void arrest(Player& t);
    void sanction(Player& t); void coup(Player& t); void invest();
    void peek(Player& t); void jamArrest(Player& t);
    void rebuildAuthorities(); Player* findPlayerByName(const std::string& n);
    std::vector<Player*> authorityListFor(Action a);
private:
    bool tryBlock(Action a);
    void clearStatusesAtTurnStart(Player& p);
    void checkForcedCoupBefore(Action a);
    void markSkipped(Player& p);
}; }