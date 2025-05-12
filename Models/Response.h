#pragma once

enum class Response
{
    OK,      // Успешное действие
    BACK,    // Возврат хода
    REPLAY,  // Перезапуск игры
    QUIT,    // Выход из игры
    CELL     // Выбор клетки на доске
};
