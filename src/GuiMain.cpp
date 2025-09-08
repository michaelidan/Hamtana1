// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#include <SFML/Graphics.hpp>
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

struct Button { sf::RectangleShape box; sf::Text label; std::function<void()> onClick;
    bool contains(sf::Vector2f p) const { return box.getGlobalBounds().contains(p); } };
bool loadFont(sf::Font& font){
    if (font.loadFromFile("assets/DejaVuSans.ttf")) return true;
    if (font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) return true;
    return false;
}

int main(){
    Game g;
    auto& a = g.addPlayer("Alice", std::make_unique<Merchant>());
    auto& b = g.addPlayer("Bob",   std::make_unique<General>());
    auto& c = g.addPlayer("Carol", std::make_unique<Judge>());
    auto& d = g.addPlayer("Dan",   std::make_unique<Baron>());
    (void)a;(void)b;(void)c;(void)d;

    sf::RenderWindow win(sf::VideoMode(980, 640), "Coup — GUI (Michael v4)");
    win.setFramerateLimit(60);
    sf::Font font; bool fontOK = loadFont(font);
    if(!fontOK) std::cerr<<"[GUI] Warning: no font found; install fonts-dejavu-core.\n";
    auto makeText = [&](const std::string&s,unsigned sz){ sf::Text t; t.setString(s); if(fontOK) t.setFont(font); t.setCharacterSize(sz); t.setFillColor(sf::Color::White); return t; };

    int selectedIndex = -1;
    std::vector<Button> buttons;

    auto addButton = [&](std::string title, float x, float y, std::function<void()> cb){
        Button b; b.box.setPosition(x,y); b.box.setSize({180.f,40.f}); b.box.setFillColor(sf::Color(60,60,80));
        b.label = makeText(title, 18); b.label.setPosition(x+10,y+8); b.onClick = cb; buttons.push_back(std::move(b));
    };

    auto runBlockSequenceThen = [&](Action a, std::function<void()> perform){
        auto auth = g.authorityListFor(a); size_t idx=0;
        while(idx<auth.size()){
            Player* cand = auth[idx]; bool decided=false, wantBlock=false;
            while(win.isOpen() && !decided){
                sf::Event ev; while(win.pollEvent(ev)){
                    if(ev.type==sf::Event::Closed){ win.close(); return; }
                    if(ev.type==sf::Event::MouseButtonPressed){
                        auto mp = win.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y});
                        sf::FloatRect rYes(520,260,100,40), rNo(640,260,100,40);
                        if(rYes.contains(mp)){ wantBlock=true; decided=true; }
                        if(rNo.contains(mp)) { wantBlock=false; decided=true; }
                    }
                }
                win.clear(sf::Color(20,20,30));
                float y0=20; for(size_t i=0;i<g.players_.size();++i){ auto& up=g.players_[i]; if(!up->alive) continue;
                    sf::RectangleShape row({360.f,34.f}); row.setPosition(20,y0);
                    row.setFillColor((int)i==selectedIndex?sf::Color(70,70,90):sf::Color(40,40,60)); win.draw(row);
                    if(fontOK){ auto t=makeText(up->name+" — coins: "+std::to_string(up->coins)+(i==g.turnIndex?"  (TURN)":""),16); t.setPosition(30,y0+6); win.draw(t); }
                    y0+=40;
                }
                if(fontOK){ auto t=makeText("חסימה: "+cand->name+" עבור "+to_string(a),22); t.setPosition(420,180); win.draw(t); }
                sf::RectangleShape yes({100.f,40.f}); yes.setPosition(520,260); yes.setFillColor(sf::Color(80,120,80));
                sf::RectangleShape no({100.f,40.f});  no.setPosition(640,260);  no.setFillColor(sf::Color(120,80,80));
                win.draw(yes); win.draw(no);
                if(fontOK){ auto ty=makeText("חסום",18); ty.setPosition(545,268); win.draw(ty); auto tn=makeText("אל תחסום",18); tn.setPosition(648,268); win.draw(tn); }
                win.display();
            }
            if(wantBlock){ g.queueBlock(cand->name, a); break; } else { idx++; }
        }
        perform();
    };

    auto rebuildButtons = [&](){
        buttons.clear();
        addButton("start turn", 420, 60,  [&]{ g.startTurn(); });
        addButton("end turn",   420, 110, [&]{ g.endTurn(); });
        addButton("gather",     630, 60,  [&]{ g.gather(); });
        addButton("tax",        630, 110, [&]{ runBlockSequenceThen(Action::Tax,   [&]{ g.tax();   }); });
        addButton("bribe",      630, 160, [&]{ runBlockSequenceThen(Action::Bribe, [&]{ g.bribe(); }); });
        addButton("arrest",     840, 60,  [&]{ if(selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); g.arrest(*t);} });
        addButton("sanction",   840, 110, [&]{ if(selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); g.sanction(*t);} });
        addButton("coup",       840, 160, [&]{ if(selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); runBlockSequenceThen(Action::Coup,[&]{ g.coup(*t);}); } });
        addButton("invest (Baron)", 630, 210, [&]{ g.invest(); });
        addButton("skip",           840, 210, [&]{ g.skip(); });
    };
    rebuildButtons();

    while(win.isOpen()){
        sf::Event ev;
        while(win.pollEvent(ev)){
            if(ev.type==sf::Event::Closed) win.close();
            if(ev.type==sf::Event::MouseButtonPressed){
                auto mp=win.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y});
                float y0=20; for(size_t i=0;i<g.players_.size();++i){ auto& up=g.players_[i]; if(!up->alive) continue;
                    sf::FloatRect r(20,y0,360,34); if(r.contains(mp)) selectedIndex=(int)i; y0+=40; }
                for(auto& b:buttons){ if(b.contains(mp) && b.onClick){ try{ b.onClick(); }catch(const GameError& e){ std::cerr<<"[ERR] "<<e.what()<<"\n"; } } }
            }
        }
        win.clear(sf::Color(20,20,30));
        float y0=20; for(size_t i=0;i<g.players_.size();++i){ auto& up=g.players_[i]; if(!up->alive) continue;
            sf::RectangleShape row({360.f,34.f}); row.setPosition(20,y0);
            row.setFillColor((int)i==selectedIndex?sf::Color(70,70,90):sf::Color(40,40,60)); win.draw(row);
            if(fontOK){ auto t=makeText(up->name+" — coins: "+std::to_string(up->coins)+(i==g.turnIndex?"  (TURN)":""),16); t.setPosition(30,y0+6); win.draw(t); }
            y0+=40;
        }
        for(auto& b:buttons){ win.draw(b.box); if(fontOK) win.draw(b.label); }
        if(fontOK){ auto t=makeText("תור: "+g.turn(),20); t.setPosition(20,560); win.draw(t);
            auto s=makeText(g.isDraw()?"מצב: תיקו":(g.singleWinner?("מנצח: "+*g.singleWinner):"מצב: פעיל"),18); s.setPosition(20,590); win.draw(s); }
        win.display();
    }
    return 0;
}