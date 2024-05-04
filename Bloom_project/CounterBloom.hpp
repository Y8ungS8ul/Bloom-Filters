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
    std::unordered_map<size_t, size_t> collisionMap; // ��� �������� ��������
    std::unordered_map<T, std::vector<size_t>> elementIndices; // ��� ������������ �������� ��������� � ���������� ��������

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
            counters[index]++; // ����������� ������� ��� ������� �������
            collisionMap[index]++;
            indices.push_back(index); //

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

    void remove(const T& item) 
    {
        for (const auto& hashFunction : hashFunctions) 
        {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index] > 0) 
            {
                counters[index]--; // ��������� �������, ���� ������� ������������
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
        // ������ ������� ������� �����
        size_t filterSize = sizeof(CountingBloomFilter);
        // ������ ������� ����� � ������
        size_t bitArraySize = counters.size() / 8; // ������ � ������, �����������, ��� 1 ��� �������� 1 ����
        // ����� ������ � ������
        return filterSize + bitArraySize;
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

    size_t getHashFunctionsCount() const {
        return hashFunctions.size();
    }

    // ����� ��� ������� ������� �����
    void clear() {
        std::fill(counters.begin(), counters.end(), false); // ������� ������
        collisionMap.clear(); // ������� ���������� � ���������
    }

};




