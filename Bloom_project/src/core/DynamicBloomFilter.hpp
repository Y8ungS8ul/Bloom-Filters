#include <vector>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include "md5.h"
#include "sha256.h"

/**
 * Класс DynBloomFilter - Динамический фильтр Блума
 * 
 * Реализация вероятностной структуры данных "Динамический фильтр Блума"
 * (Dynamic Bloom Filter). Главная особенность - автоматический расчёт
 * оптимальных параметров (размер битового массива и количество хеш-функций)
 * на основе ожидаемого количества элементов и допустимой вероятности
 * ложноположительных срабатываний.
 * 
 * @tparam T - тип хранимых элементов (обычно std::string)
 * 
 * Отличительные особенности:
 * - Автоматический подбор размера и количества хеш-функций по формулам
 * - Поддержка различных хеш-функций (кольцевые, MurmurHash, SHA256, MD5)
 * - Статистика коллизий для анализа качества хеш-функций
 * 
 * Формулы расчёта:
 * - Оптимальный размер битового массива: m = -n * ln(P) / (ln(2))^2
 * - Оптимальное количество хеш-функций: k = ln(2) * m / n
 *   где n - ожидаемое количество элементов, P - допустимая вероятность ошибки
 */
template <typename T>
class DynBloomFilter
{
private:
    std::vector<bool> data;                           // Битовый массив фильтра
    std::vector<std::function<size_t(const T&)>> hashFunctions; // Вектор хеш-функций
    std::unordered_map<size_t, size_t> collisionMap;  // Для подсчёта коллизий
    std::unordered_map<T, std::vector<size_t>> elementIndices; // Для отслеживания индексов элементов
    
    int hash_f_counter;  // Количество хеш-функций (k)
    size_t size;         // Размер битового массива (m)

public:
    /**
     * Конструктор динамического фильтра Блума
     * Автоматически вычисляет оптимальные параметры на основе:
     * - ожидаемого количества элементов (numElements)
     * - допустимой вероятности ложноположительного срабатывания (falsePositiveRate)
     * 
     * @param numElements - ожидаемое количество элементов для хранения (n)
     * @param falsePositiveRate - допустимая вероятность ложноположительного срабатывания (P)
     * 
     * @note Если numElements отрицательный, используется его абсолютное значение
     * 
     * Алгоритм:
     * 1. Вычисляется оптимальный размер битового массива m
     * 2. Вычисляется оптимальное количество хеш-функций k
     * 3. Создаётся битовый массив размера m
     * 4. Добавляются k хеш-функций (комбинация различных алгоритмов)
     */
    DynBloomFilter(int numElements, double falsePositiveRate)
    {
        // Формула: m = -n * ln(P) / (ln(2))^2
        size_t m = static_cast<size_t>(numElements * log(falsePositiveRate) / (log(2) * log(2)));
        size = m;

        // Формула: k = ln(2) * m / n
        int k = static_cast<int>(log(static_cast<double>(2)) * m / (abs(static_cast<double>(numElements))));
        hash_f_counter = k;

        // Инициализируем битовый массив (все биты = 0)
        data.resize(m, 0);

        // Добавляем необходимое количество хеш-функций
        // Используем комбинацию различных алгоритмов для наилучшего распределения
        for (int i = 0; i < k; ++i) {
            if (i == 0) {
                // Первая хеш-функция - простая кольцевая (множитель 31)
                addHashFunction([this](const T& item) { return this->hashFunction1(item); });
            }
            else if (i == 1) {
                // Вторая хеш-функция - простая кольцевая (множитель 37)
                addHashFunction([this](const T& item) { return this->hashFunction2(item); });
            }
            else if (i == 2) {
                // Третья хеш-функция - качественный MurmurHash
                addHashFunction([this](const T& item) { return this->murmurHash(item); });
            }
            else if (i > 2) {
                // Для дополнительных хеш-функций используем модифицированный стандартный хеш
                // Добавляем уникальную константу к каждому хешу для получения независимых функций
                auto hashFunction = [i](const T& item) {
                    return std::hash<T>{}(item) + i;
                };
                addHashFunction(hashFunction);
            }
        }
    }

