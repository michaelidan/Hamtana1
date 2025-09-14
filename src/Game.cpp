// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#include "Game.hpp"
#include "Player.hpp"
#include "roles/Governor.hpp"
#include "roles/Judge.hpp"
#include "roles/General.hpp"
#include "roles/Baron.hpp"
#include "roles/Merchant.hpp"
#include "roles/Spy.hpp"
#include <algorithm>
#include <sstream>
namespace coup
{
    std::string Game::turn() const { return players_.at(turnIndex)->name; }
    std::vector<std::string> Game::players() const
    {
        std::vector<std::string> v;
        for (auto &p : players_)
            if (p->alive)
                v.push_back(p->name);
        return v;
    }
    std::string Game::winner() const
    {
        if (draw)
            throw GameError(err::draw_no_single_winner());
        if (!singleWinner)
            throw GameError(err::winner_not_ready());
        return *singleWinner;
    }
    std::vector<std::string> Game::winners() const
    {
        if (!draw)
        {
            if (singleWinner)
                return {*singleWinner};
            return {};
        }
        std::vector<std::string> v;
        for (auto &p : players_)
            if (p->alive)
                v.push_back(p->name);
        return v;
    }
    Player &Game::addPlayer(const std::string &n, std::unique_ptr<Role> r)
    {
        players_.emplace_back(std::make_unique<Player>(n, std::move(r)));
        rebuildAuthorities();
        return *players_.back();
    }
    Player &Game::current() { return *players_.at(turnIndex); }
    const Player &Game::current() const { return *players_.at(turnIndex); }
    size_t Game::aliveCount() const
    {
        size_t c = 0;
        for (auto &p : players_)
            if (p->alive)
                ++c;
        return c;
    }
    std::vector<Player *> Game::alivePlayers()
    {
        std::vector<Player *> v;
        for (auto &p : players_)
            if (p->alive)
                v.push_back(p.get());
        return v;
    }
    void Game::rebuildAuthorities()
    {
        authorities.clear();
        for (auto &up : players_)
        {
            if (!up->alive)
                continue;
            if (up->isGovernor())
                authorities.governors.push_back(up.get());
            if (up->isJudge())
                authorities.judges.push_back(up.get());
            if (up->isGeneral())
                authorities.generals.push_back(up.get());
        }
    }
    Player *Game::findPlayerByName(const std::string &n)
    {
        for (auto &p : players_)
            if (p->name == n)
                return p.get();
        return nullptr;
    }
    std::vector<Player *> Game::authorityListFor(Action a)
    {
        switch (a)
        {
        case Action::Tax:
            return authorities.governors;
        case Action::Bribe:
            return authorities.judges;
        case Action::Coup:
            return authorities.generals;
        default:
            return {};
        }
    }
    void Game::clearStatusesAtTurnStart(Player& P) {
        bool hadSanction = P.sanctionActive;
        bool hadJam      = P.jamActive;
        P.sanctionActive = false;
        P.jamActive      = false;
        if (hadSanction) log.add("STATUS: sanction expired for '" + P.name + "'");
        if (hadJam)      log.add("STATUS: jam expired for '" + P.name + "'");
    }

