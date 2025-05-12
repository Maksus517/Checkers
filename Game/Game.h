#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // Старт игры
    int play()
    {
        auto start = chrono::steady_clock::now(); // Время начала игры для отслеживания длительности
        if (is_replay)
        {
            logic = Logic(&board, &config);  // Пересоздание логики для новой игры.
            config.reload();  // Перезагрузка конфигурации.
            board.redraw();   // Перерисовка игровой доски.
        }
        else
        {
            board.start_draw(); // Инициализация и отрисовка стартовой доски
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns");
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // Сброс серии ударов
            logic.find_turns(turn_num % 2); // Поиск возможных ходов для текущего игрока
            if (logic.turns.empty())
                break; // Выход из цикла, если больше нет доступных ходов
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ход игрока
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT)
                {
                    is_quit = true; // Завершение игры
                    break;
                }
                else if (resp == Response::REPLAY)
                {
                    is_replay = true; // Перезапуск игры
                    break;
                }
                else if (resp == Response::BACK)
                {
                    // Обработка возврата хода
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2); // Ход бота
        }
        auto end = chrono::steady_clock::now(); // Время окончания игры
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();

        if (is_replay)
            return play(); // Повторный запуск игры
        if (is_quit)
            return 0; // Завершение игры
        int res = 2;
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа одного из игроков
        }
        board.show_final(res); // Показ результата игры
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play(); // Повторный запуск игры
        }
        return res;
    }

private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // Время начала хода бота

        auto delay_ms = config("Bot", "BotDelayMS");
        thread th(SDL_Delay, delay_ms);
        auto turns = logic.find_best_turns(color); // Поиск ходов для бота
        th.join();
        bool is_first = true;

        // Выполнение ходов
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms); // Задержка между ходами, если это не первый ход
            }
            is_first = false;
            beat_series += (turn.xb != -1); // Увеличение серии ударов, если ход с побитием
            board.move_piece(turn, beat_series); // Перемещение фигуры на доске
        }

        auto end = chrono::steady_clock::now();  // Время окончания хода бота
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close(); // Запись времени хода бота в лог-файл
    }

    Response player_turn(const bool color)
    {
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y); // Добавление всех возможных начальных позиций в список
        }
        board.highlight_cells(cells); // Выделение возможных начальных позиций
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;
        while (true)
        {
            auto resp = hand.get_cell();
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp); // Если пользователь решил выйти или перезапустить игру
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;
            for (auto turn : logic.turns)
            {
                // Проверка корректности выбранной начальной позиции
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // Проверка правильности хода и установка текущего хода
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break; // Если ход корректен
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y); // Установка активной клетки
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2); // Добавление возможных конечных позиций
                }
            }
            board.highlight_cells(cells2); // Выделение возможных конечных позиций
        }
        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1); // Перемещение фигуры
        if (pos.xb == -1)
            return Response::OK;  // если ударов больше нет

        // Продолжение серии ударов
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2); // Поиск возможных продолжений серии ударов
            if (!logic.have_beats)
                break; // Если ударов больше нет

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2); // Добавление конечных позиций
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // Попытка сделать следующий ход
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp); // Если пользователь решил выйти или перезапустить игру
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn; // Установка текущего хода
                        break;
                    }
                }
                if (!is_correct)
                    continue; // Если ход некорректен

                board.clear_highlight();
                board.clear_active();
                beat_series += 1; // Увеличение серии ударов
                board.move_piece(pos, beat_series);
                break; // Если ход корректен
            }
        }

        return Response::OK;
    }

private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