    /**
     * Простая кольцевая хеш-функция №1
     * Использует умножение на 31 (простое число) для хорошего распределения
     * 
     * @param str - входная строка
     * @return size_t - хеш-значение
     */
    size_t hashFunction1(const std::string& str) {
        size_t hash = 0;
        for (char c : str) {
            hash = hash * 31 + c;  // Умножение на простое число даёт хорошее распределение
        }
        return hash;
    }

    /**
     * Простая кольцевая хеш-функция №2
     * Использует умножение на 37 (другое простое число) для независимости от hashFunction1
     * 
     * @param str - входная строка
     * @return size_t - хеш-значение
     */
    size_t hashFunction2(const std::string& str) {
        size_t hash = 0;
        for (char c : str) {
            hash = hash * 37 + c;  // Другое простое число для независимости
        }
        return hash;
    }

    /**
     * Криптографическая хеш-функция SHA-256
     * Обеспечивает высокую криптостойкость и равномерное распределение
     * 
     * @param str - входная строка
     * @return size_t - свёрнутое хеш-значение (из 256-битного хеша)
     */
    size_t hashSHA256(const std::string& str) {
        SHA256 sha256;
        std::string hashStr = sha256(str);
        
        // Сворачиваем строку хеша в size_t
        size_t hash = 0;
        for (char c : hashStr) {
            hash = hash * 271 + c;  // Простое преобразование для сворачивания
        }
        return hash;
    }

    /**
     * Криптографическая хеш-функция MD5
     * Обеспечивает высокую криптостойкость и равномерное распределение
     * 
     * @param str - входная строка
     * @return size_t - свёрнутое хеш-значение (из 128-битного хеша)
     */
    size_t hashMD5(const std::string& str)
    {
        MD5 md5;
        std::string hashStr = md5(str);
        
        // Сворачиваем строку хеша в size_t
        size_t hash = 0;
        for (char c : hashStr) {
            hash = hash * 131 + c;
        }
        return hash;
    }

    /**
     * Реализация MurmurHash3 (32-битная версия)
     * Высококачественная некриптографическая хеш-функция
     * Отличается высокой скоростью и хорошим распределением
     * 
     * @param myString - входная строка
     * @param len - длина строки
     * @param seed - начальное значение
     * @return uint32_t - 32-битный хеш
     */
    uint32_t murmur3_32(const std::string& myString, uint32_t len, uint32_t seed)
    {
        const char* key = myString.c_str();

        // Константы MurmurHash
        static const uint32_t c1 = 0xcc9e2d51;
        static const uint32_t c2 = 0x1b873593;
        static const uint32_t r1 = 15;
        static const uint32_t r2 = 13;
        static const uint32_t m = 5;
        static const uint32_t n = 0xe6546b64;

        uint32_t hash = seed;

        // Обработка блоков по 4 байта
        const int nblocks = len / 4;
        const uint32_t* blocks = (const uint32_t*)key;
        for (int i = 0; i < nblocks; i++) {
            uint32_t k = blocks[i];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;

            hash ^= k;
            hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
        }

        // Обработка оставшихся байтов (менее 4)
        const uint8_t* tail = (const uint8_t*)(key + nblocks * 4);
        uint32_t k1 = 0;

        switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
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
     * Адаптированная функция MurmurHash для использования в DynBloomFilter
     * 
     * @param str - входная строка
     * @return size_t - хеш-значение
     */
    size_t murmurHash(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.length());
        uint32_t seed = 0;      // Начальное значение
        uint32_t hash = murmur3_32(str, len, seed);
        return static_cast<size_t>(hash);
    }

