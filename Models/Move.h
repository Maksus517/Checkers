#pragma once
#include <stdlib.h>

typedef int8_t POS_T;  // Определение типа для хранения координат на доске

struct move_pos
{
    POS_T x, y;             // Начальные координаты
    POS_T x2, y2;           // Конечная координаты
    POS_T xb = -1, yb = -1; // Координаты побитой фигуры

    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
        // Конструктор для инициализации хода без побитой фигуры
    }
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
            : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
        // Конструктор для инициализации хода с побитой фигурой
    }

    bool operator==(const move_pos &other) const
    {
        // Оператор сравнения для проверки равенства двух ходов
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    bool operator!=(const move_pos &other) const
    {
        // Оператор сравнения для проверки неравенства двух ходов
        return !(*this == other);
    }
};
