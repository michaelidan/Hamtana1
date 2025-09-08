// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#include <iostream>
#include "Game.hpp"
#include "Player.hpp"
#include "roles/Governor.hpp"
#include "roles/Judge.hpp"
#include "roles/General.hpp"
#include "roles/Baron.hpp"
#include "roles/Merchant.hpp"
#include "roles/Spy.hpp"
using namespace coup;
int main(){
    Game g;
    auto& a=g.addPlayer("Alice", std::make_unique<Merchant>());
    auto& b=g.addPlayer("Bob",   std::make_unique<General>());
    auto& c=g.addPlayer("Carol", std::make_unique<Judge>());
    auto& d=g.addPlayer("Dan",   std::make_unique<Baron>());
    (void)a;(void)b;(void)c;(void)d;
    std::cout<<"Players: "; for(auto&s:g.players()) std::cout<<s<<" "; std::cout<<"\n";
    for(int i=0;i<6 && !g.isDraw() && !g.singleWinner.has_value();++i){
        std::cout<<"Turn: "<<g.turn()<<"\n"; g.startTurn(); auto& P=g.current();
        try{
            if(P.coins>=10){ for(auto*t:g.alivePlayers()) if(t!=&P){ g.coup(*t); break; } }
            else if(P.isBaron() && P.coins>=3){ g.invest(); }
            else if(!P.sanctionActive){ g.gather(); }
            else { g.skip(); }
        }catch(const GameError& e){ std::cout<<"Error: "<<e.what()<<"\n"; try{ g.skip(); }catch(...){} }
        g.endTurn();
    }
    std::cout<<"Log ("<<g.log.entries().size()<<"):\n"; for(auto&s:g.log.entries()) std::cout<<s<<"\n";
    if(g.isDraw()) std::cout<<"DRAW\n"; else if(g.singleWinner) std::cout<<"Winner: "<<*g.singleWinner<<"\n"; else std::cout<<"Active\n";
}