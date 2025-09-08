// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include <vector>
namespace coup { class Player;
struct AuthorityLists { std::vector<Player*> governors, judges, generals; void clear(){ governors.clear(); judges.clear(); generals.clear(); } };
}