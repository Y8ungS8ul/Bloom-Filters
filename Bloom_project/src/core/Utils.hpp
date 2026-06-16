#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <unordered_set>

/**
 * @file Utils.hpp
 * @brief Коллекция вспомогательных утилитарных функций
 * 
 * Содержит различные вспомогательные функции:
 * - Генерация случайных символов и строк
 * - Подсчет уникальных элементов
 * - Преобразование контейнеров в строки
 */

/**
 * @brief Генерирует случайную букву латинского алфавита (a-z)
 * 
 * @return char Случайная буква в нижнем регистре
 * 
 * @details Использует функцию rand() для генерации чисел в диапазоне ['a', 'z']
 * @warning Требует предварительной инициализации srand()
 */
char randomLetter();

/**
 * @brief Подсчитывает количество уникальных элементов в строке CSV
 * 
 * @param csvLine Строка в формате CSV (значения разделены запятыми)
 * @return size_t Количество уникальных элементов
 * 
 * @details Разбивает строку по запятым и вставляет элементы в unordered_set
 * @note Сложность O(n), где n - количество элементов в строке
 * 
 * @example
 * std::string csv = "apple,banana,apple,orange";
 * size_t unique = countUniqueElementsInCSVLine(csv); // Результат: 3
 */
size_t countUniqueElementsInCSVLine(const std::string& csvLine);

/**
 * @brief Подсчитывает количество уникальных слов в текстовом файле
 * 
 * @param filePath Путь к текстовому файлу
 * @return size_t Количество уникальных слов
 * 
 * @details Читает файл построчно, разбивает на слова, приводит к нижнему регистру
 * @warning Возвращает 0, если файл не удалось открыть
 * @note Сложность O(n), где n - количество слов в файле
 */
size_t countUniqueWordsInTextFile(const std::string& filePath);

/**
 * @brief Генерирует случайное целое число в заданном диапазоне
 * 
 * @param min Минимальное значение (включительно)
 * @param max Максимальное значение (включительно)
 * @return int Случайное число в диапазоне [min, max]
 * 
 * @details Использует std::default_random_engine и std::uniform_int_distribution
 * @note Более качественная генерация, чем rand()
 * @warning Может быть медленнее rand()
 */
int randomNumber(int min, int max);

/**
 * @brief Генерирует случайное целое число в заданном диапазоне (упрощенная версия)
 * 
 * @param min Минимальное значение (включительно)
 * @param max Максимальное значение (включительно)
 * @return int Случайное число в диапазоне [min, max]
 * 
 * @details Использует rand() для простоты и скорости
 * @warning Требует предварительной инициализации srand()
 * @note Может давать менее равномерное распределение, чем randomNumber()
 */
int generateRandomNumber(int min, int max);

/**
 * @brief Генерирует случайную строку заданной длины
 * 
 * @param length Длина генерируемой строки
 * @return std::string Случайная строка из латинских букв и цифр
 * 
 * @details Использует символы: a-z, A-Z, 0-9
 * @warning Требует предварительной инициализации srand()
 * 
 * @example
 * std::string str = generateRandomString(10); // Например: "aB3dE9fGh1"
 */
std::string generateRandomString(int length);

/**
 * @brief Преобразует вектор в строку с разделителями
 * 
 * @tparam T Тип элементов вектора (должен поддерживать operator<<)
 * @param vec Вектор элементов
 * @return std::string Строковое представление вектора
 * 
 * @details Элементы разделяются пробелами
 * 
 * @example
 * std::vector<int> vec = {1, 2, 3};
 * std::string str = vectorToString(vec); // Результат: "1 2 3"
 */
template <typename T>
std::string vectorToString(const std::vector<T>& vec) {
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            ss << " ";  // Добавляем пробел перед каждым элементом, кроме первого
        }
        ss << vec[i];   // Добавляем элемент вектора в строку
    }
    return ss.str();
}

#endif // UTILS_HPP