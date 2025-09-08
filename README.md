# Coup (מטלה 3) — גרסת Michael (v4)

פרויקט C++ מלא עם Makefile, דמו קונסולי, **GUI ב-SFML**, בדיקות (doctest), ומימוש מלא לפי המסמך הקנוני.
- **חסימה סדרתית ללא חלון זמן** — בדיוק כפי שהגדרת.
- **Bribe**: לא אוכל פעולה; מעניק פעולה רגילה נוספת **אחת**; אם נחסם — הכסף אבוד אך התור ממשיך; מחייב coup ראשון אחרי הצלחה.
- **Skip/Draw**: שני סבבים רצופים של כולם skip ⇒ תיקו.

## בנייה והרצה (קונסול)
```bash
make run      # בונה ומריץ דמו
make test     # מריץ בדיקות
make valgrind # בדיקת זיכרון
```

## GUI (SFML)
תלויות:
```bash
sudo apt update
sudo apt install -y g++ make libsfml-dev fonts-dejavu-core
```
הרצה:
```bash
make gui
```

ה-GUI מבצע חסימה **סדרתית**: עבור tax/bribe/coup מוצג דיאלוג לכל בעל סמכות לפי הסדר ושואל "לחסום?".
