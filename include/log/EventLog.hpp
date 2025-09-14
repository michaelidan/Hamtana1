// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

namespace coup {
    class EventLog {
        std::vector<std::string> entries_;
    public:
        // הוספת שורה ליומן
        void add(const std::string& s) { entries_.push_back(s); }
        // גישה לקריאה ליומן
        const std::vector<std::string>& entries() const { return entries_; }

        // שמירה לקובץ (יוצר תיקייה אם צריך)
        bool saveToFile(const std::string& path) const {
            try {
                std::filesystem::path p(path);
                if (!p.parent_path().empty())
                    std::filesystem::create_directories(p.parent_path());
                std::ofstream out(path, std::ios::trunc);
                if (!out) return false;
                for (const auto& s : entries_) out << s << '\n';
                return true;
            } catch (...) {
                return false;
            }
        }
    };
}
