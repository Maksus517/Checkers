#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands
class Hand
{
public:
    // Конструктор для взаимодействия с игровым полем
    Hand(Board *board) : board(board)
    {
    }

    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                    case SDL_QUIT:
                        resp = Response::QUIT; // Закрытие окна
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                        x = windowEvent.motion.x;
                        y = windowEvent.motion.y;
                        xc = int(y / (board->H / 10) - 1); // Вычисление позиции клетки по клику мыши
                        yc = int(x / (board->W / 10) - 1);
                        if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                        {
                            resp = Response::BACK; // Возврат хода
                        }
                        else if (xc == -1 && yc == 8)
                        {
                            resp = Response::REPLAY; // Перезапуска игры
                        }
                        else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                        {
                            resp = Response::CELL; // Выбор клетки
                        }
                        else
                        {
                            xc = -1;
                            yc = -1;
                        }
                        break;
                    case SDL_WINDOWEVENT:
                        if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                        {
                            board->reset_window_size(); // Изменение размера окна -> обновление размеров доски
                            break;
                        }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return {resp, xc, yc};
    }

    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                    case SDL_QUIT:
                        resp = Response::QUIT; // Закрытие окна
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        board->reset_window_size(); // Изменение размера окна -> обновление размеров доски
                        break;
                    case SDL_MOUSEBUTTONDOWN: {
                        int x = windowEvent.motion.x;
                        int y = windowEvent.motion.y;
                        int xc = int(y / (board->H / 10) - 1);
                        int yc = int(x / (board->W / 10) - 1);
                        if (xc == -1 && yc == 8)
                            resp = Response::REPLAY; // Перезапуск игры
                    }
                        break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

private:
    Board *board; // Указатель на объект Board для взаимодействия с игровым полем
};
