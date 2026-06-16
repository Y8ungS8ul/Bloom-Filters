#include "CollisionAnalyzer.hpp"
#include "../core/BloomFilter.hpp"
#include "../hash/HashFunctions.hpp"
#include <fstream>
#include <iostream>

/**
 * @brief Вычисление относительных коллизий для разных конфигураций
 * 
 * @details Детальная реализация:
 * 
 * Конфигурации хеш-функций:
 * - 1 ХФ: только murmurHash
 * - 2 ХФ: murmurHash + hashFunction1
 * - 3 ХФ: murmurHash + hashFunction1 + hashFunction2
 * - 4 ХФ: murmurHash + hashFunction1 + hashFunction2 + hashSHA256
 * - 5 ХФ: все 5 хеш-функций
 * 
 * @note Размер фильтра фиксирован для всех конфигураций
 */
std::map<double, double> calculateCollisions(size_t filterSize) {
    std::map<double, double> collisionsMap;

    // Создание 5 фильтров Блума с разным числом хеш-функций
    for (int numHashFunctions = 1; numHashFunctions <= 5; ++numHashFunctions) {
        BloomFilter<std::string> filter(filterSize);

        // Добавление хеш-функций в фильтр в зависимости от их количества
        for (int i = 0; i < numHashFunctions; ++i) {
            if (i == 0) {
                filter.addHashFunction(murmurHash);
            } else if (i == 1) {
                filter.addHashFunction(hashFunction1);
            } else if (i == 2) {
                filter.addHashFunction(hashFunction2);
            } else if (i == 3) {
                filter.addHashFunction(hashSHA256);
            } else {
                filter.addHashFunction(hashMD5);
            }
        }

        // Считывание данных из CSV-файла
        std::ifstream file("1000_SalesRecords.csv");
        if (!file.is_open()) {
            std::cerr << "Unable to open file: 1000_SalesRecords.csv" << std::endl;
            return collisionsMap;  // Возвращаем пустой словарь при ошибке
        }

        // Вставка всех строк из файла в фильтр
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            filter.insert(line);
            lineCount++;
        }
        file.close();

        // Вычисление количества коллизий
        double collisions = filter.countCollisions();
        collisionsMap.insert({static_cast<double>(numHashFunctions), collisions});
        
        // Отладочный вывод (можно закомментировать)
        std::cout << "Обработано " << lineCount << " строк для " 
                  << numHashFunctions << " хеш-функций" << std::endl;
        std::cout << "Коллизий: " << collisions << std::endl;
    }

    return collisionsMap;
}

/**
 * @brief Вычисление абсолютных коллизий для разных конфигураций
 * 
 * @details Особенности реализации:
 * - Хеш-функции добавляются каскадно (накапливаются)
 * - Использует меньший файл данных (100 строк) для ускорения
 * - Подсчитывает полные совпадения наборов индексов
 * 
 * Конфигурации:
 * - 1 ХФ: только murmurHash
 * - 2 ХФ: murmurHash + hashFunction1
 * - 3 ХФ: murmurHash + hashFunction1 + hashFunction2
 * - 4 ХФ: murmurHash + hashFunction1 + hashFunction2 + hashSHA256
 * - 5 ХФ: все 5 хеш-функций
 */
std::map<double, double> calculateCollisionsABS(size_t filterSize) {
    std::map<double, double> collisionsMap;

    // Создание 5 фильтров Блума с разным числом хеш-функций
    for (int numHashFunctions = 1; numHashFunctions <= 5; ++numHashFunctions) {
        BloomFilter<std::string> filter(filterSize);

        // Каскадное добавление хеш-функций
        for (int i = 0; i < numHashFunctions; ++i) {
            if (i == 0) {
                filter.addHashFunction(murmurHash);
            } else if (i == 1) {
                filter.addHashFunction(hashFunction1);
            } else if (i == 2) {
                filter.addHashFunction(hashFunction2);
            } else if (i == 3) {
                filter.addHashFunction(hashSHA256);
            } else if (i == 4) {
                filter.addHashFunction(hashMD5);
            }
        }

        // Считывание данных из файла (меньший файл для ускорения)
        std::ifstream file("build/100_SalesRecords.csv");
        if (!file.is_open()) {
            std::cerr << "Unable to open file: 100_SalesRecords.csv" << std::endl;
            return collisionsMap;
        }

        // Вставка строк в фильтр
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            filter.insert(line);
            lineCount++;
        }
        file.close();

        // Подсчет абсолютных коллизий
        double collisions = filter.retFullCollisions();
        collisionsMap.insert({static_cast<double>(numHashFunctions), collisions});
        
        std::cout << "Обработано " << lineCount << " строк для абсолютных коллизий ("
                  << numHashFunctions << " ХФ)" << std::endl;
        std::cout << "Абсолютных коллизий: " << collisions << std::endl;
    }

    return collisionsMap;
}

/**
 * @brief Анализ зависимости коллизий от размера фильтра
 * 
 * @details Исследует, как размер битового массива влияет на количество коллизий.
 * 
 * Параметры анализа:
 * - Начальный размер: 1000
 * - Конечный размер: 30000
 * - Шаг увеличения: 5000
 * - Количество хеш-функций: фиксировано (обычно 3)
 * - Файл данных: "1000_SalesRecords.csv"
 * 
 * @note Формула для оптимального размера фильтра: m = -n * ln(P) / (ln(2))^2
 *       где n = количество элементов, P = желаемая вероятность ложного срабатывания
 */
std::map<int, int> calculateCollisions_size(int hashfunctions_count) {
    std::map<int, int> collisionsMap;

    // Перебираем различные размеры фильтра
    size_t size_f = 1000;
    while (size_f < 30000) {
        // Создаем фильтр с текущим размером
        BloomFilter<std::string> filter(size_f);

        // Добавляем фиксированный набор хеш-функций
        filter.addHashFunction(hashFunction1);
        filter.addHashFunction(hashFunction2);
        filter.addHashFunction(murmurHash);
        
        // При необходимости добавляем дополнительные хеш-функции
        if (hashfunctions_count > 3) {
            filter.addHashFunction(hashSHA256);
        }
        if (hashfunctions_count > 4) {
            filter.addHashFunction(hashMD5);
        }

        // Считывание данных из CSV-файла
        std::ifstream file("build/100_SalesRecords.csv");
        if (!file.is_open()) {
            std::cerr << "Unable to open file: 100_SalesRecords.csv" << std::endl;
            return collisionsMap;
        }

        // Вставка данных в фильтр
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            filter.insert(line);
            lineCount++;
        }
        file.close();

        // Подсчет коллизий
        int collisions = filter.countCollisions();
        collisionsMap.insert({static_cast<int>(size_f), collisions});
        
        std::cout << "Размер фильтра: " << size_f 
                  << ", обработано строк: " << lineCount
                  << ", коллизий: " << collisions << std::endl;
        
        // Увеличиваем размер для следующей итерации
        size_f += 5000;
    }
    return collisionsMap;
}