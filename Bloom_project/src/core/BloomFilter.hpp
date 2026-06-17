#ifndef BLOOM_FILTER_HPP
#define BLOOM_FILTER_HPP

#include <vector>
#include <functional>
#include <unordered_map>
#include <string>

template <typename T>
class BloomFilter {
private:
    std::vector<bool> data;
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    std::unordered_map<size_t, size_t> collisionMap;
    std::unordered_map<T, std::vector<size_t>> elementIndices;

public:
    explicit BloomFilter(size_t size);
    
    void addHashFunction(const std::function<size_t(const T&)>& hashFunction);
    void insert(const T& item);
    bool exists(const T& item) const;
    std::vector<size_t> getIndices(const T& item) const;
    size_t getSizeInBytes() const;
    size_t countCollisions() const;
    size_t retFullCollisions() const;
    size_t getHashFunctionsCount() const;
    void clear();
};

#include "BloomFilter.cpp" // для шаблонов
#endif