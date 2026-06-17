#include "Utils.hpp"
#include <random>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <ctime>

/**
 * @brief Генерирует случайную букву латинского алфавита
 * 
 * @details Использует ASCII коды: 'a' = 97, 'z' = 122
 *          Диапазон: 26 букв
 */
char randomLetter() {
    const int a = static_cast<int>('a');
    const int z = static_cast<int>('z');
    return static_cast<char>(a + rand() % (z - a + 1));
}

/**
 * @brief Подсчет уникальных элементов в CSV строке
 * 
 * @details Алгоритм:
 *          1. Разбивает строку по запятым используя std::getline
 *          2. Каждый элемент вставляет в unordered_set
 *          3. Возвращает размер set'а
 */
size_t countUniqueElementsInCSVLine(const std::string& csvLine) {
    std::unordered_set<std::string> uniqueElements;
    std::stringstream ss(csvLine);
    std::string item;

    while (std::getline(ss, item, ',')) {
        uniqueElements.insert(item);
    }

    return uniqueElements.size();
}

/**
 * @brief Подсчет уникальных слов в текстовом файле
 * 
 * @details Алгоритм:
 *          1. Открывает файл по указанному пути
 *          2. Читает файл построчно
 *          3. Разбивает каждую строку на слова
 *          4. Приводит слова к нижнему регистру
 *          5. Вставляет в unordered_set
 *          6. Возвращает количество уникальных слов
 */
size_t countUniqueWordsInTextFile(const std::string& filePath) {
    std::unordered_set<std::string> uniqueWords;
    std::ifstream file(filePath);
    std::string line;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                // Приводим к нижнему регистру для case-insensitive подсчета
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                uniqueWords.insert(word);
            }
        }
        file.close();
    } else {
        std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
        return 0;
    }

    return uniqueWords.size();
}

/**
 * @brief Генерация случайного числа с использованием <random>
 * 
 * @details Использует std::default_random_engine и std::uniform_int_distribution
 *          Статический генератор инициализируется при первом вызове
 */
int randomNumber(int min, int max) {
    // Статический генератор инициализируется один раз
    static std::default_random_engine generator(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

/**
 * @brief Упрощенная генерация случайного числа через rand()
 * 
 * @details Использует стандартный rand() для простоты
 *          Требует предварительного вызова srand()
 */
int generateRandomNumber(int min, int max) {
    return rand() % (max - min + 1) + min;
}

/**
 * @brief Генерация случайной строки
 * 
 * @details Алгоритм:
 *          1. Определяет набор допустимых символов
 *          2. Для каждого символа генерирует случайный индекс
 *          3. Добавляет символ в результат
 * 
 * @note Набор символов включает:
 *       - Латинские буквы в нижнем регистре (26)
 *       - Латинские буквы в верхнем регистре (26)
 *       - Цифры (10)
 *       Всего: 62 возможных символа
 */
std::string generateRandomString(int length) {
    std::string result;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const int charsetSize = sizeof(charset) - 1;  // -1 для '\0'
    
    for (int i = 0; i < length; ++i) {
        result += charset[generateRandomNumber(0, charsetSize - 1)];
    }
    
    return result;
}