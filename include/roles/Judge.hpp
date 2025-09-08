// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include "Role.hpp"
namespace coup { class Judge: public Role{ public: std::string name() const override { return "Judge"; }
bool canBlockBribe() const override { return true; } }; }