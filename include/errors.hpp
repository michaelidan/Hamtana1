// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include <stdexcept>
#include <string>
namespace coup {
struct GameError : public std::runtime_error { using std::runtime_error::runtime_error; };
namespace err {
    inline const std::string not_enough_coins(const std::string& what){ return "אין מספיק מטבעות ל-" + what; }
    inline const std::string target_eliminated(){ return "לא ניתן לבחור יעד מודח"; }
    inline const std::string self_coup(){ return "אי אפשר לבצע coup על עצמך"; }
    inline const std::string gather_under_sanction(){ return "אסור לבצע gather תחת Sanction"; }
    inline const std::string tax_under_sanction(){ return "אסור לבצע tax תחת Sanction"; }
    inline const std::string arrest_zero(){ return "אי אפשר לעצור יעד עם 0 מטבעות"; }
    inline const std::string arrest_streak(){ return "אסור לבצע arrest על אותו יעד ברצף"; }
    inline const std::string arrest_jam(){ return "אסור לבצע arrest בתור זה (JamArrest)"; }
    inline const std::string bribe_under_coup(){ return "אסור bribe תחת חובת coup (נדרשים 11+)"; }
    inline const std::string skip_forbidden(){ return "אסור לדלג כשחייבים coup"; }
    inline const std::string winner_not_ready(){ return "המשחק עדיין פעיל"; }
    inline const std::string draw_no_single_winner(){ return "המשחק הסתיים בתיקו – אין מנצח יחיד"; }
}}