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
        // ������������� ���� ���-������
    }

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item) {
        for (size_t attempt = 0; attempt < 500; ++attempt) { // ������������ ���������� �������
            std::vector<size_t> indices;
            for (const auto& hashFunction : hashFunctions) {
                size_t index = hashFunction(item) % tables.size();
                indices.push_back(index);
            }

            if (insertItem(item, indices)) {
                return; // ������� �������� �������, ������� �� ������
            }
        }
        throw std::runtime_error("�� ������� �������� ������� ����� 500 �������"); // ����������� ����������, ���� �� ������� �������� �������
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
                return false; // ���� ���� �� � ����� ������� ������� �� ������
            }
        }
        return true; // ���� ������� ������ �� ���� ��������
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
        return removed; // ���������� true, ���� ������� ��� ����� ���� �� �� ����� �������
    }


private:
    bool insertItem(const T& item, const std::vector<size_t>& indices) {
        for (size_t i = 0; i < indices.size(); ++i) {
            if (tables[indices[i]].insert({ item, true }).second) {
                return true; // ������� �������� �������
            }
        }

        // ���� �� ������� �������� �������, �������� ��������� ������������ �������
        for (size_t i = 0; i < indices.size(); ++i) {
            if (evictAndInsert(item, indices[i])) {
                return true;
            }
        }
        return false; // �� ������� �������� ������� ����� ���� �������
    }

    bool evictAndInsert(const T& newItem, size_t index) {
        // ������� ��������� ������� �� �������� �����
        auto it = tables[index].begin();
        if (it != tables[index].end()) {
            T oldItem = it->first;
            tables[index].erase(it);

            // ���������� ������ ���-������� ��� ����������� ������ ������� ��� ������� ��������
            size_t newIndex = hashFunctions[(index + 1) % hashFunctions.size()](oldItem) % tables.size();

            // ��������� ������ ������� � ����� ����
            if (!tables[newIndex].insert({ oldItem, true }).second) {
                // ���� ������ ������� �� ������� ��������, �������� ��������� ���
                return evictAndInsert(oldItem, newIndex);
            }

            // ��������� ����� ������� � �������� ����
            return tables[index].insert({ newItem, true }).second;
        }

        // ���� ���� ����, ������ ��������� ����� �������
        return tables[index].insert({ newItem, true }).second;
    }

};
