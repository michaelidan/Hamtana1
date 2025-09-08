// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include <vector>
#include <string>
namespace coup { class EventLog{ std::vector<std::string> entries_; public: void add(const std::string& s){ entries_.push_back(s); } const std::vector<std::string>& entries() const { return entries_; } }; }