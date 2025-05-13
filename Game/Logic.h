﻿#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    Logic(Board *board, Config *config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine (
                !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    // Основная функция для поиска лучших ходов для заданного цвета
    vector<move_pos> find_best_turns(const bool color) {
        next_move.clear(); // очищаем вектор для хранения следующего хода
        next_best_state.clear(); // очищаем вектор для хранения следующего состояния

        // Ищем лучший первый ход, передавая текущую доску, цвет
        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        vector<move_pos> res; // итоговый вектор ходов
        int state = 0; // начинаем с начального состояния (0)

        // Проходим по цепочке выбранных ходов до тех пор, пока не достигнем конца
        do {
            res.push_back(next_move[state]); // добавляем текущий ход в результат
            state = next_best_state[state]; // переходим к следующему состоянию
        } while (state != -1 && next_move[state].x != -1); // условие завершения (нет следующего хода или нет ударов)

        return res; // возвращаем последовательность ходов
    }



private:

    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
                                double alpha = -1) {
        next_move.emplace_back(-1, -1, -1, -1); // добавляем новый элемент в вектор возможных ходов, инициализированный значениями по умолчанию
        next_best_state.emplace_back(-1); // добавляем новое состояние, пока что -1 (конец цепочки)

        // Если состояние не равно 0, значит нужно искать доступные ходы для фигуры
        if (state != 0) {
            find_turns(x, y, mtx); // ищем допустимые ходы для текущей фигуры
        }

        auto now_turns = turns; // копируем текущие возможные ходы
        auto now_have_beats = beats; // копируем информацию о наличии ударов (битвах)

        // Если бить нельзя и мы не в начале цепочки
        if (!now_have_beats && state != 0) {
            // вызываем рекурсию для другого цвета, без продолжения серии
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        double best_score = -1; // инициализация лучшего результата (максимума)

        // Перебираем все возможные ходы
        for (auto turn : now_turns) {
            size_t new_state = next_move.size(); // индекс нового состояния
            double score;
            if (now_have_beats) { // если есть возможность бить
                // рекурсивно ищем лучший результат, продолжая серию ударов
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);
            } else {
                // если бить нельзя, переходим к следующему ходу другого цвета
                score = find_first_best_turn(make_turn(mtx, turn), 1 - color, 0, best_score);
            }

            // если найден лучший результат
            if (score > best_score) {
                best_score = score; // обновляем лучший результат
                next_move[state] = turn; // запоминаем ход
                // если был удар, обновляем состояние; иначе - конец цепочки
                next_best_state[state] = (now_have_beats ? new_state : -1);
            }
        }
        return best_score; // возвращаем лучший найденный результат
    }

    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
                               double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
        // Если достигнута максимальная глубина поиска
        if (depth == Max_depth) {
            return calc_score(mtx, (depth % 2 == color)); // оцениваем текущую доску
        }

        // Если есть серия ударов (x,y заданы)
        if (x != -1) {
            find_turns(x, y, mtx); // ищем ходы для фигуры в позиции (x,y)
        } else {
            find_turns(color, mtx); // ищем ходы для текущего цвета
        }

        auto now_turns = turns; // копируем возможные ходы
        auto now_have_beats = heve_beats; // копируем информацию о наличии ударов

        // Если ударов сделать нельзя и есть серия ударов
        if (!now_have_beats && x != -1) {
            // рекурсия для другого цвета и увеличенной глубины
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }

        // Если ходов нет, то текущий игрок проиграл или ничья
        if (turns.empty()) {
            return (depth % 2 ? 0 : INF); // 0 если ходит противник, INF если свой ход
        }

        double min_score = INF + 1; // минимальный возможный результат
        double max_score = -1; // максимальный возможный результат

        // Перебираем все возможные ходы
        for (auto turn : now_turns) {
            double score;
            if (now_have_beats) { // если есть удар
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            } else {
                // если ударов нет, переходим к следующему ходу другого игрока
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }

            // обновляем минимальный и максимальный результаты
            min_score = min(min_score, score);
            max_score = max(max_score, score);

            if (depth % 2) { // если ходит текущий игрок (максимизатор)
                alpha = max(alpha, max_score); // обновляем левую границу
            } else { // противник (минимизатор)
                beta = min(beta, min_score); // обновляем правую границу
            }

            // если отсечение по альфа-бета
            if (optimization != 'O0' && alpha > beta) {
                break; // выходим из цикла
            }
            // при равенстве границ, можем вернуть приближённое значение
            if (optimization == 'O2' && alpha == beta) {
                return (depth % 2 ? max_score + 1 : min_score - 1);
            }
        }

        // возвращаем результат в зависимости от текущей глубины
        return (depth % 2 ? max_score : min_score);
    }

    //выполняет ход на копии доски и возвращает доску после хода
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }
    // оценивает состояние доски для бота
    double calc_score(const vector<vector<POS_T>> &mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1); // всего белых пешек
                wq += (mtx[i][j] == 3); //      белых королев
                b += (mtx[i][j] == 2); //       черных пешек
                bq += (mtx[i][j] == 4); //      черных королев
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4; // вес королевы
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

public:
    // ищет возможные ходы для указанного цвета
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }
    // ищет возможные ходы для указанной клетки
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    //основной метод для поиска возможных ходов на доске
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }
    // ищет возможные ходы для указанной фигуры на переданной доске
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear();
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
            case 1:
            case 2:
                // check pieces
                for (POS_T i = x - 2; i <= x + 2; i += 4)
                {
                    for (POS_T j = y - 2; j <= y + 2; j += 4)
                    {
                        if (i < 0 || i > 7 || j < 0 || j > 7)
                            continue;
                        POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                        if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                            continue;
                        turns.emplace_back(x, y, i, j, xb, yb);
                    }
                }
                break;
            default:
                // check queens
                for (POS_T i = -1; i <= 1; i += 2)
                {
                    for (POS_T j = -1; j <= 1; j += 2)
                    {
                        POS_T xb = -1, yb = -1;
                        for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                        {
                            if (mtx[i2][j2])
                            {
                                if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                                {
                                    break;
                                }
                                xb = i2;
                                yb = j2;
                            }
                            if (xb != -1 && xb != i2)
                            {
                                turns.emplace_back(x, y, i2, j2, xb, yb);
                            }
                        }
                    }
                }
                break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
            case 1:
            case 2:
                // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
            default:
                // check queens
                for (POS_T i = -1; i <= 1; i += 2)
                {
                    for (POS_T j = -1; j <= 1; j += 2)
                    {
                        for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                        {
                            if (mtx[i2][j2])
                                break;
                            turns.emplace_back(x, y, i2, j2);
                        }
                    }
                }
                break;
        }
    }

public:
    vector<move_pos> turns; //возможные ходы
    bool have_beats; // флаг обязательного взятия шашки
    int Max_depth; // максимальная глубина поиска ходов

private:
    default_random_engine rand_eng; // генератор случайных чисел
    string scoring_mode;
    string optimization; // оценка позиции бота
    vector<move_pos> next_move; // лучшие ходы
    vector<int> next_best_state; // состояние после выполненого хода
    Board *board; // указатель на Board
    Config *config; // указатель на Config
};
