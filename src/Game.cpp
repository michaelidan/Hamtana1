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
namespace coup {
std::string Game::turn() const { return players_.at(turnIndex)->name; }
std::vector<std::string> Game::players() const { std::vector<std::string> v; for(auto& p:players_) if(p->alive) v.push_back(p->name); return v; }
std::string Game::winner() const { if (draw) throw GameError(err::draw_no_single_winner()); if(!singleWinner) throw GameError(err::winner_not_ready()); return *singleWinner; }
std::vector<std::string> Game::winners() const { if(!draw){ if(singleWinner) return { *singleWinner }; return {}; } std::vector<std::string> v; for(auto& p:players_) if(p->alive) v.push_back(p->name); return v; }
Player& Game::addPlayer(const std::string& n, std::unique_ptr<Role> r){ players_.emplace_back(std::make_unique<Player>(n, std::move(r))); rebuildAuthorities(); return *players_.back(); }
Player& Game::current(){ return *players_.at(turnIndex); } const Player& Game::current() const { return *players_.at(turnIndex); }
size_t Game::aliveCount() const { size_t c=0; for(auto& p:players_) if(p->alive) ++c; return c; }
std::vector<Player*> Game::alivePlayers(){ std::vector<Player*> v; for(auto& p:players_) if(p->alive) v.push_back(p.get()); return v; }
void Game::rebuildAuthorities(){ authorities.clear(); for(auto& up:players_){ if(!up->alive) continue; if(up->isGovernor()) authorities.governors.push_back(up.get()); if(up->isJudge()) authorities.judges.push_back(up.get()); if(up->isGeneral()) authorities.generals.push_back(up.get()); } }
Player* Game::findPlayerByName(const std::string& n){ for(auto& p:players_) if(p->name==n) return p.get(); return nullptr; }
std::vector<Player*> Game::authorityListFor(Action a){ switch(a){ case Action::Tax: return authorities.governors; case Action::Bribe: return authorities.judges; case Action::Coup: return authorities.generals; default: return {}; } }
void Game::clearStatusesAtTurnStart(Player& p){ p.sanctionActive=false; p.jamActive=false; }
void Game::startTurn(){ if(aliveCount()<=1){ for(auto& p:players_) if(p->alive) singleWinner=p->name; return; } Player& P=current(); if(!inRound){ inRound=true; roundAnchorIndex=turnIndex; } clearStatusesAtTurnStart(P);
 bool mustCoup = (P.coins >= 10); if(P.isMerchant() && P.coins >= 3) P.coins += 1; P.actions=1; P.investedThisTurn=P.didPeekThisTurn=P.didJamThisTurn=false; P.forcedCoupFirstAction=mustCoup; P.bribePendingCoup=false; std::ostringstream ss; ss<<"START_TURN: '"<<P.name<<"' coins="<<P.coins<<" forcedCoup="<<(P.forcedCoupFirstAction?"true":"false"); log.add(ss.str()); }
void Game::endTurn(){ if(draw||singleWinner) return; size_t n=players_.size(); do{ turnIndex=(turnIndex+1)%n; }while(!players_.at(turnIndex)->alive); if(turnIndex==roundAnchorIndex){ bool allSkipped=true; for(auto& p:players_) if(p->alive){ if(!skippedThisRound[p.get()]){ allSkipped=false; break; } } if(allSkipped) ++skipRoundStreak; else skipRoundStreak=0; skippedThisRound.clear(); if(skipRoundStreak>=2){ draw=true; log.add("END_GAME: DRAW declared (two full rounds of all-skip)."); return; } roundAnchorIndex=turnIndex; } }
void Game::markSkipped(Player& p){ skippedThisRound[&p]=true; }
void Game::checkForcedCoupBefore(Action a){ Player& P=current(); if(P.forcedCoupFirstAction && a!=Action::Coup) throw GameError("הפעולה הראשונה חייבת להיות coup"); }
bool Game::tryBlock(Action a){
 if(!queuedBlockerName || !queuedBlockerAction || *queuedBlockerAction!=a) return false;
 Player* blocker = findPlayerByName(*queuedBlockerName); if(!blocker || !blocker->alive){ queuedBlockerName.reset(); queuedBlockerAction.reset(); return false; }
 auto list = authorityListFor(a); bool found=false; for(auto* cand:list){ if(cand==blocker){ found=true; break; } } if(!found){ queuedBlockerName.reset(); queuedBlockerAction.reset(); return false; }
 if(a==Action::Coup){ if(!blocker->isGeneral() || blocker->coins<5){ queuedBlockerName.reset(); queuedBlockerAction.reset(); return false; } blocker->coins-=5; log.add("BLOCK: General '"+blocker->name+"' blocked coup (-5)."); queuedBlockerName.reset(); queuedBlockerAction.reset(); return true; }
 if(a==Action::Bribe){ if(!blocker->isJudge()){ queuedBlockerName.reset(); queuedBlockerAction.reset(); return false; } log.add("BLOCK: Judge '"+blocker->name+"' blocked bribe."); queuedBlockerName.reset(); queuedBlockerAction.reset(); return true; }
 if(a==Action::Tax){ if(!blocker->isGovernor()){ queuedBlockerName.reset(); queuedBlockerAction.reset(); return false; } log.add("BLOCK: Governor '"+blocker->name+"' blocked tax."); queuedBlockerName.reset(); queuedBlockerAction.reset(); return true; }
 return false; }
void Game::gather(){ Player& P=current(); if(P.sanctionActive) throw GameError(err::gather_under_sanction()); checkForcedCoupBefore(Action::Gather); if(P.actions<=0) throw GameError("אין פעולות זמינות"); P.coins+=1; P.actions-=1; log.add("ACTION: gather +1 by '"+P.name+"'"); }
void Game::tax(){ Player& P=current(); if(P.sanctionActive) throw GameError(err::tax_under_sanction()); checkForcedCoupBefore(Action::Tax); if(P.actions<=0) throw GameError("אין פעולות זמינות"); int gain=2+(P.role?P.role->taxBonus():0); if(tryBlock(Action::Tax)){ P.actions-=1; log.add("ACTION: tax canceled by block for '"+P.name+"'"); return; } P.coins+=gain; P.actions-=1; log.add("ACTION: tax +"+std::to_string(gain)+" by '"+P.name+"'"); }
void Game::bribe(){ Player& P=current(); if(P.coins<4) throw GameError(err::not_enough_coins("bribe")); if(P.forcedCoupFirstAction && P.coins<11) throw GameError(err::bribe_under_coup()); if(P.bribePendingCoup) throw GameError("אסור bribe נוסף לפני coup"); P.coins-=4; if(tryBlock(Action::Bribe)){ log.add("ACTION: bribe blocked (coins lost) for '"+P.name+"'"); return; } P.actions+=1; P.forcedCoupFirstAction=true; P.bribePendingCoup=true; log.add("ACTION: bribe granted extra action for '"+P.name+"'"); }
void Game::arrest(Player& T){ Player& P=current(); checkForcedCoupBefore(Action::Arrest); if(P.actions<=0) throw GameError("אין פעולות זמינות"); if(!T.alive) throw GameError(err::target_eliminated()); if(&T==&P) throw GameError("לא ניתן לעצור את עצמך"); if(P.lastArrestTarget==&T) throw GameError(err::arrest_streak()); if(T.coins<=0) throw GameError(err::arrest_zero()); if(T.jamActive) throw GameError(err::arrest_jam()); if(T.isMerchant()){ int pay=std::min(2, T.coins); T.coins-=pay; } else if(T.isGeneral()){ /* net zero */ } else { T.coins-=1; P.coins+=1; } P.lastArrestTarget=&T; P.actions-=1; log.add("ACTION: arrest by '"+P.name+"' on '"+T.name+"'"); }
void Game::sanction(Player& T){ Player& P=current(); checkForcedCoupBefore(Action::Sanction); if(P.actions<=0) throw GameError("אין פעולות זמינות"); if(!T.alive) throw GameError(err::target_eliminated()); if(&T==&P) throw GameError("לא ניתן להטיל Sanction על עצמך"); int cost=3+(T.isJudge()?1:0); if(P.coins<cost) throw GameError(err::not_enough_coins("sanction")); P.coins-=cost; T.sanctionActive=true; if(T.isBaron()) T.coins+=1; P.actions-=1; log.add("STATUS: sanction applied on '"+T.name+"' by '"+P.name+"'"); }
void Game::coup(Player& T){ Player& P=current(); checkForcedCoupBefore(Action::Coup); if(P.actions<=0) throw GameError("אין פעולות זמינות"); if(!T.alive) throw GameError(err::target_eliminated()); if(&T==&P) throw GameError(err::self_coup()); if(P.coins<7) throw GameError(err::not_enough_coins("coup")); P.coins-=7; if(tryBlock(Action::Coup)){ P.actions-=1; P.forcedCoupFirstAction=false; P.bribePendingCoup=false; log.add("ACTION: coup blocked; '"+T.name+"' survives."); return; } T.alive=false; rebuildAuthorities(); P.actions-=1; P.forcedCoupFirstAction=false; P.bribePendingCoup=false; log.add("ACTION: coup succeeded; '"+T.name+"' eliminated."); if(aliveCount()==1){ for(auto& p:players_) if(p->alive) singleWinner=p->name; log.add("END_GAME: single winner '"+*singleWinner+"'"); } }
void Game::invest(){ Player& P=current(); checkForcedCoupBefore(Action::Invest); if(P.actions<=0) throw GameError("אין פעולות זמינות"); if(!P.isBaron()) throw GameError("רק Baron יכול להשקיע"); if(P.investedThisTurn) throw GameError("אי אפשר להשקיע פעמיים באותו תור"); if(P.coins<3) throw GameError(err::not_enough_coins("invest")); P.coins-=3; P.coins+=6; P.investedThisTurn=true; P.actions-=1; log.add("ACTION: invest by '"+P.name+"'"); }
void Game::peek(Player& T){ Player& P=current(); if(!P.isSpy()) throw GameError("רק Spy יכול לבצע peek"); if(P.didPeekThisTurn) throw GameError("peek פעם אחת בתור"); log.add("SPY: '"+P.name+"' peek '"+T.name+"' coins="+std::to_string(T.coins)); P.didPeekThisTurn=true; }
void Game::jamArrest(Player& T){ Player& P=current(); if(!P.isSpy()) throw GameError("רק Spy יכול לבצע jamArrest"); if(P.didJamThisTurn) throw GameError("jamArrest פעם אחת בתור"); T.jamActive=true; P.didJamThisTurn=true; log.add("STATUS: jamArrest applied on '"+T.name+"' by '"+P.name+"'"); }
void Game::skip(){ Player& P=current(); if(P.forcedCoupFirstAction) throw GameError(err::skip_forbidden()); P.actions=0; markSkipped(P); log.add("ACTION: skip by '"+P.name+"'"); }
}