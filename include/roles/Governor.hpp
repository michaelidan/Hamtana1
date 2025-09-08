// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include "Role.hpp"
namespace coup { class Governor: public Role{ public: std::string name() const override { return "Governor"; }
int taxBonus() const override { return 1; } bool canBlockTax() const override { return true; } }; }