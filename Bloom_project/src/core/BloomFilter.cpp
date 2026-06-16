#ifndef BLOOM_FILTER_CPP
#define BLOOM_FILTER_CPP

#include "BloomFilter.hpp"
#include <algorithm>
#include <cmath>

template <typename T>
BloomFilter<T>::BloomFilter(size_t size) : data(size, false) {}

template <typename T>
void BloomFilter<T>::addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
    hashFunctions.push_back(hashFunction);
}

template <typename T>
void BloomFilter<T>::insert(const T& item) {
    std::vector<size_t> indices;
    indices.reserve(hashFunctions.size());  // Резервируем память для оптимизации
    
    for (const auto& hashFunction : hashFunctions) {
        size_t index = hashFunction(item) % data.size();
        data[index] = true;
        collisionMap[index]++;  // Увеличиваем счетчик коллизий для индекса
        indices.push_back(index);
    }
    elementIndices[item] = indices;  // Сохраняем индексы для будущих коллизий
}

template <typename T>
bool BloomFilter<T>::exists(const T& item) const {
    for (const auto& hashFunction : hashFunctions) {
        size_t index = hashFunction(item) % data.size();
        if (!data[index]) {
            return false;  // Хотя бы один бит не установлен - элемента точно нет
        }
    }
    return true;  // Все биты установлены - элемент возможно существует
}

/**
 * @brief Возвращает индексы в битовом массиве, которые соответствуют элементу
 * 
 * @param item Элемент, индексы которого нужно получить
 * @return std::vector<size_t> Вектор индексов, где биты установлены в 1
 * 
 * @note В отличие от внутренней логики вставки, этот метод возвращает только
 *       те индексы, которые действительно установлены в 1
 */
template <typename T>
std::vector<size_t> BloomFilter<T>::getIndices(const T& item) const 
{
    std::vector<size_t> indices;
    indices.reserve(hashFunctions.size());
    
    for (const auto& hashFunction : hashFunctions) {
        size_t index = hashFunction(item) % data.size();
        if (data[index]) {  // Проверяем, что бит действительно установлен
            indices.push_back(index);
        }
    }
    return indices;
}

/**
 * @brief Вычисляет размер фильтра в байтах
 * 
 * @return size_t Размер в байтах
 * 
 * @details Размер складывается из:
 *          - Размера самого объекта BloomFilter (с учетом всех полей)
 *          - Размера битового массива (data.size() / 8 байт)
 * 
 * @note Формула: sizeof(BloomFilter) + (битовый_массив / 8)
 * @warning Метод не учитывает динамическую память для hashFunctions, collisionMap и elementIndices
 */
template <typename T>
size_t BloomFilter<T>::getSizeInBytes() const {
    // Размер объекта фильтра Блума (стеки + указатели)
    size_t filterSize = sizeof(BloomFilter);
    
    // Размер битового массива в байтах
    // std::vector<bool> хранит биты эффективно, но стандарт не гарантирует битовую упаковку
    size_t bitArraySize = data.size() / 8;
    if (data.size() % 8 != 0) {
        bitArraySize += 1;  // Добавляем байт для неполных битов
    }
    
    // Приблизительный общий размер
    return filterSize + bitArraySize;
}

/**
 * @brief Подсчет относительных коллизий (на уровне отдельных индексов)
 * 
 * @return size_t Количество коллизий
 * 
 * @details Коллизия считается, когда несколько элементов установили один и тот же бит.
 *          Для каждого индекса коллизии = (количество_установок - 1)
 * 
 * @example Если индекс 5 был установлен 3 раза, то коллизий = 2
 * 
 * @see retFullCollisions() для подсчета абсолютных коллизий
 */
template <typename T>
size_t BloomFilter<T>::countCollisions() const {
    size_t totalCollisions = 0;
    
    for (const auto& entry : collisionMap) {
        if (entry.second > 1) {  // Если индекс установлен более одного раза
            totalCollisions += entry.second - 1;  // Подсчитываем лишние установки
        }
    }
    
    return totalCollisions;
}

/**
 * @brief Подсчет абсолютных коллизий (когда элементы имеют одинаковые наборы индексов)
 * 
 * @return size_t Количество абсолютных коллизий
 * 
 * @details Абсолютная коллизия возникает, когда два разных элемента имеют
 *          полностью совпадающие наборы индексов от всех хеш-функций.
 * 
 * @warning Сложность O(n²), где n - количество уникальных элементов.
 *          Использовать только для отладки и анализа, не для production!
 * 
 * @see countCollisions() для подсчета относительных коллизий
 */
template <typename T>
size_t BloomFilter<T>::retFullCollisions() const
{
    size_t totalCollisions = 0;
    
    // Сравниваем каждую пару элементов
    for (const auto& itemEntry : elementIndices) 
    {
        const auto& itemIndices = itemEntry.second;
        
        for (const auto& otherItemEntry : elementIndices) 
        {
            // Не сравниваем элемент с самим собой
            if (itemEntry.first != otherItemEntry.first) 
            {
                const auto& otherItemIndices = otherItemEntry.second;
                
                // Если наборы индексов полностью совпадают
                if (itemIndices == otherItemIndices)
                {
                    totalCollisions++;
                }
            }
        }
    }
    
    // Каждая коллизия посчитана дважды (A с B и B с A)
    return totalCollisions / 2;
}


/**
 * @brief Возвращает количество используемых хеш-функций
 * 
 * @return size_t Количество хеш-функций
 */
template <typename T>
size_t BloomFilter<T>::getHashFunctionsCount() const {
    return hashFunctions.size();
}

/**
 * @brief Полная очистка фильтра Блума
 * 
 * @details Сбрасывает все биты в 0 и очищает информацию о коллизиях.
 *          Фильтр становится пустым, как после создания.
 * 
 * @warning Хеш-функции остаются в фильтре, их нужно удалять отдельно
 */
template <typename T>
void BloomFilter<T>::clear() {
    // Сбрасываем все биты в false
    std::fill(data.begin(), data.end(), false);
    
    // Очищаем информацию о коллизиях и индексах
    collisionMap.clear();
    elementIndices.clear();
}

#endif  // BLOOM_FILTER_CPP