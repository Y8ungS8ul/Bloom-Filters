#include <string>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <random>
#include <ctime>
#include <chrono>
#include <future>
#include <mutex>

#include <unordered_map>
#include <unordered_set>
#include <numeric>
#include <deque>



template <typename T>
class CountingBloomFilter
{
private:
    std::vector<int> counters;
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    std::unordered_map<size_t, size_t> collisionMap; // Для подсчета коллизий
    std::unordered_map<T, std::vector<size_t>> elementIndices; // Для отслеживания индексов элементов и абсолютных коллизий

public:
    CountingBloomFilter(size_t size) : counters(size, 0) {}

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction)
    {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item)
    {
        std::vector<size_t> indices;//
        for (const auto& hashFunction : hashFunctions)
        {
            size_t index = hashFunction(item) % counters.size();
            counters[index]++; // Увеличиваем счётчик для данного индекса
            collisionMap[index]++;
            indices.push_back(index); //

        }
        elementIndices[item] = indices; // Сохраняем индексы для текущего элемента
    }

    bool exists(const T& item) {
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index] == 0) {
                return false; // Если хотя бы один счётчик равен 0, элемент точно не присутствует
            }
        }
        return true; // Если все счётчики больше 0, элемент может быть в множестве
    }

    void remove(const T& item) 
    {
        for (const auto& hashFunction : hashFunctions) 
        {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index] > 0) 
            {
                counters[index]--; // Уменьшаем счётчик, если элемент присутствует
            }
        }
    }

    std::vector<size_t> getIndices(const T& item) const
    {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index]) {
                indices.push_back(index);
            }
        }
        return indices;
    }

    size_t getSizeInBytes() const {
        // Размер объекта фильтра Блума
        size_t filterSize = sizeof(CountingBloomFilter);
        // Размер массива битов в байтах
        size_t bitArraySize = counters.size() / 8; // Размер в байтах, предполагая, что 1 бит занимает 1 байт
        // Общий размер в байтах
        return filterSize + bitArraySize;
    }

    // Метод для подсчета коллизий
    size_t countCollisions() const {
        size_t totalCollisions = 0;
        for (const auto& entry : collisionMap) {
            if (entry.second > 1) {
                totalCollisions += entry.second - 1; // Подсчет коллизий для каждого индекса
            }
        }
        return totalCollisions;
    }

    size_t retFullCollisions() const
    {
        size_t totalCollisions = 0;
        // Дополнительная логика для подсчета коллизий, когда все индексы совпадают
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
                        totalCollisions++; // Увеличить счетчик коллизий, если все индексы совпадают
                    }
                }
            }
        }
        return totalCollisions;
    }

    size_t getHashFunctionsCount() const {
        return hashFunctions.size();
    }

    // Метод для очистки фильтра Блума
    void clear() {
        std::fill(counters.begin(), counters.end(), false); // Очистка данных
        collisionMap.clear(); // Очистка информации о коллизиях
    }

};