    void Game::startTurn()
    {
        if (aliveCount() <= 1)
        {
            for (auto &p : players_)
                if (p->alive)
                    singleWinner = p->name;
            return;
        }
        Player &P = current();
        if (!inRound)
        {
            inRound = true;
            roundAnchorIndex = turnIndex;
        }

        bool mustCoup = (P.coins >= 10);
        if (P.isMerchant() && P.coins >= 3)
            P.coins += 1;
        P.actions = 1;
        P.investedThisTurn = P.didPeekThisTurn = P.didJamThisTurn = false;
        P.forcedCoupFirstAction = mustCoup;
        P.bribePendingCoup = false;
        std::ostringstream ss;
        ss << "START_TURN: '" << P.name << "' coins=" << P.coins << " forcedCoup=" << (P.forcedCoupFirstAction ? "true" : "false");
        log.add(ss.str());
    }
    void Game::endTurn()
    {
        Player& P = current();
        clearStatusesAtTurnStart(P);   // פה האיפוס — בסוף התור של בעל התור

        if (draw || singleWinner)
            return;
        size_t n = players_.size();
        do
        {
            turnIndex = (turnIndex + 1) % n;
        } while (!players_.at(turnIndex)->alive);
        if (turnIndex == roundAnchorIndex)
        {
            bool allSkipped = true;
            for (auto &p : players_)
                if (p->alive)
                {
                    if (!skippedThisRound[p.get()])
                    {
                        allSkipped = false;
                        break;
                    }
                }
            if (allSkipped)
                ++skipRoundStreak;
            else
                skipRoundStreak = 0;
            skippedThisRound.clear();
            if (skipRoundStreak >= 2)
            {
                draw = true;
                log.add("END_GAME: DRAW declared (two full rounds of all-skip).");
                return;
            }
            roundAnchorIndex = turnIndex;
        }
    }
    void Game::markSkipped(Player &p) { skippedThisRound[&p] = true; }
    void Game::checkForcedCoupBefore(Action a)
    {
        Player &P = current();
        if (P.forcedCoupFirstAction && a != Action::Coup)
            throw GameError("הפעולה הראשונה חייבת להיות coup");
    }
    bool Game::tryBlock(Action a)
    {
        if (!queuedBlockerName || !queuedBlockerAction || *queuedBlockerAction != a)
            return false;
        Player *blocker = findPlayerByName(*queuedBlockerName);
        if (!blocker || !blocker->alive)
        {
            queuedBlockerName.reset();
            queuedBlockerAction.reset();
            return false;
        }
        auto list = authorityListFor(a);
        bool found = false;
        for (auto *cand : list)
        {
            if (cand == blocker)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            queuedBlockerName.reset();
            queuedBlockerAction.reset();
            return false;
        }
        if (a == Action::Coup)
        {
            if (!blocker->isGeneral() || blocker->coins < 5)
            {
                queuedBlockerName.reset();
                queuedBlockerAction.reset();
                return false;
            }
            blocker->coins -= 5;
            log.add("BLOCK: General '" + blocker->name + "' blocked coup (-5).");
            queuedBlockerName.reset();
            queuedBlockerAction.reset();
            return true;
        }
        if (a == Action::Bribe)
        {
            if (!blocker->isJudge())
            {
                queuedBlockerName.reset();
                queuedBlockerAction.reset();
                return false;
            }
            log.add("BLOCK: Judge '" + blocker->name + "' blocked bribe.");
            queuedBlockerName.reset();
            queuedBlockerAction.reset();
            return true;
        }
        if (a == Action::Tax)
        {
            if (!blocker->isGovernor())
            {
                queuedBlockerName.reset();
                queuedBlockerAction.reset();
                return false;
            }
            log.add("BLOCK: Governor '" + blocker->name + "' blocked tax.");
            queuedBlockerName.reset();
            queuedBlockerAction.reset();
            return true;
        }
        return false;
    }
    void Game::gather() {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        if (p.sanctionActive) throw GameError("sanctioned: cannot gather/tax this turn");
        if (p.actions <= 0) throw GameError("no actions left");

        p.coins += 1;
        p.actions -= 1;
        log.add("ACTION: gather +1 by '" + p.name + "' (coins=" + std::to_string(p.coins) + ")");
    }

    void Game::tax() {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        if (p.sanctionActive) throw GameError("sanctioned: cannot gather/tax this turn");
        if (p.actions <= 0) throw GameError("no actions left");

        int gain = 2;
        if (p.isGovernor()) gain = 3;

        // נסיון חסימה ע"י Governor אחר
        if (tryBlock(Action::Tax)) {
            log.add("BLOCK: tax was blocked by Governor");
            p.actions -= 1; // הפעולה נצרכה גם אם נחסמה (בהתאם למסמך שלנו)
            return;
        }

        p.coins += gain;
        p.actions -= 1;
        log.add("ACTION: tax +" + std::to_string(gain) + " by '" + p.name + "' (coins=" + std::to_string(p.coins) + ")");
    }

    void Game::bribe() {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        if (p.actions <= 0) throw GameError("no actions left");
        if (p.coins < 4)   throw GameError("not enough coins for bribe");

        // אם כבר מחויבים לפתוח ב־coup (למשל כי התחלת תור עם 10+),
        // מותר לשחד רק אם יש 11+ כדי שתוכל מיד לבצע coup אחרי התשלום.
        if (p.forcedCoupFirstAction && p.coins < 11)
            throw GameError("bribe forbidden now: coup is mandatory");

        // תשלום
        p.coins -= 4;

        // חסימה ע״י Judge
        if (tryBlock(Action::Bribe)) {
            p.actions -= 1;  // הפעולה נצרכה גם אם נחסמה
            log.add("BLOCK: bribe was blocked by Judge; coins lost");
            return;
        }

        // נטו +1 פעולה
        p.actions -= 1;
        p.actions += 2;

        // מכאן ועד שיהיו 7+ — מותר לאסוף/למסות; ברגע שיש 7+ הפעולה הבאה חייבת להיות coup.
        p.forcedCoupFirstAction = true;
        p.bribePendingCoup      = true;

        log.add("ACTION: bribe by '" + p.name + "' -> +1 net action; next must be coup");
    }




