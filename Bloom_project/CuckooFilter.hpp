#include <vector>
#include <unordered_map>
#include <functional>
#include <stdexcept>

template <typename T>
class CuckooFilter {
private:
    std::vector<std::unordered_map<T, bool>> tables;
    std::vector<std::function<size_t(const T&)>> hashFunctions;

public:
    CuckooFilter(size_t size) : tables(3, std::unordered_map<T, bool>()) {
        // Инициализация трех хеш-таблиц
    }

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item) {
        for (size_t attempt = 0; attempt < 500; ++attempt) { // Ограничиваем количество попыток
            std::vector<size_t> indices;
            for (const auto& hashFunction : hashFunctions) {
                size_t index = hashFunction(item) % tables.size();
                indices.push_back(index);
            }

            if (insertItem(item, indices)) {
                return; // Успешно вставили элемент, выходим из метода
            }
        }
        throw std::runtime_error("Не удалось вставить элемент после 500 попыток"); // Выбрасываем исключение, если не удалось вставить элемент
    }


    size_t getHashFunctionsCount() const {
        return hashFunctions.size();
    }

    void print() const {
        for (const auto& table : tables) {
            std::cout << "Table contents:" << std::endl;
            for (const auto& pair : table) {
                std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
            }
            std::cout << "-----------------------------" << std::endl;
        }
    }


    bool exists(const T& item) {
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % tables.size();
            if (tables[index].find(item) == tables[index].end()) {
                return false; // Если хотя бы в одной таблице элемент не найден
            }
        }
        return true; // Если элемент найден во всех таблицах
    }

    bool remove(const T& item) {
        bool removed = false;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % tables.size();
            auto it = tables[index].find(item);
            if (it != tables[index].end()) {
                tables[index].erase(it);
                removed = true;
            }
        }
        return removed; // Возвращает true, если элемент был удалён хотя бы из одной таблицы
    }


private:
    bool insertItem(const T& item, const std::vector<size_t>& indices) {
        for (size_t i = 0; i < indices.size(); ++i) {
            if (tables[indices[i]].insert({ item, true }).second) {
                return true; // Успешно вставили элемент
            }
        }

        // Если не удалось вставить элемент, пытаемся вытеснить существующий элемент
        for (size_t i = 0; i < indices.size(); ++i) {
            if (evictAndInsert(item, indices[i])) {
                return true;
            }
        }
        return false; // Не удалось вставить элемент после всех попыток
    }

    bool evictAndInsert(const T& newItem, size_t index) {
        // Попытка вытеснить элемент из текущего слота
        auto it = tables[index].begin();
        if (it != tables[index].end()) {
            T oldItem = it->first;
            tables[index].erase(it);

            // Используем другую хеш-функцию для определения нового индекса для старого элемента
            size_t newIndex = hashFunctions[(index + 1) % hashFunctions.size()](oldItem) % tables.size();

            // Вставляем старый элемент в новый слот
            if (!tables[newIndex].insert({ oldItem, true }).second) {
                // Если старый элемент не удалось вставить, пытаемся вытеснить его
                return evictAndInsert(oldItem, newIndex);
            }

            // Вставляем новый элемент в исходный слот
            return tables[index].insert({ newItem, true }).second;
        }

        // Если слот пуст, просто вставляем новый элемент
        return tables[index].insert({ newItem, true }).second;
    }

};
