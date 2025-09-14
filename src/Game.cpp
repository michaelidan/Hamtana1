// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael) — v5.4
// שינויי v5.4:
// - "חייב Coup" נקבע רק בתחילת תור (לפני בונוסים) אם coins>=10
// - Bribe לא יוצר חובה לקופה (רק +1 נטו), אך מותר בזמן חובה רק אם coins>=11
// - בזמן חובה: כל פעולה שאינה Coup/Bribe(11+) נחסמת במנוע
// - Jam נבדק על המבצע (השחקן שבתורו), לא על היעד
// - מניעת-עצמי (self-block) אסורה; גנרל חסימה רק אם יש לו >=5 מטבעות
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

    std::vector<std::string> Game::players() const {
        std::vector<std::string> v;
        for (auto& p : players_) if (p->alive) v.push_back(p->name);
        return v;
    }

    std::string Game::winner() const {
        if (draw) throw GameError(err::draw_no_single_winner());
        if (!singleWinner) throw GameError(err::winner_not_ready());
        return *singleWinner;
    }

    std::vector<std::string> Game::winners() const {
        if (!draw) {
            if (singleWinner) return {*singleWinner};
            return {};
        }
        std::vector<std::string> v;
        for (auto& p : players_) if (p->alive) v.push_back(p->name);
        return v;
    }

    Player& Game::addPlayer(const std::string& n, std::unique_ptr<Role> r) {
        players_.emplace_back(std::make_unique<Player>(n, std::move(r)));
        rebuildAuthorities();
        return *players_.back();
    }

    Player& Game::current() { return *players_.at(turnIndex); }
    const Player& Game::current() const { return *players_.at(turnIndex); }

    size_t Game::aliveCount() const {
        size_t c=0; for (auto& p:players_) if (p->alive) ++c; return c;
    }

    std::vector<Player*> Game::alivePlayers() {
        std::vector<Player*> v;
        for (auto& p:players_) if (p->alive) v.push_back(p.get());
        return v;
    }

    void Game::rebuildAuthorities() {
        authorities.clear();
        for (auto& up: players_) {
            if (!up->alive) continue;
            if (up->isGovernor()) authorities.governors.push_back(up.get());
            if (up->isJudge())    authorities.judges.push_back(up.get());
            if (up->isGeneral())  authorities.generals.push_back(up.get());
        }
    }

    Player* Game::findPlayerByName(const std::string& n) {
        for (auto& p:players_) if (p->name==n) return p.get();
        return nullptr;
    }

    std::vector<Player*> Game::authorityListFor(Action a) {
        switch (a) {
            case Action::Tax:   return authorities.governors;
            case Action::Bribe: return authorities.judges;
            case Action::Coup:  return authorities.generals;
            default:            return {};
        }
    }

    // מאפס סטטוסים בסיום תור של בעל התור
    void Game::clearStatusesAtTurnStart(Player& P) {
        bool hadSanction = P.sanctionActive;
        bool hadJam      = P.jamActive;
        P.sanctionActive = false;
        P.jamActive      = false;
        if (hadSanction) log.add("STATUS: sanction expired for '" + P.name + "'");
        if (hadJam)      log.add("STATUS: jam expired for '" + P.name + "'");
    }

    void Game::startTurn() {
        if (aliveCount() <= 1) {
            for (auto& p:players_) if (p->alive) singleWinner = p->name;
            return;
        }
        Player& P = current();

        if (!inRound) { inRound = true; roundAnchorIndex = turnIndex; }

        // חובה לקופה נקבעת לפי מצב המטבעות לפני בונוסים
        int coinsBeforeBonus = P.coins;
        bool mustCoupThisTurn = (coinsBeforeBonus >= 10);

        // בונוס סוחר (לא משפיע על קביעת החובה)
        if (P.isMerchant() && P.coins >= 3) P.coins += 1;

        P.actions = 1;
        P.investedThisTurn = false;
        P.didPeekThisTurn = P.didJamThisTurn = false; // peek לא בשימוש ב-GUI, אבל נשאר בטיחותית
        P.forcedCoupFirstAction = mustCoupThisTurn;   // משתמשים בשדה כ"דגל חובה לתור"
        P.bribePendingCoup = false;                   // לא משתמשים יותר בכללי v5.4

        std::ostringstream ss;
        ss << "START_TURN: '" << P.name << "' coins=" << P.coins
           << " forcedCoup=" << (P.forcedCoupFirstAction ? "true" : "false");
        log.add(ss.str());
    }

    void Game::endTurn() {
        Player& P = current();
        clearStatusesAtTurnStart(P); // איפוס סטטוסים של בעל התור

        if (draw || singleWinner) return;

        size_t n = players_.size();
        do { turnIndex = (turnIndex + 1) % n; } while (!players_.at(turnIndex)->alive);

        if (turnIndex == roundAnchorIndex) {
            bool allSkipped = true;
            for (auto& p:players_) if (p->alive) {
                if (!skippedThisRound[p.get()]) { allSkipped = false; break; }
            }
            if (allSkipped) ++skipRoundStreak; else skipRoundStreak = 0;
            skippedThisRound.clear();
            if (skipRoundStreak >= 2) {
                draw = true;
                log.add("END_GAME: DRAW declared (two full rounds of all-skip).");
                return;
            }
            roundAnchorIndex = turnIndex;
        }
    }

    void Game::markSkipped(Player& p) { skippedThisRound[&p] = true; }

    // בזמן חובה: מותר רק Coup; או Bribe אם coins>=11
    void Game::checkForcedCoupBefore(Action a) {
        Player& P = current();
        if (!P.forcedCoupFirstAction) return;

        if (a == Action::Coup) return;

        if (a == Action::Bribe) {
            if (P.coins >= 11) return; // מותר לשחד ואז עדיין ניתן לבצע Coup (יישארו >=7)
            throw GameError("bribe forbidden: coup is mandatory");
        }

        throw GameError("הפעולה הראשונה חייבת להיות coup");
    }

    bool Game::tryBlock(Action a) {
        if (!queuedBlockerName || !queuedBlockerAction || *queuedBlockerAction != a) return false;

        Player* blocker = findPlayerByName(*queuedBlockerName);
        // נקה תור-חסימה אם לא תקין
        auto resetQ = [&]{ queuedBlockerName.reset(); queuedBlockerAction.reset(); };

        if (!blocker || !blocker->alive) { resetQ(); return false; }

        // לא מאפשרים חסימה עצמית
        if (blocker == &current()) { resetQ(); return false; }

        auto list = authorityListFor(a);
        bool found = false;
        for (auto* cand : list) if (cand == blocker) { found = true; break; }
        if (!found) { resetQ(); return false; }

        if (a == Action::Coup) {
            if (!blocker->isGeneral() || blocker->coins < 5) { resetQ(); return false; }
            blocker->coins -= 5;
            log.add("BLOCK: General '" + blocker->name + "' blocked coup (-5).");
            resetQ();
            return true;
        }
        if (a == Action::Bribe) {
            if (!blocker->isJudge()) { resetQ(); return false; }
            log.add("BLOCK: Judge '" + blocker->name + "' blocked bribe.");
            resetQ();
            return true;
        }
        if (a == Action::Tax) {
            if (!blocker->isGovernor()) { resetQ(); return false; }
            log.add("BLOCK: Governor '" + blocker->name + "' blocked tax.");
            resetQ();
            return true;
        }
        return false;
    }

    void Game::gather() {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        checkForcedCoupBefore(Action::Gather);
        if (p.sanctionActive) throw GameError("sanctioned: cannot gather/tax this turn");
        if (p.actions <= 0)   throw GameError("no actions left");

        p.coins += 1;
        p.actions -= 1;
        log.add("ACTION: gather +1 by '" + p.name + "' (coins=" + std::to_string(p.coins) + ")");
    }

    void Game::tax() {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        checkForcedCoupBefore(Action::Tax);
        if (p.sanctionActive) throw GameError("sanctioned: cannot gather/tax this turn");
        if (p.actions <= 0)   throw GameError("no actions left");

        int gain = p.isGovernor() ? 3 : 2;

        if (tryBlock(Action::Tax)) {
            log.add("BLOCK: tax was blocked by Governor");
            p.actions -= 1; // הפעולה נצרכה גם אם נחסמה
            return;
        }

        p.coins += gain;
        p.actions -= 1;
        log.add("ACTION: tax +" + std::to_string(gain) + " by '" + p.name + "' (coins=" + std::to_string(p.coins) + ")");
    }

    void Game::bribe() {
        auto& p = current();
        if (!p.alive)         throw GameError("cannot act: player is eliminated");
        checkForcedCoupBefore(Action::Bribe); // בזמן חובה מותר רק אם coins>=11
        if (p.actions <= 0)   throw GameError("no actions left");
        if (p.coins < 4)      throw GameError("not enough coins for bribe");

        p.coins -= 4;

        if (tryBlock(Action::Bribe)) {
            p.actions -= 1;  // הפעולה נצרכה גם אם נחסמה
            log.add("BLOCK: bribe was blocked by Judge; coins lost");
            return;
        }

        // נטו +1 פעולה
        p.actions -= 1;
        p.actions += 2;

        // אין יותר "bribePendingCoup" — הכלל נקבע רק בתחילת תור
        log.add("ACTION: bribe by '" + p.name + "' -> +1 net action");
    }

    void Game::arrest(Player& T) {
        Player& P = current();
        checkForcedCoupBefore(Action::Arrest);
        if (P.actions <= 0)      throw GameError("אין פעולות זמינות");
        if (!T.alive)            throw GameError(err::target_eliminated());
        if (&T == &P)            throw GameError("לא ניתן לעצור את עצמך");
        if (P.lastArrestTarget == &T) throw GameError(err::arrest_streak());
        if (T.coins <= 0)        throw GameError(err::arrest_zero());
        if (P.jamActive)         throw GameError(err::arrest_jam()); // חשוב: ג'אם על המבצע

        if (T.isMerchant()) {
            int pay = std::min(2, T.coins);
            T.coins -= pay;
        } else if (T.isGeneral()) {
            /* net zero */
        } else {
            T.coins -= 1;
            P.coins += 1;
        }
        P.lastArrestTarget = &T;
        P.actions -= 1;
        log.add("ACTION: arrest by '" + P.name + "' on '" + T.name + "'");
    }

    void Game::sanction(Player& T) {
        Player& P = current();
        checkForcedCoupBefore(Action::Sanction);
        if (P.actions <= 0)   throw GameError("אין פעולות זמינות");
        if (!T.alive)         throw GameError(err::target_eliminated());
        if (&T == &P)         throw GameError("לא ניתן להטיל Sanction על עצמך");
        int cost = 3 + (T.isJudge()?1:0);
        if (P.coins < cost)   throw GameError(err::not_enough_coins("sanction"));

        P.coins -= cost;
        T.sanctionActive = true;
        if (T.isBaron()) T.coins += 1;
        P.actions -= 1;
        log.add("STATUS: sanction applied on '" + T.name + "' by '" + P.name + "'");
    }

    void Game::coup(Player& T) {
        Player& P = current();

        checkForcedCoupBefore(Action::Coup);

        if (P.actions <= 0)   throw GameError("no actions left");
        if (!T.alive)         throw GameError(err::target_eliminated());
        if (&T == &P)         throw GameError(err::self_coup());
        if (P.coins < 7)      throw GameError(err::not_enough_coins("coup"));

        // תשלום על ההפיכה
        P.coins -= 7;

        // ניסיון חסימה (גנרל משלם 5 – ההיגיון בתוך tryBlock)
        if (tryBlock(Action::Coup)) {
            // הפעולה נצרכה; מבטלים חובה; נשארים בתור אם יש פעולות
            P.actions -= 1;
            P.forcedCoupFirstAction = false; // חסימה משחררת את החובה
            log.add("ACTION: coup blocked; '" + T.name + "' survives");
            log.add("ACTION: coup resolved; actions left = " + std::to_string(P.actions));
            return;
        }

        // לא נחסם – היעד מודח
        T.alive = false;
        rebuildAuthorities();

        P.actions -= 1;
        P.forcedCoupFirstAction = false; // החובה קוימה
        log.add("ACTION: coup succeeded; '" + T.name + "' eliminated");
        log.add("ACTION: coup resolved; actions left = " + std::to_string(P.actions));

        if (aliveCount() == 1) {
            for (auto& p : players_) if (p->alive) singleWinner = p->name;
            log.add("END_GAME: single winner '" + *singleWinner + "'");
        }
    }

    void Game::invest() {
        Player& P = current();
        checkForcedCoupBefore(Action::Invest);
        if (P.actions <= 0)   throw GameError("אין פעולות זמינות");
        if (!P.isBaron())     throw GameError("רק Baron יכול להשקיע");
        if (P.investedThisTurn) throw GameError("אי אפשר להשקיע פעמיים באותו תור");
        if (P.coins < 3)      throw GameError(err::not_enough_coins("invest"));
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
        p.didPeekThisTurn = true;
        log.add("SPY: '" + p.name + "' peeked '" + t.name + "' -> coins=" + std::to_string(t.coins));
    }

    void Game::jamArrest(Player& t) {
        auto& p = current();
        if (!p.alive) throw GameError("cannot act: player is eliminated");
        if (!p.isSpy()) throw GameError("only Spy can jam arrest");
        if (p.didJamThisTurn) throw GameError("jam already used this turn");
        if (!t.alive) throw GameError("target is eliminated");
        t.jamActive = true;     // ייחסם מ-Arrest בתורו הבא
        p.didJamThisTurn = true;
        log.add("SPY: '" + p.name + "' jam-arrest on '" + t.name + "' (next turn only)");
    }

    void Game::skip() {
        Player& P = current();
        if (P.forcedCoupFirstAction) throw GameError(err::skip_forbidden());
        P.actions = 0;
        markSkipped(P);
        log.add("ACTION: skip by '" + P.name + "'");
    }
}
