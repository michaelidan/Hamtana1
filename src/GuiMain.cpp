// =============================================
// Emails: ronavraham1999@gmail.com_michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael/Ron v5.1)
// הערות בעברית בכל המקומות החשובים. ה-UI על המסך באנגלית בלבד.
// =============================================
#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>
#include <optional>
#include <cctype>

#include "Game.hpp"
#include "Player.hpp"
#include "roles/Governor.hpp"
#include "roles/Judge.hpp"
#include "roles/General.hpp"
#include "roles/Baron.hpp"
#include "roles/Merchant.hpp"
#include "roles/Spy.hpp"

using namespace coup;

// ---------- טעינת פונט: קודם assets/, אם לא קיים – נתיב מערכת ----------
static bool loadFont(sf::Font& font){
    if (font.loadFromFile("assets/DejaVuSans.ttf")) return true;
    if (font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) return true;
    return false;
}

// טקסט נוח ל־UTF-8
static sf::Text makeText(const std::string& s, unsigned sz, const sf::Font* font){
    sf::Text t;
    if(font) t.setFont(*font);
    t.setString(sf::String::fromUtf8(s.begin(), s.end()));
    t.setCharacterSize(sz);
    t.setFillColor(sf::Color::White);
    return t;
}

// ---------- מצב מסך ----------
enum class UIState { Setup, Playing };

// קופסאות טקסט לשמות שחקנים במסך ה־Setup
struct TextBox {
    sf::RectangleShape box;
    sf::Text label;
    std::string text;
    bool focused{false};
    void draw(sf::RenderWindow& w){ w.draw(box); w.draw(label); }
    void setString(const std::string& s, const sf::Font* font){
        text = s; 
        label = makeText(s, 18, font);
        label.setPosition(box.getPosition().x + 8, box.getPosition().y + 6);
    }
    void setPos(float x, float y){ box.setPosition(x,y); label.setPosition(x+8,y+6); }
    void setSize(float w, float h){ box.setSize({w,h}); }
};