    /**
     * Добавление хеш-функции в фильтр
     * 
     * @param hashFunction - хеш-функция, принимающая элемент T и возвращающая size_t
     */
    void addHashFunction(const std::function<size_t(const T&)>& hashFunction)
    {
        hashFunctions.push_back(hashFunction);
    }

    /**
     * Вставка элемента в фильтр Блума
     * Для каждой хеш-функции вычисляется индекс и устанавливается соответствующий бит
     * 
     * @param item - элемент для вставки
     */
    void insert(const T& item)
    {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions)
        {
            size_t index = hashFunction(item) % data.size();
            data[index] = 1;              // Устанавливаем бит в 1
            collisionMap[index]++;        // Увеличиваем счётчик коллизий
            indices.push_back(index);     // Сохраняем индекс для анализа
        }
        elementIndices[item] = indices;   // Сохраняем индексы для текущего элемента
    }

    /**
     * Проверка наличия элемента в фильтре
     * 
     * @param item - проверяемый элемент
     * @return true - элемент возможно присутствует (может быть ложноположительное срабатывание)
     * @return false - элемент точно отсутствует
     */
    bool exists(const T& item)
    {
        for (const auto& hashFunction : hashFunctions) 
        {
            size_t index = hashFunction(item) % data.size();
            if (data[index] == 0) {
                return false; // Если хоть один бит не установлен, элемента точно нет
            }
        }
        return true; // Если все биты установлены, элемент возможно существует
    }

    /**
     * Получение индексов битов, соответствующих элементу
     * 
     * @param item - элемент
     * @return std::vector<size_t> - вектор индексов установленных битов
     */
    std::vector<size_t> getIndices(const T& item) const
    {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % data.size();
            if (data[index]) {
                indices.push_back(index);
            }
        }
        return indices;
    }

    /**
     * Вычисление размера фильтра в байтах
     * 
     * @return size_t - размер в байтах
     */
    size_t getSizeInBytes() const {
        size_t filterSize = sizeof(DynBloomFilter);           // Размер объекта фильтра
        size_t bitArraySize = data.size() / 8;                // Размер битового массива в байтах
        return filterSize + bitArraySize;                     // Общий размер
    }

    /**
     * Подсчёт относительных коллизий
     * Коллизия - ситуация, когда несколько элементов устанавливают один и тот же бит
     * 
     * @return size_t - общее количество коллизий
     */
    size_t countCollisions() const {
        size_t totalCollisions = 0;
        for (const auto& entry : collisionMap) {
            if (entry.second > 1) {
                totalCollisions += entry.second - 1;  // Подсчёт коллизий для каждого индекса
            }
        }
        return totalCollisions;
    }

    /**
     * Подсчёт абсолютных коллизий (полных совпадений)
     * Абсолютная коллизия - когда два элемента имеют полностью совпадающие наборы индексов
     * 
     * @return size_t - количество абсолютных коллизий
     */
    size_t retFullCollisions() const
    {
        size_t totalCollisions = 0;
        // Дополнительная логика для подсчёта коллизий, когда все индексы совпадают
        for (const auto& itemEntry : elementIndices)
        {
            const auto& itemIndices = itemEntry.second;
            for (const auto& otherItemEntry : elementIndices)
            {
                if (itemEntry.first != otherItemEntry.first)
                {
                    const auto& otherItemIndices = otherItemEntry.second;
                    if (itemIndices == otherItemIndices)
                    {
                        totalCollisions++;  // Увеличить счётчик коллизий, если все индексы совпадают
                    }
                }
            }
        }
        return totalCollisions;
    }

    /**
     * Получение количества хеш-функций
     * 
     * @return size_t - количество зарегистрированных хеш-функций
     */
    size_t getHashFunctionsCount() const {
        return hashFunctions.size();
    }

    /**
     * Очистка фильтра Блума
     * Сбрасывает все биты и очищает статистику коллизий
     */
    void clear() {
        std::fill(data.begin(), data.end(), false);  // Очистка данных
        collisionMap.clear();                         // Очистка информации о коллизиях
    }
};