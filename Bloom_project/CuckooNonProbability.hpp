#include <iostream>
#include <vector>
#include <string>
#include <functional> // Для std::hash

class CuckooFilterN {
private:
    std::vector<std::string> table;
    int size;
    int numBuckets;

public:
    CuckooFilterN(int size) : size(size), numBuckets(1000) {
        table.resize(numBuckets);
    }

    void insert(const std::string& key) 
    {
        int index1 = hash1(key);
        int index2 = hash2(key);
        int index3 = hash3(key);

        //Используем первые два индекса для вставки, если они свободны
        if (table[index1] == "" && table[index2] == "") {
            table[index1] = key;
        }
        else {
            //Если места уже заняты, пытаемся найти другое место
            int i = 1;
            while (i <= numBuckets) {
                index1 = (index1 + i) % numBuckets;
                index2 = (index2 + i) % numBuckets;
                if (table[index1] == "" && table[index2] == "") {
                    table[index1] = key;
                    break;
                }
                i++;
            }
        }
    }

    bool contains(const std::string& key) const {
        int index1 = hash1(key);
        int index2 = hash2(key);
        int index3 = hash3(key);

        return table[index1] == key || table[index2] == key || table[index3] == key;
    }

    void remove(const std::string& key) {
        int index1 = hash1(key);
        int index2 = hash2(key);
        int index3 = hash3(key);

        // Удаляем элемент, если он найден
        if (table[index1] == key) {
            table[index1] = "";
        }
        else if (table[index2] == key) {
            table[index2] = "";
        }
        else if (table[index3] == key) {
            table[index3] = "";
        }
    }

private:

    //простейшее суммирование ASCII-кодов символов строки
    int hash1(const std::string& key) const {
        int hashValue = 0;
        for (char c : key) {
            hashValue += c;
        }
        return hashValue % numBuckets;
    }

    //квадратичный хеш
    int hash2(const std::string& key) const {
        int hashValue = 0;
        for (char c : key) {
            hashValue += c * c; // Простой вариант, использующий квадраты символов
        }
        return hashValue % numBuckets;
    }

    // Встроенный STL-хеш код
    int hash3(const std::string& key) const {
        return std::hash<std::string>()(key) % numBuckets;
    }


};