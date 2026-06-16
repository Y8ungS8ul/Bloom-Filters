#ifndef HASH_FUNCTIONS_HPP
#define HASH_FUNCTIONS_HPP

#include <string>
#include <cstdint>

/**
 * @file HashFunctions.hpp
 * @brief Коллекция хеш-функций для использования в вероятностных структурах данных
 * 
 * Содержит реализации различных хеш-функций:
 * - Простые полиномиальные хеши (hashFunction1, hashFunction2)
 * - Криптографические хеши (SHA256, MD5)
 * - Высокопроизводительный MurmurHash3
 */

/**
 * @brief Простая полиномиальная хеш-функция №1
 * 
 * @param str Входная строка
 * @return size_t Хеш-значение
 * 
 * @details Использует полином с основанием 31.
 *          Простая и быстрая, но может давать много коллизий.
 * @note Сложность O(n), где n - длина строки
 */
size_t hashFunction1(const std::string& str);

/**
 * @brief Простая полиномиальная хеш-функция №2
 * 
 * @param str Входная строка
 * @return size_t Хеш-значение
 * 
 * @details Использует полином с основанием 37.
 *          Отличается от hashFunction1 для уменьшения коллизий.
 * @note Сложность O(n), где n - длина строки
 */
size_t hashFunction2(const std::string& str);

/**
 * @brief Хеш-функция на основе SHA256
 * 
 * @param str Входная строка
 * @return size_t Хеш-значение
 * 
 * @details Вычисляет SHA256 от строки и преобразует результат в size_t.
 *          Криптографически безопасна, но медленная.
 * @warning Преобразование SHA256 в size_t может потерять часть энтропии
 */
size_t hashSHA256(const std::string& str);

/**
 * @brief Хеш-функция на основе MD5
 * 
 * @param str Входная строка
 * @return size_t Хеш-значение
 * 
 * @details Вычисляет MD5 от строки и преобразует результат в size_t.
 *          Быстрее SHA256, но менее безопасна.
 * @warning Не использовать для криптографических целей
 */
size_t hashMD5(const std::string& str);

/**
 * @brief Реализация MurmurHash3 (32-битная версия)
 * 
 * @param str Входная строка
 * @param len Длина строки
 * @param seed Начальное значение (seed)
 * @return uint32_t 32-битное хеш-значение
 * 
 * @details Высокопроизводительный некриптографический хеш.
 *          Хорошее распределение и мало коллизий.
 * @see https://github.com/aappleby/smhasher
 */
uint32_t murmur3_32(const std::string& str, uint32_t len, uint32_t seed);

/**
 * @brief Адаптированная версия MurmurHash для BloomFilter
 * 
 * @param str Входная строка
 * @return size_t Хеш-значение
 * 
 * @details Обертка над murmur3_32 с фиксированным seed = 0.
 *          Оптимальна для использования в фильтрах Блума.
 * @note Рекомендуется для большинства случаев использования
 */
size_t murmurHash(const std::string& str);

#endif // HASH_FUNCTIONS_HPP