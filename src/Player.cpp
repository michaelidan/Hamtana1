// =============================================
// Email: michael9090124@gmail.com
// קובץ זה שייך למטלה 3 — Coup (גרסת Michael)
// הערות בעברית בכל השורות החשובות להסבר הלוגיקה.
// =============================================
#include "Player.hpp"
#include "roles/Governor.hpp"
#include "roles/Judge.hpp"
#include "roles/General.hpp"
#include "roles/Baron.hpp"
#include "roles/Merchant.hpp"
#include "roles/Spy.hpp"
namespace coup {
bool Player::isGovernor() const { return dynamic_cast<Governor*>(role.get())!=nullptr; }
bool Player::isJudge()    const { return dynamic_cast<Judge*>(role.get())!=nullptr; }
bool Player::isGeneral()  const { return dynamic_cast<General*>(role.get())!=nullptr; }
bool Player::isBaron()    const { return dynamic_cast<Baron*>(role.get())!=nullptr; }
bool Player::isMerchant() const { return dynamic_cast<Merchant*>(role.get())!=nullptr; }
bool Player::isSpy()      const { return dynamic_cast<Spy*>(role.get())!=nullptr; }
}