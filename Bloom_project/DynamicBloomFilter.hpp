#include <vector>
#include <functional>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include "md5.h"
#include "sha256.h"


template <typename T>
class DynBloomFilter
{
private:
    //std::bitset<size_t> data;
    std::vector<bool> data;
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    std::unordered_map<size_t, size_t> collisionMap; // ��� �������� ��������
    std::unordered_map<T, std::vector<size_t>> elementIndices; // ��� ������������ �������� ��������� � ���������� ��������

    int hash_f_counter;
    size_t size;



public:
    DynBloomFilter(int numElements, double falsePositiveRate)
    {
        size_t m  = static_cast<size_t>(numElements * log(falsePositiveRate) / (log(2) * log(2)));
        //std::cout << "Size DFB =" << m << std::endl;
        size = m;

        int k = static_cast<int>(log(static_cast<double>(2)) * m / (abs(static_cast<double>(numElements))));

        hash_f_counter = k;

        // ������������� �������
        data.resize(m, 0);

        // �������� � ���������� ���-�������

        
        for (int i = 0; i < k; ++i) {
            if (i == 0)
            {
                addHashFunction([this](const T& item) { return this->hashFunction1(item); });
            }
            else if (i == 1)
            {
                
                addHashFunction([this](const T& item) { return this->hashFunction2(item); });
            }
            else if (i == 2)
            {
               
                addHashFunction([this](const T& item) { return this->murmurHash(item); });
            }
            else if (i > 2)
            {
                // ������ ���-�������, ����������� ���� ����������� �������
                auto hashFunction = [i](const T& item) {
                    return std::hash<T>{}(item)+i;
                    };
                addHashFunction(hashFunction);
            }
        }
        
    }

    // ������ ������������� ������� ����� ��� �����
    size_t hashFunction1(const std::string& str) {
        size_t hash = 0;
        for (char c : str) {
            hash = hash * 31 + c; // ������ ���-������� ��� ������
        }
        return hash;
    }

    size_t hashFunction2(const std::string& str) {
        size_t hash = 0;
        for (char c : str) {
            hash = hash * 37 + c; // ������ ���-������� ��� ������
        }
        return hash;
    }

    size_t hashSHA256(const std::string& str) {
        SHA256 sha256;
        std::string hashStr = sha256(str);
        // std::cout << hashStr << std::endl;
         // �������������� ������ ���� � size_t. ��� ������� ������, ������� ����� �� ���� ���������.
         // � �������� ���������� ��� ����� ������������� ����� ������� ����� ��������������.
        size_t hash = 0;

        for (char c : hashStr) {
            hash = hash * 271 + c; // ������ �������� ��������������
        }
        //std::cout << "Your hash:" << std::endl;
        //std::cout << hash << std::endl;
        return hash;

    }

    size_t hashMD5(const std::string& str)
    {
        MD5 md5;
        std::string hashStr = md5(str);
        // std::cout << hashStr << std::endl;
         // �������������� ������ ���� � size_t. ��� ������� ������, ������� ����� �� ���� ���������.
         // � �������� ���������� ��� ����� ������������� ����� ������� ����� ��������������.
        size_t hash = 0;

        for (char c : hashStr) {
            hash = hash * 131 + c; // ������ �������� ��������������
        }
        //std::cout << "Your hash:" << std::endl;
        //std::cout << hash << std::endl;
        return hash;
    }



    uint32_t murmur3_32(const std::string& myString, uint32_t len, uint32_t seed)
    {
        const char* key = myString.c_str(); // �������������� std::string � const char*

        static const uint32_t c1 = 0xcc9e2d51;
        static const uint32_t c2 = 0x1b873593;
        static const uint32_t r1 = 15;
        static const uint32_t r2 = 13;
        static const uint32_t m = 5;
        static const uint32_t n = 0xe6546b64;

        uint32_t hash = seed;

        const int nblocks = len / 4;
        const uint32_t* blocks = (const uint32_t*)key;
        int i;
        for (i = 0; i < nblocks; i++) {
            uint32_t k = blocks[i];
            k *= c1;
            k = (k << r1) | (k >> (32 - r1));
            k *= c2;

            hash ^= k;
            hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
        }

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

        hash ^= len;
        hash ^= (hash >> 16);
        hash *= 0x85ebca6b;
        hash ^= (hash >> 13);
        hash *= 0xc2b2ae35;
        hash ^= (hash >> 16);

        return hash;
    }

    // �������������� ������� ����������� MurmurHash ��� ������������� � BloomFilter
    size_t murmurHash(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.length());
        uint32_t seed = 0; // �� ������ ������������ ����� �������� ��� seed
        uint32_t hash = murmur3_32(str, len, seed);
        return static_cast<size_t>(hash);
    }

    void addHashFunction(const std::function<size_t(const T&)>& hashFunction)
    {
        hashFunctions.push_back(hashFunction);
    }

    void insert(const T& item)
    {
        std::vector<size_t> indices;//
        for (const auto& hashFunction : hashFunctions)
        {
            size_t index = hashFunction(item) % data.size();
            data[index] = 1;
            collisionMap[index]++;
            indices.push_back(index);
        }
        elementIndices[item] = indices; // ��������� ������� ��� �������� ��������
    }

    bool exists(const T& item)
    {
        for (const auto& hashFunction : hashFunctions) 
        {
            size_t index = hashFunction(item) % data.size();
            if (data[index] == 0) {
                return false; // ���� ���� ���� ��� �� ����������, ������� ����� �� ����������.
            }
        }
        return true; // ���� ��� ���� �����������, ������� �������� ����������.
    }
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

    size_t getSizeInBytes() const {
        // ������ ������� ������� �����
        size_t filterSize = sizeof(DynBloomFilter);
        // ������ ������� ����� � ������
        size_t bitArraySize = data.size() / 8; // ������ � ������, �����������, ��� 1 ��� �������� 1 ����
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
        std::fill(data.begin(), data.end(), false); // ������� ������
        collisionMap.clear(); // ������� ���������� � ���������
    }

};