int main(){
    // ---------- חלון ----------
    sf::RenderWindow win(sf::VideoMode(1000, 700), "Coup — GUI (v5.1)");
    win.setFramerateLimit(60);

    // ---------- פונט ----------
    sf::Font font; bool fontOK = loadFont(font);
    if(!fontOK) std::cerr<<"[GUI] Warning: no font found; install fonts-dejavu-core or copy assets/DejaVuSans.ttf\n";

    // ---------- מסך Setup ----------
    UIState state = UIState::Setup;
    int desiredPlayers = 4; // בחירת מספר שחקנים 2..6
    std::vector<TextBox> nameBoxes(6);
    for(int i=0;i<6;++i){
        nameBoxes[i].box.setSize({300.f,32.f});
        nameBoxes[i].box.setFillColor(sf::Color(40,40,60));
        nameBoxes[i].box.setOutlineThickness( nameBoxes[i].focused?2.f:1.f );
        nameBoxes[i].box.setOutlineColor(sf::Color(90,90,120));
        nameBoxes[i].setPos(60.f, 160.f + i*40.f);
        nameBoxes[i].setString("Player"+std::to_string(i+1), fontOK?&font:nullptr);
    }

    auto drawSetup = [&](){
        win.clear(sf::Color(20,20,30));
        auto title = makeText("Setup: choose number of players (2-6) and type names", 22, fontOK?&font:nullptr);
        title.setPosition(60, 40); 
        win.draw(title);

        // כפתורי בחירה 2..6
        for(int k=2;k<=6;++k){
            sf::RectangleShape b({40.f,40.f}); 
            b.setPosition(60+(k-2)*50.f, 90.f);
            b.setFillColor(k==desiredPlayers?sf::Color(80,140,80):sf::Color(60,60,80));
            win.draw(b);
            auto t = makeText(std::to_string(k), 18, fontOK?&font:nullptr);
            t.setPosition(60+(k-2)*50.f+13, 98); 
            win.draw(t);
        }

        // קופסאות שמות
        for(int i=0;i<desiredPlayers;++i){
            nameBoxes[i].box.setOutlineThickness(nameBoxes[i].focused?2.f:1.f);
            nameBoxes[i].draw(win);
        }

        // כפתור Start game
        sf::RectangleShape start({160.f,40.f}); 
        start.setPosition(60, 160.f + desiredPlayers*40.f + 20.f);
        start.setFillColor(sf::Color(80,140,80)); 
        win.draw(start);
        auto st = makeText("Start game", 18, fontOK?&font:nullptr); 
        st.setPosition(start.getPosition().x+28, start.getPosition().y+8); 
        win.draw(st);

        win.display();
    };

    // ---------- מודל משחק ----------
    Game g;
    bool turnStarted = false;   // פעולות מותרות רק אחרי startTurn
    int selectedIndex = -1;     // יעד נבחר לפעולות עם Target

    // ---------- אזור לוג גלול ----------
    sf::FloatRect logBounds(420.f, 440.f, 560.f, 220.f);
    int logScroll = 0;
    const int LOG_LINES = 10;

    // ---------- כפתורים ----------
    struct Button {
        sf::RectangleShape box;
        sf::Text label;
        std::function<void()> onClick;
        bool enabled{true};
        bool requiresTarget{false};
        std::optional<Action> action;
    };
    std::vector<Button> buttons;

    auto addButton = [&](std::string title, float x, float y, std::optional<Action> act, bool requiresTarget, std::function<void()> cb){
        Button b;
        b.box.setPosition(x,y); 
        b.box.setSize({180.f,40.f});
        b.box.setFillColor(sf::Color(60,60,80));
        b.label = makeText(title, 18, fontOK?&font:nullptr);
        b.label.setPosition(x+10,y+8);
        b.action = act;
        b.requiresTarget = requiresTarget;
        b.onClick = std::move(cb);
        buttons.push_back(std::move(b));
    };

    auto rebuildButtons = [&](){
        buttons.clear();
        addButton("start turn",      420,  60, std::nullopt, false, [&]{ if(!turnStarted){ g.startTurn(); turnStarted=true; } });
        addButton("end turn",        420, 110, std::nullopt, false, [&]{ if(turnStarted){ g.endTurn(); turnStarted=false; selectedIndex=-1; } });
        addButton("gather",          630,  60, Action::Gather,   false, [&]{ if(turnStarted) g.gather(); });
        addButton("tax",             630, 110, Action::Tax,      false, [&]{ /* חסימה תטופל למטה */ });
        addButton("bribe",           630, 160, Action::Bribe,    false, [&]{ /* חסימה תטופל למטה */ });
        addButton("invest (Baron)",  630, 210, Action::Invest,   false, [&]{ if(turnStarted) g.invest(); });

        addButton("arrest",          840,  60, Action::Arrest,   true,  [&]{ if(turnStarted && selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); g.arrest(*t);} });
        addButton("sanction",        840, 110, Action::Sanction, true,  [&]{ if(turnStarted && selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); g.sanction(*t);} });
        addButton("coup",            840, 160, Action::Coup,     true,  [&]{ if(turnStarted && selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); /* חסימה למטה */ } });
        addButton("skip",            840, 210, Action::Skip,     false, [&]{ if(turnStarted) g.skip(); });

        // Spy (ללא חלון חסימה)
        addButton("peek (Spy)",      420, 160, std::nullopt, true,  [&]{ if (turnStarted && selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); try{ g.peek(*t);}catch(const GameError& e){ std::cerr<<"[ERR] "<<e.what()<<"\n"; } }});
        addButton("jam arrest (Spy)",420, 210, std::nullopt, true,  [&]{ if (turnStarted && selectedIndex>=0){ auto* t=g.players_[selectedIndex].get(); try{ g.jamArrest(*t);}catch(const GameError& e){ std::cerr<<"[ERR] "<<e.what()<<"\n"; } }});

        addButton("save log",        420, 310, std::nullopt,     false, [&]{
            if (g.log.saveToFile("logs/game_log.txt"))
                std::cerr<<"[GUI] Log saved to logs/game_log.txt\n";
            else
                std::cerr<<"[GUI] Failed to save log\n";
        });
    };
    rebuildButtons();

    auto canSpyPeek = [&]()->bool{
        if (!turnStarted) return false;
        Player& P = g.current();
        if (!P.isSpy()) return false;
        if (P.didPeekThisTurn) return false;
        return true;
    };
    auto canSpyJam = [&](Player* T)->bool{
        if (!turnStarted) return false;
        if (!T) return false;
        Player& P = g.current();
        if (!P.isSpy()) return false;
        if (P.didJamThisTurn) return false;
        if (!T->alive || T==&P) return false;
        return true;
    };

    // בדיקת היתכנות פעולה לפי מצב המשחק — זה שולט בצביעה ירוק/אפור
    auto canAction = [&](Action a, Player* T) -> bool {
        if (!turnStarted && a != Action::Skip) return false; // כלום לפני start turn
        Player& P = g.current();

        switch (a) {
        case Action::Gather:
            if (P.sanctionActive || P.actions <= 0) return false;
            if (!P.forcedCoupFirstAction) return true;
            // אחרי bribe: מותר לאסוף עד שמגיעים ל-7
            return (P.bribePendingCoup && P.coins < 7);

        case Action::Tax:
            if (P.sanctionActive || P.actions <= 0) return false;
            if (!P.forcedCoupFirstAction) return true;
            // אחרי bribe: מותר למסות עד שמגיעים ל-7
            return (P.bribePendingCoup && P.coins < 7);

        case Action::Bribe:
            if (P.actions <= 0)     return false;
            if (P.bribePendingCoup) return false;      // לא עושים bribe על bribe
            if (P.forcedCoupFirstAction)
                return P.coins >= 11;                  // כשכבר "חייב coup" — מותר לשחד רק אם אפשר מייד לבצע coup
            return P.coins >= 4;

        case Action::Arrest:
            if (!T) return false;
            if (P.forcedCoupFirstAction) return false;
            if (P.actions <= 0) return false;
            if (!T->alive || T==&P) return false;
            if (P.lastArrestTarget==T) return false;
            if (T->coins <= 0) return false;
            if (T->jamActive) return false;
            return true;

        case Action::Sanction: {
            if (!T) return false;
            if (P.forcedCoupFirstAction) return false;
            if (P.actions <= 0) return false;
            if (!T->alive || T==&P) return false;
            int cost = 3 + (T->isJudge()?1:0);
            return P.coins >= cost;
        }

        case Action::Coup:
            if (!T) return false;
            if (P.actions <= 0) return false;
            if (!T->alive || T==&P) return false;
            return P.coins >= 7;

        case Action::Invest:
            if (P.forcedCoupFirstAction) return false;
            if (P.actions <= 0) return false;
            if (!P.isBaron()) return false;
            if (P.investedThisTurn) return false;
            return P.coins >= 3;

        case Action::Skip:
            // אסור "skip" רק כשחייבים coup עכשיו (יש 7+)
            return !(P.forcedCoupFirstAction && P.coins >= 7);
        }
        return false;
    };



    // רצף חסימה אינטראקטיבי: מציג מועמד/ת סמכות ושואל אם לחסום
    auto runBlockSequenceThen = [&](Action a, std::function<void()> perform){
        auto auth = g.authorityListFor(a); size_t idx=0;
        while(idx<auth.size()){
            Player* cand = auth[idx]; bool decided=false, wantBlock=false;
            while(win.isOpen() && !decided){
                sf::Event ev; 
                while(win.pollEvent(ev)){
                    if(ev.type==sf::Event::Closed){ win.close(); return; }
                    if(ev.type==sf::Event::MouseButtonPressed){
                        auto mp = win.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y});
                        sf::FloatRect rYes(560,300,120,44), rNo(700,300,120,44);
                        if(rYes.contains(mp)){ wantBlock=true; decided=true; }
                        if(rNo.contains(mp)) { wantBlock=false; decided=true; }
                    }
                }
                win.clear(sf::Color(20,20,30));
                // רשימת שחקנים משמאל
                float y0=20; 
                for(size_t i=0;i<g.players_.size();++i){ 
                    auto& up=g.players_[i]; if(!up->alive) continue;
                    sf::RectangleShape row({380.f,34.f}); row.setPosition(20,y0);
                    row.setFillColor((int)i==selectedIndex?sf::Color(70,70,90):sf::Color(40,40,60)); 
                    std::string role = up->role?up->role->name():"?";
                    auto t=makeText(up->name+" ("+role+") - coins: "+std::to_string(up->coins)+(i==g.turnIndex?"  (TURN)":"") ,16, fontOK?&font:nullptr); 
                    t.setPosition(30,y0+6);

                    // שחקן שבתורו עכשיו — מסגרת ירוקה
                    if ((int)i == g.turnIndex) {
                        row.setOutlineThickness(3.f);
                        row.setOutlineColor(sf::Color(80,160,80));
                    } else {
                        row.setOutlineThickness(0.f);
                    }

                    win.draw(row);
                    win.draw(t);

                    // תגי סטטוסים
                    if (up->sanctionActive) {
                        auto badge = makeText("SANCTIONED", 14, fontOK?&font:nullptr);
                        badge.setFillColor(sf::Color(200, 40, 40));
                        badge.setPosition(320, y0 + 6);
                        win.draw(badge);
                    }
                    if (up->jamActive) {
                        auto badge2 = makeText("JAMMED", 14, fontOK?&font:nullptr);
                        badge2.setFillColor(sf::Color(200,200,40));
                        float dy = up->sanctionActive ? 24.f : 0.f;
                        badge2.setPosition(320, y0 + 6 + dy);
                        win.draw(badge2);
                    }

                    y0+=40;
                }
                // טקסט הדיאלוג
                std::string actor = g.current().name;
                std::string candrole = cand->role?cand->role->name():"?";
                auto msg = makeText(candrole+" "+cand->name+": Block "+to_string(a)+" by "+actor+"?", 22, fontOK?&font:nullptr);
                msg.setPosition(420,220); win.draw(msg);
                // כפתורים
                sf::RectangleShape yes({120.f,44.f}); yes.setPosition(560,300); yes.setFillColor(sf::Color(80,140,80));
                sf::RectangleShape no ({120.f,44.f}); no .setPosition(700,300); no .setFillColor(sf::Color(120,80,80));
                win.draw(yes); win.draw(no);
                auto ty=makeText("Block",18, fontOK?&font:nullptr);      ty.setPosition(595,310); win.draw(ty);
                auto tn=makeText("Don't block",18, fontOK?&font:nullptr);tn.setPosition(710,310); win.draw(tn);
                win.display();
            }
            if(wantBlock){ g.queueBlock(cand->name, a); break; } else { idx++; }
        }
        perform();
    };

    // צביעה ירוק/אפור ועדכון enabled
    auto updateButtons = [&](){
        Player* target = (selectedIndex>=0 ? g.players_[selectedIndex].get() : nullptr);
        for(auto& b:buttons){
            if(b.action.has_value()){
                bool en = canAction(*b.action, b.requiresTarget?target:nullptr);
                b.enabled = en;
            } else if (b.label.getString() == sf::String("peek (Spy)")) {
                b.enabled = canSpyPeek();
            } else if (b.label.getString() == sf::String("jam arrest (Spy)")) {
                Player* T = (selectedIndex>=0 ? g.players_[selectedIndex].get() : nullptr);
                b.enabled = canSpyJam(T);
            } else {
                // start/end/save log
                if(b.label.getString() == sf::String("start turn")){
                    b.enabled = !turnStarted && !g.isDraw() && !g.singleWinner.has_value();
                } else if (b.label.getString() == sf::String("end turn")) {
                    Player& P = g.current();
                    bool mustCoupNow = P.forcedCoupFirstAction && P.coins >= 7;
                    // מותר לסיים תור כל עוד לא חייבים coup עכשיו
                    b.enabled = turnStarted && !mustCoupNow;
                } else {
                    b.enabled = true; // save log וכו'
                }
            }
            b.box.setFillColor(b.enabled ? sf::Color(80,140,80) : sf::Color(60,60,80));
        }
    };



    // ---------- לולאת אירועים/ציור ----------
    while(win.isOpen()){
        if(state==UIState::Setup){
            // ציור המסך
            drawSetup();

            // אירועים למסך ה־Setup
            sf::Event ev;
            while(win.pollEvent(ev)){
                if(ev.type==sf::Event::Closed) win.close();

                if(ev.type==sf::Event::MouseButtonPressed){
                    auto mp = win.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y});
                    // בחירת 2..6
                    for(int k=2;k<=6;++k){
                        sf::FloatRect r(60+(k-2)*50.f, 90.f, 40.f, 40.f);
                        if(r.contains(mp)) desiredPlayers = k;
                    }
                    // פוקוס על קופסאות שמות
                    for(int i=0;i<desiredPlayers;++i){
                        sf::FloatRect r(nameBoxes[i].box.getGlobalBounds());
                        nameBoxes[i].focused = r.contains(mp);
                    }
                    // התחלת המשחק
                    sf::FloatRect rStart(60, 160.f + desiredPlayers*40.f + 20.f, 160.f, 40.f);
                    if(rStart.contains(mp)){
                        // בניית המשחק: תפקידים קבועים במחזור
                        std::vector<std::string> names;
                        for(int i=0;i<desiredPlayers;++i) names.push_back(nameBoxes[i].text);
                        g = Game(); // reset
                        for(int i=0;i<desiredPlayers;++i){
                            int rix = i % 6;
                            std::unique_ptr<Role> r;
                            if(rix==0) r = std::make_unique<Merchant>();
                            if(rix==1) r = std::make_unique<General>();
                            if(rix==2) r = std::make_unique<Judge>();
                            if(rix==3) r = std::make_unique<Baron>();
                            if(rix==4) r = std::make_unique<Governor>();
                            if(rix==5) r = std::make_unique<Spy>();
                            g.addPlayer(names[i], std::move(r));
                        }
                        turnStarted=false; selectedIndex=-1; logScroll=0;
                        rebuildButtons();
                        state = UIState::Playing;
                    }
                }
                if(ev.type==sf::Event::TextEntered){
                    // קלט טקסט לשמות (ASCII פשוט)
                    if(ev.text.unicode>=32 && ev.text.unicode<127){
                        for(int i=0;i<desiredPlayers;++i) if(nameBoxes[i].focused){
                            nameBoxes[i].text.push_back( static_cast<char>(ev.text.unicode) );
                            nameBoxes[i].setString(nameBoxes[i].text, fontOK?&font:nullptr);
                        }
                    }else if(ev.text.unicode==8){ // Backspace
                        for(int i=0;i<desiredPlayers;++i) if(nameBoxes[i].focused){
                            if(!nameBoxes[i].text.empty()) nameBoxes[i].text.pop_back();
                            nameBoxes[i].setString(nameBoxes[i].text, fontOK?&font:nullptr);
                        }
                    }
                }
            }
            continue;
        }

        // ---------- מצב משחק פעיל ----------
        sf::Event ev;
        while (win.pollEvent(ev))
        {
            if (ev.type == sf::Event::Closed) { win.close(); }

            if (ev.type == sf::Event::MouseButtonPressed)
            {
                auto mp = win.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y});

                // בחירת יעד מהרשימה
                float y0 = 20.f;
                for (size_t i = 0; i < g.players_.size(); ++i) {
                    auto& up = g.players_[i];
                    if (!up->alive) continue;
                    if (sf::FloatRect(20, y0, 380, 34).contains(mp)) selectedIndex = (int)i;
                    y0 += 40.f;
                }

                // לחיצה על כפתורים
                for (auto &b : buttons) {
                    if (!b.box.getGlobalBounds().contains(mp) || !b.enabled) continue;

                    // אוטו-סיום תור: אם אין פעולות ולֹא חייבים coup עכשיו — נסיים תור
                    auto autoEnd = [&](){
                        if (!turnStarted) return;
                        Player &P = g.current();
                        bool mustCoupNow = P.forcedCoupFirstAction && P.coins >= 7;
                        if (P.actions <= 0 && !mustCoupNow) {
                            g.endTurn();
                            turnStarted = false;
                            selectedIndex = -1;
                        }
                    };

                    // פעולות עם חלון חסימה
                    if (b.action.has_value() &&
                        (*b.action == Action::Tax || *b.action == Action::Bribe || *b.action == Action::Coup))
                    {
                        if (*b.action == Action::Tax) {
                            runBlockSequenceThen(Action::Tax,   [&]{ g.tax();   });
                            autoEnd();
                        } else if (*b.action == Action::Bribe) {
                            runBlockSequenceThen(Action::Bribe, [&]{
                                g.bribe();
                                // נבחר יעד ראשון חוקי ל-Coup כדי שהכפתור יהיה ירוק מיד
                                for (size_t i=0; i<g.players_.size(); ++i) {
                                    if ((int)i != g.turnIndex && g.players_[i]->alive) { selectedIndex = (int)i; break; }
                                }
                            });
                            autoEnd();
                        } else { // Coup
                            if (selectedIndex >= 0) {
                                auto* t = g.players_[selectedIndex].get();
                                runBlockSequenceThen(Action::Coup,  [&]{ g.coup(*t); });
                                autoEnd();
                            }
                        }
                    } else {
                        // שאר הכפתורים (start/end/spy/jam/save log וכו’)
                        b.onClick();
                        autoEnd();
                    }
                }
            }

            // גלילת לוג
            if (ev.type == sf::Event::MouseWheelScrolled) {
                auto mp = win.mapPixelToCoords({ev.mouseWheelScroll.x, ev.mouseWheelScroll.y});
                if (logBounds.contains(mp)) {
                    int total = (int)g.log.entries().size();
                    int maxScroll = std::max(0, total - LOG_LINES);
                    logScroll -= (int)ev.mouseWheelScroll.delta;
                    if (logScroll < 0) logScroll = 0;
                    if (logScroll > maxScroll) logScroll = maxScroll;
                }
            }
        }



        // עדכון צבעים/זמינות
        updateButtons();

        // ציור: רשימת שחקנים (כולל role)
        win.clear(sf::Color(20,20,30));
        float y0=20; 
        for(size_t i=0;i<g.players_.size();++i){ 
            auto& up=g.players_[i]; if(!up->alive) continue;
            sf::RectangleShape row({380.f,34.f}); row.setPosition(20,y0);
            row.setFillColor((int)i==selectedIndex?sf::Color(70,70,90):sf::Color(40,40,60));

            std::string role = up->role?up->role->name():"?";
            auto t=makeText(up->name+" ("+role+") - coins: "+std::to_string(up->coins)+(i==g.turnIndex?"  (TURN)":"") ,16, fontOK?&font:nullptr); 
            t.setPosition(30,y0+6);

            // שחקן שבתורו עכשיו — מסגרת ירוקה
            if ((int)i == g.turnIndex) {
                row.setOutlineThickness(3.f);
                row.setOutlineColor(sf::Color(80,160,80));
            } else {
                row.setOutlineThickness(0.f);
            }

            win.draw(row);
            win.draw(t);

            // אם השחקן תחת סנקציה – תג אדום "SANCTIONED"
            if (up->sanctionActive) {
                auto badge = makeText("SANCTIONED", 14, fontOK?&font:nullptr);
                badge.setFillColor(sf::Color(200, 40, 40));
                badge.setPosition(320, y0 + 6);
                win.draw(badge);
            }
            // אם היעד תחת Jam — תג צהוב "JAMMED"
            if (up->jamActive) {
                auto badge2 = makeText("JAMMED", 14, fontOK?&font:nullptr);
                badge2.setFillColor(sf::Color(200, 200, 40));
                float dy = up->sanctionActive ? 24.f : 0.f;
                badge2.setPosition(320, y0 + 6 + dy);
                win.draw(badge2);
            }

            y0+=40;
        }

        // כפתורים
        for(auto& b:buttons){ win.draw(b.box); win.draw(b.label); }

        // רמז לבחירת יעד
        auto hint = makeText(selectedIndex>=0 ? ("Target: "+g.players_[selectedIndex]->name)
                                              : "Select a target (click a player)", 
                             16, fontOK?&font:nullptr);
        hint.setPosition(420, 260); win.draw(hint);

        // שורת סטטוס + תור נוכחי
        std::string status = g.isDraw()? "Status: Draw" : (g.singleWinner? ("Winner: "+*g.singleWinner) : (turnStarted? "Status: Turn open" : "Status: Waiting to start turn"));
        auto tStatus = makeText(status,18, fontOK?&font:nullptr); tStatus.setPosition(20,660); win.draw(tStatus);
        auto tTurn   = makeText("Turn: "+g.turn(),20, fontOK?&font:nullptr); tTurn.setPosition(20,630); win.draw(tTurn);

        // פנל לוג גלול
        sf::RectangleShape panel({logBounds.width, logBounds.height}); 
        panel.setPosition({logBounds.left, logBounds.top}); 
        panel.setFillColor(sf::Color(30, 30, 45)); 
        win.draw(panel);
        int total = (int)g.log.entries().size();
        int start = std::max(0, total - LOG_LINES - logScroll);
        int end   = std::min(total, start + LOG_LINES);
        float y = logBounds.top + 10.f;
        for (int i = start; i < end; ++i) {
            auto t = makeText(g.log.entries()[i], 14, fontOK?&font:nullptr);
            t.setPosition(logBounds.left + 10.f, y);
            win.draw(t);
            y += 18.f;
        }

        // ----- Overlay סוף משחק -----
        if (g.singleWinner.has_value() || g.isDraw()) {
            sf::RectangleShape overlay({1000.f, 700.f});
            overlay.setFillColor(sf::Color(0,0,0,180));
            win.draw(overlay);

            std::string msg = g.singleWinner ? ("Congratulations, " + *g.singleWinner + "! You Win!")
                                             : "Draw! Everyone still alive wins.";
            auto m = makeText(msg, 36, fontOK?&font:nullptr);
            m.setPosition(140, 220);
            win.draw(m);

            auto ok = makeText("[  OK  ]", 28, fontOK?&font:nullptr);
            ok.setPosition(430, 320);
            win.draw(ok);

            win.display();

            bool done=false;
            while (!done && win.isOpen()) {
                sf::Event e;
                while (win.pollEvent(e)) {
                    if (e.type == sf::Event::Closed) { win.close(); return 0; }
                    if (e.type == sf::Event::MouseButtonPressed || e.type == sf::Event::KeyPressed)
                        done = true;
                }
                sf::sleep(sf::milliseconds(10));
            }
            return 0;
        }

        win.display();
    }

    return 0;
}
