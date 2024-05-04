
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
    std::unordered_map<size_t, T> valueMap; // ��� �������� �������� ���������
    std::unordered_map<T, std::vector<size_t>> elementIndices; // ��� ������������ �������� ���������
    std::unordered_map<size_t, size_t> collisionMap; // ��� �������� ��������

public:
    InvertibleBloomFilter(size_t size) : counters(size, 0) {}

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item) {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            counters[index]++; // ����������� ������� ��� ������� �������
            indices.push_back(index); // ��������� ������� ��� �������� ��������
            collisionMap[index]++;
            // ������ �������� ��������, ���� ��� ������ ���������
            if (counters[index] == 1) {
                valueMap[index] = item;
            }
        }
        elementIndices[item] = indices; // ��������� ������� ��� �������� ��������
    }

    bool exists(const T& item) {
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index] == 0) {
                return false; // ���� ���� �� ���� ������� ����� 0, ������� ����� �� ������������
            }
        }
        return true; // ���� ��� �������� ������ 0, ������� ����� ���� � ���������
    }

    void remove(const T& item) {
        std::vector<size_t> indices = getIndices(item);
        for (const auto& index : indices) {
            if (counters[index] > 0) {
                counters[index]--; // ��������� �������, ���� ������� ������������
                // ���� ������� ���� 0, ������� �������� �� valueMap
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
        return T(); // ���������� ������ ��������, ���� ������� �� ������
    }

    size_t getSizeInBytes() const {
        // ������ ������� ������� �����
        size_t filterSize = sizeof(InvertibleBloomFilter);
        // ������ ������� ����� � ������
        size_t bitArraySize = counters.size() / 8; // ������ � ������, �����������, ��� 1 ��� �������� 1 ����
        // ����� ������ � ������
        return filterSize + bitArraySize;
    }

    std::vector<T> getAllValues() const {
        std::vector<T> values;
        for (const auto& entry : valueMap) {
            values.push_back(entry.second);
        }
        return values;
    }

    // ����� ��� �������� ��������
    size_t countCollisions() const {
        size_t totalCollisions = 0;
        for (const auto& entry : collisionMap) {
            if (entry.second > 1) {
                totalCollisions += entry.second - 1; // ������� �������� ��� ������� �������
            }
        }
        return totalCollisions;
    }

    size_t retFullCollisions() const
    {
        size_t totalCollisions = 0;
        // �������������� ������ ��� �������� ��������, ����� ��� ������� ���������
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
                        totalCollisions++; // ��������� ������� ��������, ���� ��� ������� ���������
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
        std::fill(counters.begin(), counters.end(), 0); // ������� ������
        valueMap.clear(); // ������� ���������� � ��������� ���������
        elementIndices.clear(); // ������� ���������� � �������� ���������
    }
};
