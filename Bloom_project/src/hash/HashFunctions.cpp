#include "HashFunctions.hpp"
#include "../hash/sha256.h"
#include "../hash/md5.h"
#include <cstring>

/**
 * @brief Простая полиномиальная хеш-функция с основанием 31
 * 
 * @details Использует классический алгоритм хеширования строк:
 *          hash = hash * 31 + c
 *          31 выбрано как простое число для лучшего распределения
 */
size_t hashFunction1(const std::string& str) {
    size_t hash = 0;
    for (char c : str) {
        // Используем умножение на простое число и сложение с кодом символа
        hash = hash * 31 + static_cast<size_t>(c);
    }
    return hash;
}

/**
 * @brief Простая полиномиальная хеш-функция с основанием 37
 * 
 * @details Использует основание 37 для разнообразия с hashFunction1.
 *          Позволяет получить независимые хеш-значения от той же строки.
 */
size_t hashFunction2(const std::string& str) {
    size_t hash = 0;
    for (char c : str) {
        hash = hash * 37 + static_cast<size_t>(c);
    }
    return hash;
}

/**
 * @brief Хеш-функция на основе SHA256
 * 
 * @details Вычисляет SHA256 и преобразует результат в size_t.
 *          Использует простое преобразование для совместимости с BloomFilter.
 */
size_t hashSHA256(const std::string& str) {
    SHA256 sha256;
    std::string hashStr = sha256(str);
    
    size_t hash = 0;
    for (char c : hashStr) {
        // Используем простое число 271 для преобразования
        hash = hash * 271 + static_cast<size_t>(c);
    }
    return hash;
}

/**
 * @brief Хеш-функция на основе MD5
 * 
 * @details Вычисляет MD5 и преобразует результат в size_t.
 *          Быстрее SHA256, но с меньшей криптостойкостью.
 */
size_t hashMD5(const std::string& str) {
    MD5 md5;
    std::string hashStr = md5(str);
    
    size_t hash = 0;
    for (char c : hashStr) {
        // Используем простое число 131 для разнообразия с SHA256
        hash = hash * 131 + static_cast<size_t>(c);
    }
    return hash;
}

/**
 * @brief Реализация MurmurHash3 (32-битная)
 * 
 * @details MurmurHash - быстрый некриптографический хеш с хорошим распределением.
 *          Алгоритм:
 *          1. Обрабатывает строку блоками по 4 байта
 *          2. Применяет операции умножения и сдвига
 *          3. Финальное смешивание для улучшения распределения
 */
uint32_t murmur3_32(const std::string& str, uint32_t len, uint32_t seed) {
    const char* key = str.c_str();
    
    // Константы MurmurHash3
    static const uint32_t c1 = 0xcc9e2d51;
    static const uint32_t c2 = 0x1b873593;
    static const uint32_t r1 = 15;
    static const uint32_t r2 = 13;
    static const uint32_t m = 5;
    static const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;

    // Обработка 4-байтовых блоков
    const int nblocks = len / 4;
    const uint32_t* blocks = reinterpret_cast<const uint32_t*>(key);
    
    for (int i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    // Обработка оставшихся байтов
    const uint8_t* tail = reinterpret_cast<const uint8_t*>(key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
    case 3:
        k1 ^= static_cast<uint32_t>(tail[2]) << 16;
        [[fallthrough]];
    case 2:
        k1 ^= static_cast<uint32_t>(tail[1]) << 8;
        [[fallthrough]];
    case 1:
        k1 ^= static_cast<uint32_t>(tail[0]);

        k1 *= c1;
        k1 = (k1 << r1) | (k1 >> (32 - r1));
        k1 *= c2;
        hash ^= k1;
    }

    // Финальное смешивание
    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

/**
 * @brief Адаптированная версия MurmurHash для BloomFilter
 * 
 * @details Обертка над murmur3_32 с фиксированным seed = 0.
 *          Результат приводится к size_t для совместимости.
 */
size_t murmurHash(const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.length());
    uint32_t seed = 0;  // Стандартный seed для фильтра Блума
    uint32_t hash = murmur3_32(str, len, seed);
    return static_cast<size_t>(hash);
}