
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

template<typename T>
class InvertibleBloomFilter {
private:
    std::vector<int> counters;
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    std::unordered_map<size_t, T> valueMap; // Для хранения значений элементов
    std::unordered_map<T, std::vector<size_t>> elementIndices; // Для отслеживания индексов элементов
    std::unordered_map<size_t, size_t> collisionMap; // Для подсчета коллизий

public:
    InvertibleBloomFilter(size_t size) : counters(size, 0) {}

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item) {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            counters[index]++; // Увеличиваем счётчик для данного индекса
            indices.push_back(index); // Сохраняем индексы для текущего элемента
            collisionMap[index]++;
            // Храним значение элемента, если это первое вхождение
            if (counters[index] == 1) {
                valueMap[index] = item;
            }
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

    void remove(const T& item) {
        std::vector<size_t> indices = getIndices(item);
        for (const auto& index : indices) {
            if (counters[index] > 0) {
                counters[index]--; // Уменьшаем счётчик, если элемент присутствует
                // Если счетчик стал 0, удаляем значение из valueMap
                if (counters[index] == 0) {
                    valueMap.erase(index);
                }
            }
        }
    }

    std::vector<size_t> getIndices(const T& item) const {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index]) {
                indices.push_back(index);
            }
        }
        return indices;
    }

    T getValue(size_t index) const {
        auto it = valueMap.find(index);
        if (it != valueMap.end()) {
            return it->second;
        }
        return T(); // Возвращаем пустое значение, если элемент не найден
    }

    size_t getSizeInBytes() const {
        // Размер объекта фильтра Блума
        size_t filterSize = sizeof(InvertibleBloomFilter);
        // Размер массива битов в байтах
        size_t bitArraySize = counters.size() / 8; // Размер в байтах, предполагая, что 1 бит занимает 1 байт
        // Общий размер в байтах
        return filterSize + bitArraySize;
    }

    std::vector<T> getAllValues() const {
        std::vector<T> values;
        for (const auto& entry : valueMap) {
            values.push_back(entry.second);
        }
        return values;
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

    size_t getHashFunctionsCount() const 
    {
        return hashFunctions.size();
    }

    void clear() {
        std::fill(counters.begin(), counters.end(), 0); // Очистка данных
        valueMap.clear(); // Очистка информации о значениях элементов
        elementIndices.clear(); // Очистка информации о индексах элементов
    }
};