    void Game::arrest(Player &T)
    {
        Player &P = current();
        checkForcedCoupBefore(Action::Arrest);
        if (P.actions <= 0)
            throw GameError("אין פעולות זמינות");
        if (!T.alive)
            throw GameError(err::target_eliminated());
        if (&T == &P)
            throw GameError("לא ניתן לעצור את עצמך");
        if (P.lastArrestTarget == &T)
            throw GameError(err::arrest_streak());
        if (T.coins <= 0)
            throw GameError(err::arrest_zero());
        if (T.jamActive)
            throw GameError(err::arrest_jam());
        if (T.isMerchant())
        {
            int pay = std::min(2, T.coins);
            T.coins -= pay;
        }
        else if (T.isGeneral())
        { /* net zero */
        }
        else
        {
            T.coins -= 1;
            P.coins += 1;
        }
        P.lastArrestTarget = &T;
        P.actions -= 1;
        log.add("ACTION: arrest by '" + P.name + "' on '" + T.name + "'");
    }
    void Game::sanction(Player &T)
    {
        Player &P = current();
        checkForcedCoupBefore(Action::Sanction);
        if (P.actions <= 0)
            throw GameError("אין פעולות זמינות");
        if (!T.alive)
            throw GameError(err::target_eliminated());
        if (&T == &P)
            throw GameError("לא ניתן להטיל Sanction על עצמך");
        int cost = 3 + (T.isJudge() ? 1 : 0);
        if (P.coins < cost)
            throw GameError(err::not_enough_coins("sanction"));
        P.coins -= cost;
        T.sanctionActive = true;
        if (T.isBaron())
            T.coins += 1;
        P.actions -= 1;
        log.add("STATUS: sanction applied on '" + T.name + "' by '" + P.name + "'");
    }
void Game::coup(Player& T) {
    Player& P = current();

    checkForcedCoupBefore(Action::Coup);

    if (P.actions <= 0)                 throw GameError("no actions left");
    if (!T.alive)                       throw GameError(err::target_eliminated());
    if (&T == &P)                       throw GameError(err::self_coup());
    if (P.coins < 7)                    throw GameError(err::not_enough_coins("coup"));

    // תשלום על ההפיכה
    P.coins -= 7;

    // ניסיון חסימה (גנרל משלם 5 – ההיגיון צריך להיות בתוך tryBlock)
    if (tryBlock(Action::Coup)) {
        // הפעולה נצרכה; מנקים חובת Coup; נשארים בתור אם יש פעולות
        P.actions -= 1;
        P.forcedCoupFirstAction = false;
        P.bribePendingCoup      = false;
        log.add("ACTION: coup blocked; '" + T.name + "' survives");
        log.add("ACTION: coup resolved; actions left = " + std::to_string(P.actions));
        return;
    }

    // לא נחסם – היעד מודח
    T.alive = false;
    rebuildAuthorities();

    // הפעולה נצרכה; מנקים חובת Coup; נשארים בתור אם יש פעולות
    P.actions -= 1;
    P.forcedCoupFirstAction = false;
    P.bribePendingCoup      = false;
    log.add("ACTION: coup succeeded; '" + T.name + "' eliminated");
    log.add("ACTION: coup resolved; actions left = " + std::to_string(P.actions));

    // בדיקת סיום משחק
    if (aliveCount() == 1) {
        for (auto& p : players_) if (p->alive) singleWinner = p->name;
        log.add("END_GAME: single winner '" + *singleWinner + "'");
    }
}

    void Game::invest()
    {
        Player &P = current();
        checkForcedCoupBefore(Action::Invest);
        if (P.actions <= 0)
            throw GameError("אין פעולות זמינות");
        if (!P.isBaron())
            throw GameError("רק Baron יכול להשקיע");
        if (P.investedThisTurn)
            throw GameError("אי אפשר להשקיע פעמיים באותו תור");
        if (P.coins < 3)
            throw GameError(err::not_enough_coins("invest"));
        P.coins -= 3;
        P.coins += 6;
        P.investedThisTurn = true;
        P.actions -= 1;
        log.add("ACTION: invest by '" + P.name + "'");
    }
void Game::peek(Player& t) {
    auto& p = current();
    if (!p.alive) throw GameError("cannot act: player is eliminated");
    if (!p.isSpy()) throw GameError("only Spy can peek");
    if (p.didPeekThisTurn) throw GameError("peek already used this turn");
    if (!t.alive) throw GameError("target is eliminated");

    // הפעולה חינמית ולא צורכת תור
    p.didPeekThisTurn = true;
    log.add("SPY: '" + p.name + "' peeked '" + t.name + "' -> coins=" + std::to_string(t.coins));
}

void Game::jamArrest(Player& t) {
    auto& p = current();
    if (!p.alive) throw GameError("cannot act: player is eliminated");
    if (!p.isSpy()) throw GameError("only Spy can jam arrest");
    if (p.didJamThisTurn) throw GameError("jam already used this turn");
    if (!t.alive) throw GameError("target is eliminated");

    // האפקט תקף רק לתור הבא של היעד
    t.jamActive    = true;
    p.didJamThisTurn = true;
    log.add("SPY: '" + p.name + "' jam-arrest on '" + t.name + "' (next turn only)");
}

    void Game::skip()
    {
        Player &P = current();
        // אסור לדלג רק אם ממש חייבים לבצע Coup עכשיו (יש 7+ בזמן חובה)
        if (P.forcedCoupFirstAction && P.coins >= 7) {
            throw GameError(err::skip_forbidden());
        }
        P.actions = 0;
        markSkipped(P);
        log.add("ACTION: skip by '" + P.name + "'");
    }

}