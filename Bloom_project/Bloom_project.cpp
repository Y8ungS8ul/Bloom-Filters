#include <iostream>
#include <iomanip>
#include <limits>
#include <functional>
#include <vector>
#include <functional>
#include <algorithm>
#include <bitset>


#include "ProgramMenu.h"
#include "CounterBloom.hpp"
#include "InvertibleBloomFilter.hpp"
#include "CuckooFilter.hpp"
#include "CuckooNonProbability.hpp"
#include "YFastTrie.hpp"
#include "MyLinkedList.hpp"
#include "DynamicBloomFilter.hpp"


#include <sqlite3.h>
#include "sha256.h"
#include "md5.h"


#include <sstream>
#include <fstream>
#include <SFML/Graphics.hpp>
#include <string>

#include <cmath>
#include <random>
#include <ctime>
#include <chrono>
#include <future>
#include <mutex>

#include <unordered_map>
#include <unordered_set>
#include <numeric>


using namespace sf;


//HyperLogLog

// Функция для генерации случайного символа из латинского алфавита
char randomLetter() {
    // ASCII коды для 'a' и 'z'
    const int a = 'a';
    const int z = 'z';
    // Генерация случайного числа в диапазоне от 'a' до 'z'
    return static_cast<char>(a + rand() % (z - a + 1));
}

class HyperLogLog 
{
public:
    HyperLogLog(int p) : p(p), m(1 << p), registers(m, 0) {}

    void insert(const std::string& element) {
        size_t hash = std::hash<std::string>{}(element);
        int index = hash % m;
        int rho = getRho(hash);
        if (rho > registers[index]) {
            registers[index] = rho;
        }
    }

    double estimate() const 
    {
        double alpha = getAlpha(p);
        double sum = 0.0;
        for (size_t value : registers) {
            sum += std::pow(2.0, value);
        }
        double estimate = alpha * m * m / sum;
        if (estimate <= 2.5 * m) {
            size_t zeroCount = std::count(registers.begin(), registers.end(), 0);
            if (zeroCount != 0) {
                estimate = m * std::log(static_cast<double>(m) / zeroCount);
            }
        }
        return estimate;
    }

private:
    int p;
    size_t m;
    std::vector<int> registers;

    int getRho(size_t hash) const {
        int rho = 1;
        while ((hash & 1) == 0) {
            rho++;
            hash >>= 1;
        }
        return rho;
    }

    double getAlpha(int p) const {
        if (p == 4) return 0.673;
        if (p == 5) return 0.697;
        if (p == 6) return 0.709;
        return 0.7213 / (1 + 1.079 / m);
    }
};



// Функция для подсчета уникальных элементов в строке CSV
size_t countUniqueElementsInCSVLine(const std::string& csvLine) {
    std::unordered_set<std::string> uniqueElements;
    std::stringstream ss(csvLine);
    std::string item;

    // Разбор строки и добавление уникальных элементов в set
    while (std::getline(ss, item, ',')) {
        uniqueElements.insert(item);
    }

    // Возвращаем количество уникальных элементов
    return uniqueElements.size();
}

// Функция для подсчета уникальных слов в текстовом файле
size_t countUniqueWordsInTextFile(const std::string& filePath) {
    std::unordered_set<std::string> uniqueWords;
    std::ifstream file(filePath);
    std::string line;

    if (file.is_open()) {
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                // Преобразование слова в нижний регистр для учета различий в регистре
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                uniqueWords.insert(word);
            }
        }
        file.close();
    }
    else {
        std::cout << "Не удалось открыть файл." << std::endl;
        return 0;
    }

    return uniqueWords.size();
}


template <typename T>
class BloomFilter
{
private:
    //std::bitset<size_t> data;
    std::vector<bool> data;
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    std::unordered_map<size_t, size_t> collisionMap; // Для подсчета коллизий
    std::unordered_map<T, std::vector<size_t>> elementIndices; // Для отслеживания индексов элементов и абсолютных коллизий

public:
    BloomFilter(size_t size) : data(size, 0) {}

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
            indices.push_back(index); //
          
        }
        elementIndices[item] = indices; // Сохраняем индексы для текущего элемента
    }

    bool exists(const T& item)
    {
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % data.size();
            if (data[index] == 0) {
                return false; // Если хоть один бит не установлен, элемент точно не существует.
            }
        }
        return true; // Если все биты установлены, элемент возможно существует.
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
        // Размер объекта фильтра Блума
        size_t filterSize = sizeof(BloomFilter);
        // Размер массива битов в байтах
        size_t bitArraySize = data.size() / 8; // Размер в байтах, предполагая, что 1 бит занимает 1 байт
        // Общий размер в байтах
        return filterSize + bitArraySize;
    }

    //Метод для подсчета коллизий
    size_t countCollisions() const {
        size_t totalCollisions = 0;
        for (const auto& entry : collisionMap) {
            if (entry.second > 1) {
                totalCollisions += entry.second - 1; //Подсчет коллизий для каждого индекса
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
        std::fill(data.begin(), data.end(), false); // Очистка данных
        collisionMap.clear(); // Очистка информации о коллизиях
    }

};


// Пример использования фильтра Блюма для строк
size_t hashFunction1(const std::string& str) {
    size_t hash = 0;
    for (char c : str) {
        hash = hash * 31 + c; // Пример хэш-функции для строки
    }
    return hash;
}

size_t hashFunction2(const std::string& str) {
    size_t hash = 0;
    for (char c : str) {
        hash = hash * 37 + c; // Другая хэш-функция для строки
    }
    return hash;
}

size_t hashSHA256(const std::string& str) {
    SHA256 sha256;
    std::string hashStr = sha256(str);
   // std::cout << hashStr << std::endl;
    // Преобразование строки хеша в size_t. Это простой пример, который может не быть идеальным.
    // В реальном приложении вам может потребоваться более сложный метод преобразования.
    size_t hash = 0;
    
    for (char c : hashStr) {
        hash = hash * 271 + c; // Пример простого преобразования
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
     // Преобразование строки хеша в size_t. Это простой пример, который может не быть идеальным.
     // В реальном приложении вам может потребоваться более сложный метод преобразования.
    size_t hash = 0;

    for (char c : hashStr) {
        hash = hash * 131 + c; // Пример простого преобразования
    }
    //std::cout << "Your hash:" << std::endl;
    //std::cout << hash << std::endl;
    return hash;
}

#include <cstdint>

uint32_t murmur3_32(const std::string& myString, uint32_t len, uint32_t seed) 
{
    const char* key = myString.c_str(); // Преобразование std::string в const char*
    
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

// Адаптированная функция хеширования MurmurHash для использования в BloomFilter
size_t murmurHash(const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.length());
    uint32_t seed = 0; // Вы можете использовать любое значение для seed
    uint32_t hash = murmur3_32(str, len, seed);
    return static_cast<size_t>(hash);
}



// функция настройки текста
void InitText(Text & mtext, float xpos, float ypos, String str, int size_font = 60,
    Color menu_text_color = Color::White, int bord = 0, Color border_color = Color::Black);




// Функция перехода к фильтр блум basic variation
void BasicBloom()
{
    
    sf::RenderWindow Play(sf::VideoMode::getDesktopMode(), L"Уровень 1", sf::Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::RectangleShape background_play(sf::Vector2f(Play.getSize().x, Play.getSize().y));
    sf::Texture texture_window;
    if (!texture_window.loadFromFile("image/bloomimg.jpg")) exit(1);
    background_play.setTexture(&texture_window);

    // Создание объекта sf::Text
    sf::Text text;
    // Установка шрифта
    text.setFont(font);
    text.setString(L" [ESC] - вернуться назад \n [1] - визуализация Фильтра Блума на 10 'мест' \n [2] - 20-местный Фильтр Блума с демонстрацией работы / (очистка) \n [3] - показать формулы и прочее* \n [4] - 30 местный ФБ (Мурмур-хф) \n\n [A] - добавить слово 'apple' \n [B] - добавить слово 'banana' \n [Q] - добавить случайно-сгенерированное слово на 20 символов");
    text.setCharacterSize(26);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(1180, 1070));

    sf::Text text2;
    text2.setFont(font);
    text2.setCharacterSize(26);
    text2.setFillColor(sf::Color::White);
    text2.setPosition(sf::Vector2f(180, 1090));

   
    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения фона под текст-управления
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);


    //Формулы:

    //1 - оптимальное число ХФ

    // Создание прямоугольника
    sf::RectangleShape rectangle2(sf::Vector2f(311, 76));
    rectangle2.setPosition(200, 150);
    rectangle2.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle2.setOutlineThickness(5); // Установка толщины рамки
    rectangle2.setOutlineColor(sf::Color::Red); // Установка цвета рамки
    sf::Texture f1;
    if (!f1.loadFromFile("image/f_optimalHF.png"))
    {
        return;
    }
    sf::Sprite Spritef1(f1);
    Spritef1.setPosition(200, 150);

    sf::Text f1_optimalhf_text;
    f1_optimalhf_text.setFont(font);
    f1_optimalhf_text.setCharacterSize(30);
    f1_optimalhf_text.setFillColor(sf::Color::Red);
    f1_optimalhf_text.setPosition(530, 170); // Позиция слева сверху

    sf::Text f1_optimalhftitle;
    f1_optimalhftitle.setFont(font);
    f1_optimalhftitle.setCharacterSize(34);
    f1_optimalhftitle.setFillColor(sf::Color::White);
    f1_optimalhftitle.setPosition(250, 50);
    f1_optimalhftitle.setString(L"Оптимальное количество хеш-функций для \n m-размерного фильтра с n-элементами:");

    //2 - вероятность ложноположительного срабатывания

    // Создание прямоугольника
    sf::RectangleShape rectangle3(sf::Vector2f(303, 99));
    rectangle3.setPosition(1200, 150);
    rectangle3.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle3.setOutlineThickness(5); // Установка толщины рамки
    rectangle3.setOutlineColor(sf::Color::Red); // Установка цвета рамки
    sf::Texture f2;
    if (!f2.loadFromFile("image/f_prob.png"))
    {
        return;
    }
    sf::Sprite Spritef2(f2);
    Spritef2.setPosition(1200, 150);

    sf::Text f2_prob;
    f2_prob.setFont(font);
    f2_prob.setCharacterSize(30);
    f2_prob.setFillColor(sf::Color::Red);
    f2_prob.setPosition(1540, 185); // Позиция справа

    sf::Text f2_ftitle;
    f2_ftitle.setFont(font);
    f2_ftitle.setCharacterSize(34);
    f2_ftitle.setFillColor(sf::Color::White);
    f2_ftitle.setPosition(1050, 70);
    f2_ftitle.setString(L"Вероятность ложно положительного срабатывания [P]: ");


 
    // Создание вектора для хранения квадратов
    std::vector<sf::RectangleShape> squares;
    std::vector<sf::Text> numbers; // Вектор для хранения текстовых объектов
    BloomFilter<std::string> filter(20);
    filter.addHashFunction(hashFunction1);
    filter.addHashFunction(hashFunction2);
    //filter.addHashFunction(hashSHA256);
    //filter.addHashFunction(murmurHash);

    
    


    while (Play.isOpen())
    {
        sf::Event event;
        while (Play.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                Play.close();
            else if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) { Play.close(); }
                else if (event.key.code == sf::Keyboard::Num1) // Проверка нажатия клавиши "1"
                {

                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");


                    // Создание 10 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    // Расчет общего отступа для 20 квадратов с учетом их размеров и желаемого отступа
                    float totalWidth = 10 * 50 + (10 - 1) * 10; // 20 квадратов * 50 пикселей в ширину каждого + 19 отступов * 10 пикселей каждый
                    float startX = (Play.getSize().x - totalWidth) / 2; // Начальная позиция X для первого квадрата

                    for (int i = 0; i < 10; ++i)
                    {
                        sf::RectangleShape square(sf::Vector2f(50, 50)); // Размер квадрата
                        square.setFillColor(sf::Color::Red); // Цвет квадрата
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2); // Позиционирование квадрата с учетом отступа
                        squares.push_back(square);

                        // Создание текстового объекта для каждого квадрата
                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24); // Размер шрифта
                        number.setFillColor(sf::Color::White); // Цвет текста
                        number.setPosition(square.getPosition().x + square.getSize().x / 2 - number.getLocalBounds().width / 2, square.getPosition().y + square.getSize().y / 2 - number.getLocalBounds().height / 2); // Центрирование текста внутри квадрата
                        numbers.push_back(number);
                    }
                }
                else if (event.key.code == sf::Keyboard::Num2) // Проверка нажатия клавиши "2"
                {
                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");
                    // Создание 20 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    // Расчет общего отступа для 20 квадратов с учетом их размеров и желаемого отступа
                    float totalWidth = 20 * 50 + (20 - 1) * 10; // 20 квадратов * 50 пикселей в ширину каждого + 19 отступов * 10 пикселей каждый
                    float startX = (Play.getSize().x - totalWidth) / 2; // Начальная позиция X для первого квадрата

                    for (int i = 0; i < 20; ++i)
                    {
                        sf::RectangleShape square(sf::Vector2f(50, 50)); // Размер квадрата
                        square.setFillColor(sf::Color::Red); // Цвет квадрата
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2); // Позиционирование квадрата с учетом отступа
                        squares.push_back(square);

                        // Создание текстового объекта для каждого квадрата
                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24); // Размер шрифта
                        number.setFillColor(sf::Color::White); // Цвет текста
                        number.setPosition(square.getPosition().x + square.getSize().x / 2 - number.getLocalBounds().width / 2, square.getPosition().y + square.getSize().y / 2 - number.getLocalBounds().height / 2); // Центрирование текста внутри квадрата
                        numbers.push_back(number);
                    }
      
                    
                    
                }
                else if (event.key.code == sf::Keyboard::A) //apple 
                {
                   

                    filter.insert("apple");
                    std::vector<size_t> idxs = filter.getIndices("apple");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Green); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                }
                else if (event.key.code == sf::Keyboard::B) //banana
                {
                    

                    filter.insert("banana");
                    std::vector<size_t> idxs = filter.getIndices("banana");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Black); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                }
                else if (event.key.code == sf::Keyboard::Q) //qwertyuiop
                {


                    filter.insert("qwertyjuiceapplemasterpiece");
                    std::vector<size_t> idxs = filter.getIndices("qwertyjuiceapplemasterpiece");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Magenta); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                }
                else if (event.key.code == sf::Keyboard::Num3)
                {
                    text2.setString(L" Число коллизий: " + std::to_string(filter.countCollisions()) + L"\n Количество хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                    //с округлением до ближайшего целого числа
                    f1_optimalhf_text.setString(" = " + std::to_string(0.6931 * 20 / 3) + " ~ " + std::to_string(std::ceil(0.6931 * 20 / 3)));
                    f2_prob.setString(" = " + std::to_string(pow((1 - exp(-2 * 3 / 20)), 2)));

                }

                else if (event.key.code == sf::Keyboard::Num4)
                {
                    BloomFilter<std::string> filter(30);
                    filter.addHashFunction(hashFunction1);
                    filter.addHashFunction(hashFunction2);
                    //filter.addHashFunction(hashSHA256);
                    filter.addHashFunction(murmurHash);
                    // Создание 20 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    // Расчет общего отступа для 20 квадратов с учетом их размеров и желаемого отступа
                    float totalWidth = 30 * 50 + (30 - 1) * 10; // 20 квадратов * 50 пикселей в ширину каждого + 19 отступов * 10 пикселей каждый
                    float startX = (Play.getSize().x - totalWidth) / 2; // Начальная позиция X для первого квадрата

                    for (int i = 0; i < 30; ++i)
                    {
                        sf::RectangleShape square(sf::Vector2f(50, 50)); // Размер квадрата
                        square.setFillColor(sf::Color::Red); // Цвет квадрата
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2); // Позиционирование квадрата с учетом отступа
                        squares.push_back(square);

                        // Создание текстового объекта для каждого квадрата
                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24); // Размер шрифта
                        number.setFillColor(sf::Color::White); // Цвет текста
                        number.setPosition(square.getPosition().x + square.getSize().x / 2 - number.getLocalBounds().width / 2, square.getPosition().y + square.getSize().y / 2 - number.getLocalBounds().height / 2); // Центрирование текста внутри квадрата
                        numbers.push_back(number);
                    }

                    filter.insert("apple");
                    std::vector<size_t> idxs = filter.getIndices("apple");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Green); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }

                    filter.insert("banana");
                    std::vector<size_t> idxs2 = filter.getIndices("banana");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs2)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Black); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }

                    filter.insert("qwertyjuiceapplemasterpiece");
                    std::vector<size_t> idxs3 = filter.getIndices("qwertyjuiceapplemasterpiece");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs3)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Magenta); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }

                    text2.setString(L" Число коллизий: " + std::to_string(filter.countCollisions()) + L"\n Количество хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));

                    //с округлением до ближайшего целого числа
                    f1_optimalhf_text.setString(" = " + std::to_string(0.6931 * 30 / 3) + " ~ " + std::to_string(std::ceil(0.6931 * 30 / 3)));
                    f2_prob.setString(" = " + std::to_string(pow((1 - exp(-3 * 3 / 30)), 3)));



                }

            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    Play.clear();
                    
                }
            }

        }

        Play.clear();
        Play.draw(background_play);
        Play.draw(rectangle1);
        Play.draw(gifSprite1);
        Play.draw(rectangle2);
        Play.draw(rectangle3);
        Play.draw(Spritef1);
        Play.draw(Spritef2);
        Play.draw(f1_optimalhf_text);
        Play.draw(f1_optimalhftitle);
        Play.draw(f2_prob);
        Play.draw(f2_ftitle);
        Play.draw(text);
        Play.draw(text2);
       
       
        for (const auto& square : squares)
        {
            Play.draw(square); // Отображение квадратов
        }
        for (const auto& number : numbers)
        {
            Play.draw(number); // Отображение текстовых объектов
        }

        Play.display();
    }
   
}



// Функция для генерации случайного числа в заданном диапазоне
int randomNumber(int min, int max) {
    static std::default_random_engine generator(std::time(0));
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator);
}

void Options()
{
    sf::RenderWindow Play(sf::VideoMode::getDesktopMode(), L"HLL", sf::Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::RectangleShape background_play(sf::Vector2f(Play.getSize().x, Play.getSize().y));
    sf::Texture texture_window;
    if (!texture_window.loadFromFile("image/hllback.jpg")) exit(1);
    background_play.setTexture(&texture_window);


    // Создание прямоугольника
    sf::RectangleShape rectangle(sf::Vector2f(500, 500));
    rectangle.setPosition(750, 500);
    rectangle.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle.setOutlineThickness(5); // Установка толщины рамки
    rectangle.setOutlineColor(sf::Color::Red); // Установка цвета рамки


    // Загрузка гиф-анимации в текстуру
    sf::Texture Texture1;
    if (!Texture1.loadFromFile("image/hllV.png"))
    {
        return;
    }

    // Создание спрайта для отображения фона под текст-управления
    sf::Sprite Sprite(Texture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    Sprite.setScale(500.0f / Texture1.getSize().x, 500.0f / Texture1.getSize().y);
    Sprite.setPosition(750, 500);


    // Создание объекта sf::Text
    sf::Text text;
    // Установка шрифта
    text.setFont(font);
    text.setString(L" [ESC] - вернуться назад \n [1] - визуализация HLL \n [2] - Пример работы DBF с hyperloglog \n [A] - добавить слово 'apple' \n [B] - добавить слово 'banana' \n [Q] - добавить случайно-сгенерированное слово на 20 символов");
    text.setCharacterSize(26);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(1180, 1070));

    // Создание объектов sf::Text для каждого сообщения
    sf::Text text1, text2, text3, text4, text5, text6;
    text1.setFont(font);
    text2.setFont(font);
    text3.setFont(font);
    text4.setFont(font);
    text5.setFont(font);
    text6.setFont(font);

    int size = 30;
    // Установка размера шрифта и цвета
    text1.setCharacterSize(size);
    text2.setCharacterSize(size);
    text3.setCharacterSize(size);
    text4.setCharacterSize(size);
    text5.setCharacterSize(size);
    text6.setCharacterSize(size);

    text1.setFillColor(sf::Color::White);
    text2.setFillColor(sf::Color::White);
    text3.setFillColor(sf::Color::White);
    text4.setFillColor(sf::Color::White);
    text5.setFillColor(sf::Color::White);
    text6.setFillColor(sf::Color::White);

    // Установка позиций текстовых объектов
    text1.setPosition(1300, 500);
    text2.setPosition(1300, 550);
    text3.setPosition(1300, 600);
    text4.setPosition(1300, 650);
    text5.setPosition(1300, 700);
    text6.setPosition(1300, 750);

    std::string filePath = "data.txt"; // Укажите путь к вашему текстовому файлу
    HyperLogLog hll(12); // Инициализация HyperLogLog с параметром p=12

    std::ifstream file(filePath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line))
        {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word)
            {
                hll.insert(word);
            }
        }
        file.close();
    }
    else {
        std::cout << "Не удалось открыть файл." << std::endl;
        return;
    }

    double estimate = hll.estimate();
   


    std::string csvLine;
    std::ifstream file1("100_SalesRecords.csv");

    if (file1.is_open()) {
        // Чтение первой строки из файла
        if (std::getline(file1, csvLine)) {
            size_t uniqueCount = countUniqueElementsInCSVLine(csvLine);
            
        }
        else
        {
            std::cout << "Не удалось прочитать строку из файла." << std::endl;
        }
        file.close();
    }
    else
    {
        std::cout << "Не удалось открыть файл." << std::endl;
    }
    std::string filePath1 = "data.txt"; // Укажите путь к вашему текстовому файлу
    size_t uniqueCount = countUniqueWordsInTextFile(filePath1);
   

    srand(static_cast<unsigned>(time(0))); // Инициализация генератора случайных чисел

    /*
    std::ofstream file2("words.txt"); // Создание файла words.txt
    if (!file2) {
        std::cerr << "Не удалось открыть файл для записи." << std::endl;
        return 1;
    }

    std::string find2 = "";

    for (int i = 0; i < 10000; ++i)
    {
        for (int j = 0; j < 10; ++j)
        {
            file2 << randomLetter(); // Запись случайного символа в файл
        }
        file2 << '\n'; // Переход на новую строку после каждого слова
    }

    file2.close(); // Закрытие файла
    std::cout << "Файл words.txt успешно создан." << std::endl;
    */

    std::string filePath3 = "words.txt"; // Укажите путь к вашему текстовому файлу
    HyperLogLog hll2(13); // Инициализация HyperLogLog с параметром p=12

    std::ifstream file3(filePath3);
    if (file3.is_open())
    {
        std::string line;
        while (std::getline(file3, line))
        {
            std::stringstream ss(line);
            std::string word;
            while (ss >> word) {
                hll2.insert(word);
            }
        }
        file3.close();
    }
    else
    {
        std::cout << "Не удалось открыть файл." << std::endl;
        return;
    }

    estimate = hll2.estimate();
   
    std::string line;
    int wordCount = 0;
    file.open("words.txt"); // Замените "words.txt" на имя вашего файла

    if (file.is_open()) {
        while (std::getline(file, line)) {
            // Проверяем, что строка не пустая
            if (!line.empty()) {
                wordCount++;
            }
        }
        file.close();
    }
    else
    {
        std::cout << "Не удалось открыть файл" << std::endl;
        return;
    }

    



    int number = estimate; //проверим число из HLL
    int roundedNumber;

    // Определение ближайшего высшего разряда
    int order = pow(10.0, floor(log10(number)));

    // Округление до ближайшего высшего разряда
    if (number - order >= order / 2) {
        // Если число ближе к верхней границе, округляем вверх
        roundedNumber = (number / order + 1) * order;
    }
    else {
        // Если число ближе к нижней границе, округляем вниз
        roundedNumber = number / order * order;
    }

   


    //Создание фильтра блума на основе HLL-подсчета
    

    DynBloomFilter<std::string> filter(-roundedNumber, 0.1);

    file.open("words.txt");
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл" << std::endl;
        return;
    }
    std::string word;
    while (std::getline(file, word)) {
        //Добавляем слово в Bloom Filter
        filter.insert(word);
        //std::cout << word << std::endl;
    }

    file.close();

    while (Play.isOpen())
    {
        sf::Event event;
        while (Play.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                Play.close();
            else if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) { Play.close(); }
                else if (event.key.code == sf::Keyboard::Num1) // Проверка нажатия клавиши "1"
                {

                    text1.setString(L"Есть ли в фильтре несуществующее слово '11111111'? :" + std::to_string(filter.exists("11111111")));
                    text2.setString(L"Хранится ли в фильтре существующее слово 'hpuelitwch'? :" + std::to_string(filter.exists("hpuelitwch")));
                    text3.setString(L"Абсолютное количество коллизий:" + std::to_string(0));
                    text4.setString(L"Относительное количество коллизий:" + std::to_string(filter.countCollisions()));
                    text5.setString(L"Количество хеш-функций:" + std::to_string(filter.getHashFunctionsCount()));
                    text6.setString(L"Размер ДФБ в байтах: " + std::to_string(sizeof(filter)));
                }
            }

            Play.clear();
            Play.draw(background_play);
            Play.draw(rectangle);
            Play.draw(Sprite);
            Play.draw(text1);
            Play.draw(text2);
            Play.draw(text3);
            Play.draw(text4);
            Play.draw(text5);
            Play.draw(text6);

            Play.display();
        }
    }
}

std::map<double, double> calculateCollisions(size_t filterSize);
//метод для анализа коллизий при 3 хф от размера Фильтра
std::map<int, int> calculateCollisions_size(int hashfunctions_count);
std::map<double, double> calculateCollisionsABS(size_t filterSize);

// Функция настройки игры
void CSV_BLOOM()
{
    RenderWindow Options(VideoMode::getDesktopMode(), L"Настройки", Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::Font font1;
    if (!font1.loadFromFile("fonts/timesbd.ttf")) exit(1);
    RectangleShape background_opt(Vector2f(Options.getSize().x, Options.getSize().y));
    Texture texture_opt;
    if (!texture_opt.loadFromFile("image/blcsv.jpg")) exit(2);
   
    sf::Texture texture;
    if (!texture.loadFromFile("image/button1.png"))
    {
        return;
    }

    sf::Texture texture2;
    if (!texture2.loadFromFile("image/button2.png"))
    {
        return;
    }

    sf::Texture texture3;
    if (!texture3.loadFromFile("image/button3.png"))
    {
        return;
    }

    sf::Texture texture4;
    if (!texture4.loadFromFile("image/button4.png"))
    {
        return;
    }

    sf::Texture back_texture;
    if (!back_texture.loadFromFile("image/back_button2.png"))
    {
        return;
    }

    sf::Sprite sprite1(texture);
    sprite1.setPosition(100, 1130); // Установка позиции спрайта

    sf::Sprite sprite2(texture2);
    sprite2.setPosition(350, 1130);

    sf::Sprite sprite3(texture3);
    sprite3.setPosition(600, 1130);

    sf::Sprite sprite4(texture4);
    sprite4.setPosition(850, 1130);

    sf::Sprite back_sprite(back_texture);
    back_sprite.setPosition(95, 975);


    sf::Text b1label;
    b1label.setFont(font);
    b1label.setCharacterSize(20);
    b1label.setFillColor(sf::Color::White);
    b1label.setPosition(170, 1300); // Установка позиции текста
    b1label.setString(L"Назад");

    sf::Text b2label;
    b2label.setFont(font);
    b2label.setCharacterSize(20);
    b2label.setFillColor(sf::Color::White);
    b2label.setPosition(430, 1300); // Установка позиции текста
    b2label.setString(L"CSV*");

    sf::Text b3label;
    b3label.setFont(font);
    b3label.setCharacterSize(20);
    b3label.setFillColor(sf::Color::White);
    b3label.setPosition(650, 1300); // Установка позиции текста
    b3label.setString(L"HF - C-count*");

    sf::Text b4label;
    b4label.setFont(font);
    b4label.setCharacterSize(20);
    b4label.setFillColor(sf::Color::White);
    b4label.setPosition(895, 1300); // Установка позиции текста
    b4label.setString(L"SBF - C-count*");



    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    sf::Text rect_text1;
    rect_text1.setFont(font);
    rect_text1.setCharacterSize(24);
    rect_text1.setFillColor(sf::Color::White);
    rect_text1.setPosition(1180, 1090); // Позиция слева сверху
    rect_text1.setString(L"[1] - СSV* - открыть файл на 1000 строк и проверить работу фильтра блума");

    sf::Text rect_text2;
    rect_text2.setFont(font);
    rect_text2.setCharacterSize(24);
    rect_text2.setFillColor(sf::Color::White);
    rect_text2.setPosition(1180, 1120); // Позиция слева сверху
    rect_text2.setString(L"[2] - HF - C-count* - диаграмма сравнения зависимости коллизий от числа ХФ");

    sf::Text rect_text3;
    rect_text3.setFont(font);
    rect_text3.setCharacterSize(24);
    rect_text3.setFillColor(sf::Color::White);
    rect_text3.setPosition(1180, 1150); // Позиция слева сверху
    rect_text3.setString(L"[3] - SBF - C-count* - диаграмма сравнения числа коллизий между ФБ с \nразными размерами");



    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения гиф-анимации
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);
   
    



    // Таймер для обновления слова каждую секунду
    sf::Clock clock;
 
    // Создание текстового объекта для отображения счетчика
    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(200, 900); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text isText;
    isText.setFont(font);
    isText.setCharacterSize(24);
    isText.setFillColor(sf::Color::White);
    isText.setPosition(200, 850); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text kfText;
    kfText.setFont(font);
    kfText.setCharacterSize(24);
    kfText.setFillColor(sf::Color::Green);
    kfText.setPosition(200, 800); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tText;
    tText.setFont(font);
    tText.setCharacterSize(24);
    tText.setFillColor(sf::Color::Green);
    tText.setPosition(200, 925); // Позиция слева сверху
    std::string tTextContent;

    std::string line;
    int counter = 0;
    int maxLines = 20; // Максимальное количество строк, которые будут отображаться одновременно
    std::vector<sf::Text> linesText(maxLines);
    for (auto& text : linesText) {
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
    }

    background_opt.setTexture(&texture_opt);

    std::ifstream file("1000_SalesRecords.csv");
    if (!file.is_open()) 
    {
        std::cerr << "Unable to open file" << std::endl;
        return;
    }

    //переменная для отслеживания ввода
    
    int startDisplaying = 0;
    bool fl = 0;
    bool alllinesAdded = 0;
    size_t filterSize = 100000;
    BloomFilter<std::string> filter(filterSize);
    filter.addHashFunction(hashFunction1);
    filter.addHashFunction(hashFunction2);
    
  

    while (Options.isOpen()) 
    {
        sf::Event event;
       
        while (Options.pollEvent(event)) 
        {
            if (event.type == sf::Event::Closed) Options.close();
            if (event.type == sf::Event::KeyPressed) 
            {
                if (event.key.code == sf::Keyboard::Escape) Options.close();
                if (event.key.code == sf::Keyboard::Num1) 
                { 
                    // Проверка нажатия кнопки 1
                    startDisplaying = 1; // Изменение состояния на начало вывода строк
                    filter.addHashFunction(hashSHA256);
                    
                }
                else if (event.key.code == sf::Keyboard::Num2)
                {
                    startDisplaying = 2;
                    filter.addHashFunction(murmurHash);
                    filter.addHashFunction(hashSHA256);
                    filter.addHashFunction(hashMD5);
                }
                else if (event.key.code == sf::Keyboard::Num3)
                {
                    startDisplaying = 3;
                }
            }
            
        }

        Options.clear();
        Options.draw(background_opt);

        if (startDisplaying == 1)
        {
           
            if (std::getline(file, line)) 
            {
                filter.insert(line); // Добавление строки в фильтр Блума
                counter++;
                if (counter % 10000 == 0)
                {
                    counterText.setString(std::to_string(counter) + " added");
                    Options.draw(counterText);
                }

                // Обновление позиций текстовых объектов
                for (int i = 0; i < maxLines; ++i) {
                    linesText[i].setString(line);
                    linesText[i].setPosition(200, 200 + i * 30); // Пример позиции, сдвигаем вниз
                }
            }
            else
            {
                // Если достигнут конец файла, закрываем его и выходим из цикла
                file.close();
                
                alllinesAdded = true;
                //break;

            }

            // Отрисовка текстовых объектов
            for (const auto& text : linesText)
            {
                Options.draw(text);
                counterText.setString(std::to_string(counter) + " added");
                Options.draw(counterText);
            }


          
            // Проверка на наличие слова "apple" после добавления всех строк
            if (alllinesAdded && !fl) 
            {
                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);
                
                sf::Clock timer;
                bool exists = filter.exists("Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                sf::Time elapsed = timer.getElapsedTime();
                tText.setString("Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds");
                tTextContent = "Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds";
                Options.draw(tText);

                fl = true;
                    
                
            }
            // Обновление tText только если измерение времени было выполнено
            if (fl) {
                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                tText.setString(tTextContent);
                Options.draw(tText);
            }
            // Отображение текста и "кнопки"
            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 2)
        {
            std::map<double, double> diagrammap = calculateCollisions(5000);
            std::map<double, double> abscolli = calculateCollisionsABS(5000);
            sf::Text titul, title;
            
            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;
            std::vector <sf::Text> abscollisions;

            int columnIndex = 0;
            for (const auto& pair : diagrammap) 
            {
                double columnHeight = pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + columnIndex * 300, 790 - columnHeight);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::White); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                sf::Text prob;
                
                prob.setFont(font1);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Yellow); // Цвет текста
                prob.setString("P = "+ std::to_string(pow((1 - exp(-pair.first * 1000 / 50000)), pair.first))); // Текст подписи
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);


                columnIndex++;
                if (columnIndex >= 5) break;
            }

            int x = 348, y = 860;
            for (const auto& pair : abscolli)
            {
             

                // Создание подписи для столбца
                sf::Text abscollision;
                abscollision.setFont(font);
                abscollision.setCharacterSize(24); // Размер шрифта
                abscollision.setFillColor(sf::Color::White); // Цвет текста
                abscollision.setString(std::to_string(std::ceil(pair.first)) + " - " + std::to_string(std::ceil(pair.second))); // Текст подписи
                abscollision.setPosition(x, y); // Позиционирование подписи под столбцом
                abscollisions.push_back(abscollision);
                x += 300;
                

            }

           

            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n[Вероятность ложноположительного срабатывания] \n[Число хеш-функций] - [Абсолютное количество коллизий]");
            titul.setPosition(700, 250);
            title.setFont(font);
            title.setCharacterSize(50); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"    Зависимость уровня коллизий от числа хеш-функций \nСравнение вероятностей ложноположительного результата");
            title.setPosition(Options.getSize().x / 2 - 650, 120);
            // Отрисовка столбцов
            for (const auto& column : columns) 
            {
                Options.draw(column);
            }
            for (const auto& label : labels) 
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }
            for (const auto& abscollision : abscollisions)
            {
                Options.draw(abscollision);
            }


              
            Options.draw(titul);
            Options.draw(title);
            // Отображение текста

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 3)
        {
            sf::Text titul, title;
            std::map<int, int> diagrammap = calculateCollisions_size(3);

            sf::Text info;
            sf::Text tp;
            tp.setFont(font);
            tp.setCharacterSize(35); // Размер шрифта
            tp.setFillColor(sf::Color::Red);
            tp.setPosition(245, 703);
            tp.setString("P =");

            info.setFont(font);
            info.setCharacterSize(30); // Размер шрифта
            info.setFillColor(sf::Color::Green); // Цвет текста
            info.setString(" ~ " + std::to_string(-1000*log(0.001)/pow(0.69,2)) + L" : оптимальный размер ФБ для получения ложноположительного срабатывания с P = 0.001, n = 1000");
            info.setPosition(450, 840);

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;

            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture;
            if (!gifTexture.loadFromFile("image/white.jpg"))
            {
                return;
            }

            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSprite(gifTexture);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник
            gifSprite.setScale(1800.0f / gifTexture.getSize().x, 400.0f / gifTexture.getSize().y);
            gifSprite.setPosition(200, 370);

            // Создание прямоугольника
            sf::RectangleShape rectangle(sf::Vector2f(1800, 400));
            rectangle.setPosition(200, 370);
            rectangle.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle.setOutlineThickness(5); // Установка толщины рамки
            rectangle.setOutlineColor(sf::Color::Red); // Установка цвета рамки



            // Создание прямоугольника для формулы m
            sf::RectangleShape rectangle4(sf::Vector2f(210, 100));
            rectangle4.setPosition(200, 800);
            rectangle4.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle4.setOutlineThickness(5); // Установка толщины рамки
            rectangle4.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture2;
            if (!gifTexture2.loadFromFile("image/f3.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef(gifTexture2);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник
           
            gifSpritef.setPosition(200, 800);


            // Создание прямоугольника для формулы оптимальной p
            sf::RectangleShape rectangle5(sf::Vector2f(311, 76));
            rectangle5.setPosition(1200, 550);
            rectangle5.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle5.setOutlineThickness(5); // Установка толщины рамки
            rectangle5.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture3;
            if (!gifTexture3.loadFromFile("image/f_optimalHF.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef2(gifTexture3);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef2.setPosition(1200, 550);


            int columnIndex = 0;
            int f = 0;

            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.second / 25;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + columnIndex * 300, 1000 - columnHeight - 50 - 80 - 50 - 50 - 100);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::Black); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                double probValue = pow((1 - exp(-3.0 * 1000.0 / pair.first)), 3);
                

                sf::Text prob;
                prob.setFont(font);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Red); // Цвет текста
                prob.setString(std::to_string(probValue)); // Текст подписи
                
                prob.setPosition(column.getPosition().x, label.getPosition().y + 30); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);

                columnIndex++;
                
               
 
            }

           

            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            titul.setPosition(750, 300);
            title.setFont(font);
            title.setCharacterSize(40); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title.setPosition(Options.getSize().x / 2 - 600, 120);


            // Отрисовка прямоугольника
            Options.draw(rectangle);
            Options.draw(gifSprite);
            Options.draw(rectangle4);
            Options.draw(gifSpritef);
            Options.draw(rectangle5);
            Options.draw(gifSpritef2);
            
            
            // Отрисовка столбцов
            for (const auto& column : columns) 
            {
                Options.draw(column);
            }
            for (const auto& label : labels) 
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }
           

            // Отображение отрисованного на экране
            
            Options.draw(info);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(tp);


            // Отображение текста и "кнопок" - указателей

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }

        
        Options.display();
    }
    
}


//Функция, которая возвращает строку из элементов вектора
template <typename T>
std::string vectorToString(const std::vector<T>& vec) {
    std::stringstream ss;
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != 0) {
            ss << " "; // Добавляем пробел перед каждым элементом, кроме первого
        }
        ss << vec[i]; // Добавляем элемент вектора в строку
    }
    return ss.str(); // Возвращаем строку
}

// Функция с описанием приложения
void About_Game()
{
    RenderWindow About(VideoMode::getDesktopMode(), L"menu", Style::Fullscreen);
    RectangleShape background_ab(Vector2f(VideoMode::getDesktopMode().width, VideoMode::getDesktopMode().height));
    Texture texture_ab;
    if (!texture_ab.loadFromFile("image/menu2.jpg")) exit(3);
    background_ab.setTexture(&texture_ab);

    while (About.isOpen())
    {
        Event event_play;
        while (About.pollEvent(event_play))
        {
            if (event_play.type == Event::Closed) About.close();
            if (event_play.type == Event::KeyPressed)
            {
                if (event_play.key.code == Keyboard::Escape) About.close();
            }
        }
        About.clear();
        About.draw(background_ab);
        About.display();
    }
}



//Метод для вычисления ОТНОСИТЕЛЬНОГО числа коллизий: количество ХФ - число коллизий
std::map<double, double> calculateCollisions(size_t filterSize) {
    std::map<double, double> collisionsMap;

    // Создание 5 фильтров Блума с разным числом хеш-функций
    for (int numHashFunctions = 1; numHashFunctions <= 5; ++numHashFunctions) {
        BloomFilter<std::string> filter(filterSize);

        // Добавление хеш-функций в фильтр
        for (int i = 0; i < numHashFunctions; ++i) 
        {
            if (i == 0)
                filter.addHashFunction(murmurHash); // Пример использования хеш-функции
            else if (i == 1)
                filter.addHashFunction(hashFunction1);
            else if (i == 2)
                filter.addHashFunction(hashFunction2);
            else if (i == 3)
                filter.addHashFunction(hashSHA256);
            else
                filter.addHashFunction(hashMD5);
        }

        // Считывание данных из файла
        std::ifstream file("1000_SalesRecords.csv");
        if (!file.is_open()) {
            std::cerr << "Unable to open file" << std::endl;
            return collisionsMap; // Возвращаем пустой словарь, если файл не открыт
        }

        std::string line;
        while (std::getline(file, line)) { // Считывание каждой строки целиком
            filter.insert(line); // Добавление строки в фильтр Блума
        }
        file.close();

        // Вычисление количества коллизий и добавление результатов в словарь
        double collisions = filter.countCollisions();
        collisionsMap.insert({ numHashFunctions, collisions });
    }

    return collisionsMap;
}


//Метод для вычисления АБСОЛЮТНОГО ПОКАЗАТЕЛЯ коллизий: количество ХФ - число коллизий
std::map<double, double> calculateCollisionsABS(size_t filterSize) {
    std::map<double, double> collisionsMap;

    // Создание 5 фильтров Блума с разным числом хеш-функций
    for (int numHashFunctions = 1; numHashFunctions <= 5; ++numHashFunctions) {
        BloomFilter<std::string> filter(filterSize);

        // Добавление хеш-функций в фильтр
        for (int i = 0; i < numHashFunctions; ++i)
        {
            if (i == 0)
                filter.addHashFunction(murmurHash); // Пример использования хеш-функции
            else if (i == 1)
            {
                filter.addHashFunction(murmurHash);
                filter.addHashFunction(hashFunction1);
            }
            else if (i == 2)
            {
                filter.addHashFunction(murmurHash);
                filter.addHashFunction(hashFunction1);
            filter.addHashFunction(hashFunction2);
            }
            else if (i == 3)
            {
                filter.addHashFunction(murmurHash);
                filter.addHashFunction(hashFunction1);
                filter.addHashFunction(hashFunction2);
                filter.addHashFunction(hashSHA256);
            }
                
            else
            {
                filter.addHashFunction(murmurHash);
                filter.addHashFunction(hashFunction1);
                filter.addHashFunction(hashFunction2);
                filter.addHashFunction(hashSHA256);
                filter.addHashFunction(hashMD5);
            }
               
        }

        // Считывание данных из файла
        std::ifstream file("1000_SalesRecords.csv");
        if (!file.is_open()) {
            std::cerr << "Unable to open file" << std::endl;
            return collisionsMap; // Возвращаем пустой словарь, если файл не открыт
        }

        std::string line;
        while (std::getline(file, line)) { // Считывание каждой строки целиком
            filter.insert(line); // Добавление строки в фильтр Блума
        }
        file.close();

        // Вычисление количества коллизий и добавление результатов в словарь
        double collisions = filter.retFullCollisions();
        collisionsMap.insert({ numHashFunctions, collisions });
    }

    return collisionsMap;
}



//метод для анализа коллизий при 3 хф от размера Фильтра
std::map<int, int> calculateCollisions_size(int hashfunctions_count) 
{
    std::map<int, int> collisionsMap;

    size_t size_f = 1000;
    while (size_f < 30000)
    {

        BloomFilter<std::string> filter(size_f);

       
            filter.addHashFunction(hashFunction1); // Пример использования хеш-функции
            filter.addHashFunction(hashFunction2);
            filter.addHashFunction(murmurHash);
        

        // Считывание данных из файла
        std::ifstream file("1000_SalesRecords.csv");
        if (!file.is_open())
        {
            std::cerr << "Unable to open file" << std::endl;
            return collisionsMap; // Возвращаем пустой словарь, если файл не открыт
        }

        std::string line;
        while (std::getline(file, line)) { // Считывание каждой строки целиком
            filter.insert(line); // Добавление строки в фильтр Блума
        }
        file.close();

        // Вычисление количества коллизий и добавление результатов в словарь
        int collisions = filter.countCollisions();
        collisionsMap.insert({ size_f, collisions });
        size_f += 5000;
    }
    

    return collisionsMap;
}



//Фильтр Блума с подсчетом - метод для отображения на экран
void CountingBF()
{
    RenderWindow Options(VideoMode::getDesktopMode(), L"Настройки", Style::Fullscreen); //Default можно указать для открытия в окне

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::Font font1;
    if (!font1.loadFromFile("fonts/timesbd.ttf")) exit(1);
    RectangleShape background_opt(Vector2f(Options.getSize().x, Options.getSize().y));
    Texture texture_opt;
    if (!texture_opt.loadFromFile("image/cBFimg.jpg")) exit(2);

    sf::Texture texture;
    if (!texture.loadFromFile("image/button1.png"))
    {
        return;
    }

    sf::Texture texture2;
    if (!texture2.loadFromFile("image/button2.png"))
    {
        return;
    }

    sf::Texture texture3;
    if (!texture3.loadFromFile("image/button3.png"))
    {
        return;
    }

    sf::Texture texture4;
    if (!texture4.loadFromFile("image/button4.png"))
    {
        return;
    }

    sf::Texture back_texture;
    if (!back_texture.loadFromFile("image/back_button2.png"))
    {
        return;
    }

    sf::Sprite sprite1(texture);
    sprite1.setPosition(100, 1130); // Установка позиции спрайта

    sf::Sprite sprite2(texture2);
    sprite2.setPosition(350, 1130);

    sf::Sprite sprite3(texture3);
    sprite3.setPosition(600, 1130);

    sf::Sprite sprite4(texture4);
    sprite4.setPosition(850, 1130);

    sf::Sprite back_sprite(back_texture);
    back_sprite.setPosition(95, 975);


    sf::Text b1label;
    b1label.setFont(font);
    b1label.setCharacterSize(20);
    b1label.setFillColor(sf::Color::White);
    b1label.setPosition(170, 1300); // Установка позиции текста
    b1label.setString(L"Назад");

    sf::Text b2label;
    b2label.setFont(font);
    b2label.setCharacterSize(20);
    b2label.setFillColor(sf::Color::White);
    b2label.setPosition(430, 1300); // Установка позиции текста
    b2label.setString(L"CSV*");

    sf::Text b3label;
    b3label.setFont(font);
    b3label.setCharacterSize(20);
    b3label.setFillColor(sf::Color::White);
    b3label.setPosition(650, 1300); // Установка позиции текста
    b3label.setString(L"HF - C-count*");

    sf::Text b4label;
    b4label.setFont(font);
    b4label.setCharacterSize(20);
    b4label.setFillColor(sf::Color::White);
    b4label.setPosition(895, 1300); // Установка позиции текста
    b4label.setString(L"SBF - C-count*");



    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    sf::Text rect_text1;
    rect_text1.setFont(font);
    rect_text1.setCharacterSize(24);
    rect_text1.setFillColor(sf::Color::White);
    rect_text1.setPosition(1180, 1090); // Позиция слева сверху
    rect_text1.setString(L"[1] - СSV* - открыть файл на 1000 строк и проверить работу фильтра блума");

    sf::Text rect_text2;
    rect_text2.setFont(font);
    rect_text2.setCharacterSize(24);
    rect_text2.setFillColor(sf::Color::White);
    rect_text2.setPosition(1180, 1120); // Позиция слева сверху
    rect_text2.setString(L"");

    sf::Text rect_text3;
    rect_text3.setFont(font);
    rect_text3.setCharacterSize(24);
    rect_text3.setFillColor(sf::Color::White);
    rect_text3.setPosition(1180, 1150); // Позиция слева сверху
    rect_text3.setString(L"");



    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения гиф-анимации
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);





    // Таймер для обновления слова каждую секунду
    sf::Clock clock;

    // Создание текстового объекта для отображения счетчика
    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(200, 900); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text isText;
    isText.setFont(font);
    isText.setCharacterSize(24);
    isText.setFillColor(sf::Color::White);
    isText.setPosition(200, 850); // Позиция слева сверху

    // Создание текстового объекта для отображения числа хеш-функций
    sf::Text kfText;
    kfText.setFont(font);
    kfText.setCharacterSize(24);
    kfText.setFillColor(sf::Color::Green);
    kfText.setPosition(200, 800); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tText;
    tText.setFont(font);
    tText.setCharacterSize(24);
    tText.setFillColor(sf::Color::Green);
    tText.setPosition(200, 925); // Позиция слева сверху
    std::string tTextContent;

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tTextrem;
    tTextrem.setFont(font);
    tTextrem.setCharacterSize(24);
    tTextrem.setFillColor(sf::Color::Red);
    tTextrem.setPosition(200, 955); // Позиция слева сверху

    std::deque<std::string> lines;
    std::string line;
    int counter = 0;
    int maxLines = 20; // Максимальное количество строк, которые будут отображаться одновременно
    std::vector<sf::Text> linesText(maxLines);
    for (auto& text : linesText)
    {
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
    }

    background_opt.setTexture(&texture_opt);

    std::ifstream file("1000_SalesRecords.csv");
    if (!file.is_open())
    {
        std::cerr << "Unable to open file" << std::endl;
        return;
    }

    //переменная для отслеживания ввода

    int startDisplaying = 0;
    bool fl = 0;
    bool alllinesAdded = 0;
    size_t filterSize = 10000;
    CountingBloomFilter<std::string> filter(filterSize);
    filter.addHashFunction(hashFunction1);
    filter.addHashFunction(hashFunction2);
    std::unordered_set<std::string> addedLines;
    std::vector<std::string> uniqueLines; // Контейнер для хранения уникальных строк


    while (Options.isOpen())
    {
        sf::Event event;

        while (Options.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) Options.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) Options.close();
                if (event.key.code == sf::Keyboard::Num1)
                {
                    // Проверка нажатия кнопки 1
                    startDisplaying = 1; // Изменение состояния на начало вывода строк
                    filter.addHashFunction(murmurHash);

                }
                else if (event.key.code == sf::Keyboard::Num2)
                {
                    startDisplaying = 2;
                   
                }
                else if (event.key.code == sf::Keyboard::Num3)
                {
                    startDisplaying = 3;
                }
            }

        }

        Options.clear();
        Options.draw(background_opt);

        if (startDisplaying == 1)
        {

            if (std::getline(file, line)) {
                filter.insert(line); // Добавление строки в фильтр Блума
                counter++;
                if (counter % 10000 == 0) {
                    counterText.setString(std::to_string(counter) + " added");
                    Options.draw(counterText);
                }

                // Проверяем, была ли строка уже добавлена
                if (addedLines.find(line) == addedLines.end()) {
                    // Если строка еще не была добавлена, добавляем ее в уникальные строки
                    uniqueLines.push_back(line);
                    addedLines.insert(line);
                }
            }
            else {
                // Если достигнут конец файла, закрываем его и выходим из цикла
                file.close();
                alllinesAdded = true;
            }

            // Обновляем linesText только с уникальными строками
            for (int i = 0; i < maxLines && i < uniqueLines.size(); ++i) {
                linesText[i].setString(uniqueLines[i]);
                linesText[i].setPosition(200, 200 + i * 30); // Пример позиции, сдвигаем вниз
            }

            // Отрисовка текстовых объектов
            for (const auto& text : linesText) {
                Options.draw(text);
                counterText.setString(std::to_string(counter) + " added");
                Options.draw(counterText);
            }



            // Проверка на наличие слова "apple" после добавления всех строк
            if (alllinesAdded && !fl)
            {
                
                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                sf::Clock timer;
                bool exists = filter.exists("Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                sf::Time elapsed = timer.getElapsedTime();
                tText.setString("Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds");
                tTextContent = "Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds";
                Options.draw(tText);

                fl = true;


            }
            // Обновление tText только если измерение времени было выполнено
            if (fl) {
                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                tText.setString(tTextContent);
                Options.draw(tText);

                filter.remove("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36");

                tTextrem.setString(L"Есть ли удаленная строка?: " + std::to_string(filter.exists("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36")));
                Options.draw(tTextrem);




            }
            // Отображение текста и "кнопки"
            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 2)
        {
            std::map<double, double> diagrammap = calculateCollisions(5000);
            std::map<double, double> abscolli = calculateCollisionsABS(5000);
            sf::Text titul, title;

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;
            std::vector <sf::Text> abscollisions;

            int columnIndex = 0;
            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + columnIndex * 300, 790 - columnHeight);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::White); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                sf::Text prob;

                prob.setFont(font1);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Yellow); // Цвет текста
                prob.setString("P = " + std::to_string(pow((1 - exp(-pair.first * 1000 / 50000)), pair.first))); // Текст подписи
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);


                columnIndex++;
                if (columnIndex >= 5) break;
            }

            int x = 348, y = 860;
            for (const auto& pair : abscolli)
            {


                // Создание подписи для столбца
                sf::Text abscollision;
                abscollision.setFont(font);
                abscollision.setCharacterSize(24); // Размер шрифта
                abscollision.setFillColor(sf::Color::White); // Цвет текста
                abscollision.setString(std::to_string(std::ceil(pair.first)) + " - " + std::to_string(std::ceil(pair.second))); // Текст подписи
                abscollision.setPosition(x, y); // Позиционирование подписи под столбцом
                abscollisions.push_back(abscollision);
                x += 300;


            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n[Вероятность ложноположительного срабатывания] \n[Число хеш-функций] - [Абсолютное количество коллизий]");
            titul.setPosition(700, 250);
            title.setFont(font);
            title.setCharacterSize(50); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"    Зависимость уровня коллизий от числа хеш-функций \nСравнение вероятностей ложноположительного результата");
            title.setPosition(Options.getSize().x / 2 - 650, 120);
            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }
            for (const auto& abscollision : abscollisions)
            {
                Options.draw(abscollision);
            }



            Options.draw(titul);
            Options.draw(title);
            // Отображение текста

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 3)
        {
            sf::Text titul, title;
            std::map<int, int> diagrammap = calculateCollisions_size(3);

            sf::Text info;
            sf::Text tp;
            tp.setFont(font);
            tp.setCharacterSize(35); // Размер шрифта
            tp.setFillColor(sf::Color::Red);
            tp.setPosition(245, 703);
            tp.setString("P =");

            info.setFont(font);
            info.setCharacterSize(30); // Размер шрифта
            info.setFillColor(sf::Color::Green); // Цвет текста
            info.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + L" : оптимальный размер ФБ для получения ложноположительного срабатывания с P = 0.001, n = 1000");
            info.setPosition(450, 840);

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;

            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture;
            if (!gifTexture.loadFromFile("image/white.jpg"))
            {
                return;
            }

            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSprite(gifTexture);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник
            gifSprite.setScale(1800.0f / gifTexture.getSize().x, 400.0f / gifTexture.getSize().y);
            gifSprite.setPosition(200, 370);

            // Создание прямоугольника
            sf::RectangleShape rectangle(sf::Vector2f(1800, 400));
            rectangle.setPosition(200, 370);
            rectangle.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle.setOutlineThickness(5); // Установка толщины рамки
            rectangle.setOutlineColor(sf::Color::Red); // Установка цвета рамки



            // Создание прямоугольника для формулы m
            sf::RectangleShape rectangle4(sf::Vector2f(210, 100));
            rectangle4.setPosition(200, 800);
            rectangle4.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle4.setOutlineThickness(5); // Установка толщины рамки
            rectangle4.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture2;
            if (!gifTexture2.loadFromFile("image/f3.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef(gifTexture2);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef.setPosition(200, 800);


            // Создание прямоугольника для формулы оптимальной p
            sf::RectangleShape rectangle5(sf::Vector2f(311, 76));
            rectangle5.setPosition(1200, 550);
            rectangle5.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle5.setOutlineThickness(5); // Установка толщины рамки
            rectangle5.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture3;
            if (!gifTexture3.loadFromFile("image/f_optimalHF.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef2(gifTexture3);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef2.setPosition(1200, 550);


            int columnIndex = 0;
            int f = 0;

            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.second / 25;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + columnIndex * 300, 1000 - columnHeight - 50 - 80 - 50 - 50 - 100);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::Black); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                double probValue = pow((1 - exp(-3.0 * 1000.0 / pair.first)), 3);


                sf::Text prob;
                prob.setFont(font);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Red); // Цвет текста
                prob.setString(std::to_string(probValue)); // Текст подписи

                prob.setPosition(column.getPosition().x, label.getPosition().y + 30); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);

                columnIndex++;



            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            titul.setPosition(750, 300);
            title.setFont(font);
            title.setCharacterSize(40); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title.setPosition(Options.getSize().x / 2 - 600, 120);


            // Отрисовка прямоугольника
            Options.draw(rectangle);
            Options.draw(gifSprite);
            Options.draw(rectangle4);
            Options.draw(gifSpritef);
            Options.draw(rectangle5);
            Options.draw(gifSpritef2);


            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }


            // Отображение отрисованного на экране

            Options.draw(info);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(tp);


            // Отображение текста и "кнопок" - указателей

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }


        Options.display();
    }

}


void InvertibleBF()
{
    RenderWindow Options(VideoMode::getDesktopMode(), L"Настройки", Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::Font font1;
    if (!font1.loadFromFile("fonts/timesbd.ttf")) exit(1);
    RectangleShape background_opt(Vector2f(Options.getSize().x, Options.getSize().y));
    Texture texture_opt;
    if (!texture_opt.loadFromFile("image/InvBF.jpg")) exit(2);

    sf::Texture texture;
    if (!texture.loadFromFile("image/button1.png"))
    {
        return;
    }

    sf::Texture texture2;
    if (!texture2.loadFromFile("image/button2.png"))
    {
        return;
    }

    sf::Texture texture3;
    if (!texture3.loadFromFile("image/button3.png"))
    {
        return;
    }

    sf::Texture texture4;
    if (!texture4.loadFromFile("image/button4.png"))
    {
        return;
    }

    sf::Texture back_texture;
    if (!back_texture.loadFromFile("image/back_button2.png"))
    {
        return;
    }

    sf::Sprite sprite1(texture);
    sprite1.setPosition(100, 1130); // Установка позиции спрайта

    sf::Sprite sprite2(texture2);
    sprite2.setPosition(350, 1130);

    sf::Sprite sprite3(texture3);
    sprite3.setPosition(600, 1130);

    sf::Sprite sprite4(texture4);
    sprite4.setPosition(850, 1130);

    sf::Sprite back_sprite(back_texture);
    back_sprite.setPosition(95, 975);


    sf::Text b1label;
    b1label.setFont(font);
    b1label.setCharacterSize(20);
    b1label.setFillColor(sf::Color::White);
    b1label.setPosition(170, 1300); // Установка позиции текста
    b1label.setString(L"Назад");

    sf::Text b2label;
    b2label.setFont(font);
    b2label.setCharacterSize(20);
    b2label.setFillColor(sf::Color::White);
    b2label.setPosition(430, 1300); // Установка позиции текста
    b2label.setString(L"CSV*");

    sf::Text b3label;
    b3label.setFont(font);
    b3label.setCharacterSize(20);
    b3label.setFillColor(sf::Color::White);
    b3label.setPosition(650, 1300); // Установка позиции текста
    b3label.setString(L"HF - C-count*");

    sf::Text b4label;
    b4label.setFont(font);
    b4label.setCharacterSize(20);
    b4label.setFillColor(sf::Color::White);
    b4label.setPosition(895, 1300); // Установка позиции текста
    b4label.setString(L"SBF - C-count*");



    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    sf::Text rect_text1;
    rect_text1.setFont(font);
    rect_text1.setCharacterSize(24);
    rect_text1.setFillColor(sf::Color::White);
    rect_text1.setPosition(1180, 1090); // Позиция слева сверху
    rect_text1.setString(L"[1] - СSV* - открыть файл на 1000 строк и проверить работу фильтра блума");

    sf::Text rect_text2;
    rect_text2.setFont(font);
    rect_text2.setCharacterSize(24);
    rect_text2.setFillColor(sf::Color::White);
    rect_text2.setPosition(1180, 1120); // Позиция слева сверху
    rect_text2.setString(L"");

    sf::Text rect_text3;
    rect_text3.setFont(font);
    rect_text3.setCharacterSize(24);
    rect_text3.setFillColor(sf::Color::White);
    rect_text3.setPosition(1180, 1150); // Позиция слева сверху
    rect_text3.setString(L"");



    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения гиф-анимации
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);





    // Таймер для обновления слова каждую секунду
    sf::Clock clock;

    // Создание текстового объекта для отображения счетчика
    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(200, 900); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text isText;
    isText.setFont(font);
    isText.setCharacterSize(24);
    isText.setFillColor(sf::Color::White);
    isText.setPosition(200, 850); // Позиция слева сверху

    // Создание текстового объекта для отображения числа хеш-функций
    sf::Text kfText;
    kfText.setFont(font);
    kfText.setCharacterSize(24);
    kfText.setFillColor(sf::Color::Green);
    kfText.setPosition(200, 800); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tText;
    tText.setFont(font);
    tText.setCharacterSize(24);
    tText.setFillColor(sf::Color::Green);
    tText.setPosition(200, 925); // Позиция слева сверху
    std::string tTextContent;

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tTextrem;
    tTextrem.setFont(font);
    tTextrem.setCharacterSize(24);
    tTextrem.setFillColor(sf::Color::Red);
    tTextrem.setPosition(200, 955); // Позиция слева сверху


    sf::Text tTextremID;
    tTextremID.setFont(font);
    tTextremID.setCharacterSize(24);
    tTextremID.setFillColor(sf::Color::White);
    tTextremID.setPosition(200, 975); // Позиция слева сверху

    std::deque<std::string> lines;
    std::string line;
    int counter = 0;
    int maxLines = 20; // Максимальное количество строк, которые будут отображаться одновременно
    std::vector<sf::Text> linesText(maxLines);
    for (auto& text : linesText)
    {
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
    }

    background_opt.setTexture(&texture_opt);

    std::ifstream file("1000_SalesRecords.csv");
    if (!file.is_open())
    {
        std::cerr << "Unable to open file" << std::endl;
        return;
    }

    //переменная для отслеживания ввода

    int startDisplaying = 0;
    bool fl = 0;
    bool alllinesAdded = 0;
    size_t filterSize = 10000;
    InvertibleBloomFilter<std::string> filter(filterSize);
    filter.addHashFunction(hashFunction1);
    filter.addHashFunction(hashFunction2);
    std::unordered_set<std::string> addedLines;
    std::vector<std::string> uniqueLines; // Контейнер для хранения уникальных строк


    while (Options.isOpen())
    {
        sf::Event event;

        while (Options.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) Options.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) Options.close();
                if (event.key.code == sf::Keyboard::Num1)
                {
                    // Проверка нажатия кнопки 1
                    startDisplaying = 1; // Изменение состояния на начало вывода строк
                    filter.addHashFunction(murmurHash);

                }
                else if (event.key.code == sf::Keyboard::Num2)
                {
                    startDisplaying = 2;

                }
                else if (event.key.code == sf::Keyboard::Num3)
                {
                    startDisplaying = 3;
                }
            }

        }

        Options.clear();
        Options.draw(background_opt);

        if (startDisplaying == 1)
        {

            if (std::getline(file, line)) {
                filter.insert(line); // Добавление строки в фильтр Блума
                counter++;
                if (counter % 10000 == 0) {
                    counterText.setString(std::to_string(counter) + " added");
                    Options.draw(counterText);
                }

                // Проверяем, была ли строка уже добавлена
                if (addedLines.find(line) == addedLines.end()) {
                    // Если строка еще не была добавлена, добавляем ее в уникальные строки
                    uniqueLines.push_back(line);
                    addedLines.insert(line);
                }
            }
            else {
                // Если достигнут конец файла, закрываем его и выходим из цикла
                file.close();
                alllinesAdded = true;
            }

            // Обновляем linesText только с уникальными строками
            for (int i = 0; i < maxLines && i < uniqueLines.size(); ++i) {
                linesText[i].setString(uniqueLines[i]);
                linesText[i].setPosition(200, 200 + i * 30); // Пример позиции, сдвигаем вниз
            }

            // Отрисовка текстовых объектов
            for (const auto& text : linesText) {
                Options.draw(text);
                counterText.setString(std::to_string(counter) + " added");
                Options.draw(counterText);
            }



            // Проверка на наличие слова "apple" после добавления всех строк
            if (alllinesAdded && !fl)
            {

                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                sf::Clock timer;
                bool exists = filter.exists("Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                sf::Time elapsed = timer.getElapsedTime();
                tText.setString("Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds");
                tTextContent = "Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds";
                Options.draw(tText);

                fl = true;


            }
            // Обновление tText только если измерение времени было выполнено
            if (fl) {
                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.exists("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                tText.setString(tTextContent);
                Options.draw(tText);

                filter.remove("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36");

               // tTextrem.setString(L"Есть ли удаленная строка?: " + std::to_string(filter.exists("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36")));

                tTextrem.setString(L"Индексы строки: 'Europe,Portugal,Cereal,Offline,C,4/10/2014...' :" + vectorToString(filter.getIndices("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));

                tTextremID.setString(L"Строка по индексу '2059' :" + filter.getValue(2059));



                Options.draw(tTextrem);
                Options.draw(tTextremID);




            }
            // Отображение текста и "кнопки"
            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 2)
        {
            std::map<double, double> diagrammap = calculateCollisions(5000);
            std::map<double, double> abscolli = calculateCollisionsABS(5000);
            sf::Text titul, title;

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;
            std::vector <sf::Text> abscollisions;

            int columnIndex = 0;
            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + columnIndex * 300, 790 - columnHeight);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::White); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                sf::Text prob;

                prob.setFont(font1);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Yellow); // Цвет текста
                prob.setString("P = " + std::to_string(pow((1 - exp(-pair.first * 1000 / 50000)), pair.first))); // Текст подписи
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);


                columnIndex++;
                if (columnIndex >= 5) break;
            }

            int x = 348, y = 860;
            for (const auto& pair : abscolli)
            {


                // Создание подписи для столбца
                sf::Text abscollision;
                abscollision.setFont(font);
                abscollision.setCharacterSize(24); // Размер шрифта
                abscollision.setFillColor(sf::Color::White); // Цвет текста
                abscollision.setString(std::to_string(std::ceil(pair.first)) + " - " + std::to_string(std::ceil(pair.second))); // Текст подписи
                abscollision.setPosition(x, y); // Позиционирование подписи под столбцом
                abscollisions.push_back(abscollision);
                x += 300;


            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n[Вероятность ложноположительного срабатывания] \n[Число хеш-функций] - [Абсолютное количество коллизий]");
            titul.setPosition(700, 250);
            title.setFont(font);
            title.setCharacterSize(50); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"    Зависимость уровня коллизий от числа хеш-функций \nСравнение вероятностей ложноположительного результата");
            title.setPosition(Options.getSize().x / 2 - 650, 120);
            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }
            for (const auto& abscollision : abscollisions)
            {
                Options.draw(abscollision);
            }



            Options.draw(titul);
            Options.draw(title);
            // Отображение текста

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 3)
        {
            sf::Text titul, title;
            std::map<int, int> diagrammap = calculateCollisions_size(3);

            sf::Text info;
            sf::Text tp;
            tp.setFont(font);
            tp.setCharacterSize(35); // Размер шрифта
            tp.setFillColor(sf::Color::Red);
            tp.setPosition(245, 703);
            tp.setString("P =");

            info.setFont(font);
            info.setCharacterSize(30); // Размер шрифта
            info.setFillColor(sf::Color::Green); // Цвет текста
            info.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + L" : оптимальный размер ФБ для получения ложноположительного срабатывания с P = 0.001, n = 1000");
            info.setPosition(450, 840);

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;

            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture;
            if (!gifTexture.loadFromFile("image/white.jpg"))
            {
                return;
            }

            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSprite(gifTexture);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник
            gifSprite.setScale(1800.0f / gifTexture.getSize().x, 400.0f / gifTexture.getSize().y);
            gifSprite.setPosition(200, 370);

            // Создание прямоугольника
            sf::RectangleShape rectangle(sf::Vector2f(1800, 400));
            rectangle.setPosition(200, 370);
            rectangle.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle.setOutlineThickness(5); // Установка толщины рамки
            rectangle.setOutlineColor(sf::Color::Red); // Установка цвета рамки



            // Создание прямоугольника для формулы m
            sf::RectangleShape rectangle4(sf::Vector2f(210, 100));
            rectangle4.setPosition(200, 800);
            rectangle4.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle4.setOutlineThickness(5); // Установка толщины рамки
            rectangle4.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture2;
            if (!gifTexture2.loadFromFile("image/f3.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef(gifTexture2);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef.setPosition(200, 800);


            // Создание прямоугольника для формулы оптимальной p
            sf::RectangleShape rectangle5(sf::Vector2f(311, 76));
            rectangle5.setPosition(1200, 550);
            rectangle5.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle5.setOutlineThickness(5); // Установка толщины рамки
            rectangle5.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture3;
            if (!gifTexture3.loadFromFile("image/f_optimalHF.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef2(gifTexture3);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef2.setPosition(1200, 550);


            int columnIndex = 0;
            int f = 0;

            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.second / 25;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + columnIndex * 300, 1000 - columnHeight - 50 - 80 - 50 - 50 - 100);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::Black); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                double probValue = pow((1 - exp(-3.0 * 1000.0 / pair.first)), 3);


                sf::Text prob;
                prob.setFont(font);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Red); // Цвет текста
                prob.setString(std::to_string(probValue)); // Текст подписи

                prob.setPosition(column.getPosition().x, label.getPosition().y + 30); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);

                columnIndex++;



            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            titul.setPosition(750, 300);
            title.setFont(font);
            title.setCharacterSize(40); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title.setPosition(Options.getSize().x / 2 - 600, 120);


            // Отрисовка прямоугольника
            Options.draw(rectangle);
            Options.draw(gifSprite);
            Options.draw(rectangle4);
            Options.draw(gifSpritef);
            Options.draw(rectangle5);
            Options.draw(gifSpritef2);


            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }


            // Отображение отрисованного на экране

            Options.draw(info);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(tp);


            // Отображение текста и "кнопок" - указателей

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }


        Options.display();
    }

}



void CuckooBF()
{
    RenderWindow Options(VideoMode::getDesktopMode(), L"Настройки", Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::Font font1;
    if (!font1.loadFromFile("fonts/timesbd.ttf")) exit(1);
    RectangleShape background_opt(Vector2f(Options.getSize().x, Options.getSize().y));
    Texture texture_opt;
    if (!texture_opt.loadFromFile("image/Cbf.jpg")) exit(2);

    sf::Texture texture;
    if (!texture.loadFromFile("image/button1.png"))
    {
        return;
    }

    sf::Texture texture2;
    if (!texture2.loadFromFile("image/button2.png"))
    {
        return;
    }

    sf::Texture texture3;
    if (!texture3.loadFromFile("image/button3.png"))
    {
        return;
    }

    sf::Texture texture4;
    if (!texture4.loadFromFile("image/button4.png"))
    {
        return;
    }

    sf::Texture back_texture;
    if (!back_texture.loadFromFile("image/back_button2.png"))
    {
        return;
    }

    sf::Sprite sprite1(texture);
    sprite1.setPosition(100, 1130); // Установка позиции спрайта

    sf::Sprite sprite2(texture2);
    sprite2.setPosition(350, 1130);

    sf::Sprite sprite3(texture3);
    sprite3.setPosition(600, 1130);

    sf::Sprite sprite4(texture4);
    sprite4.setPosition(850, 1130);

    sf::Sprite back_sprite(back_texture);
    back_sprite.setPosition(95, 975);


    sf::Text b1label;
    b1label.setFont(font);
    b1label.setCharacterSize(20);
    b1label.setFillColor(sf::Color::White);
    b1label.setPosition(170, 1300); // Установка позиции текста
    b1label.setString(L"Назад");

    sf::Text b2label;
    b2label.setFont(font);
    b2label.setCharacterSize(20);
    b2label.setFillColor(sf::Color::White);
    b2label.setPosition(430, 1300); // Установка позиции текста
    b2label.setString(L"CSV*");

    sf::Text b3label;
    b3label.setFont(font);
    b3label.setCharacterSize(20);
    b3label.setFillColor(sf::Color::White);
    b3label.setPosition(650, 1300); // Установка позиции текста
    b3label.setString(L"HF - C-count*");

    sf::Text b4label;
    b4label.setFont(font);
    b4label.setCharacterSize(20);
    b4label.setFillColor(sf::Color::White);
    b4label.setPosition(895, 1300); // Установка позиции текста
    b4label.setString(L"SBF - C-count*");



    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    sf::Text rect_text1;
    rect_text1.setFont(font);
    rect_text1.setCharacterSize(24);
    rect_text1.setFillColor(sf::Color::White);
    rect_text1.setPosition(1180, 1090); // Позиция слева сверху
    rect_text1.setString(L"[1] - СSV* - открыть файл на 1000 строк и проверить работу фильтра блума");

    sf::Text rect_text2;
    rect_text2.setFont(font);
    rect_text2.setCharacterSize(24);
    rect_text2.setFillColor(sf::Color::White);
    rect_text2.setPosition(1180, 1120); // Позиция слева сверху
    rect_text2.setString(L"");

    sf::Text rect_text3;
    rect_text3.setFont(font);
    rect_text3.setCharacterSize(24);
    rect_text3.setFillColor(sf::Color::White);
    rect_text3.setPosition(1180, 1150); // Позиция слева сверху
    rect_text3.setString(L"");



    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения гиф-анимации
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);





    // Таймер для обновления слова каждую секунду
    sf::Clock clock;

    // Создание текстового объекта для отображения счетчика
    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(200, 900); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text isText;
    isText.setFont(font);
    isText.setCharacterSize(24);
    isText.setFillColor(sf::Color::White);
    isText.setPosition(200, 850); // Позиция слева сверху

    // Создание текстового объекта для отображения числа хеш-функций
    sf::Text kfText;
    kfText.setFont(font);
    kfText.setCharacterSize(24);
    kfText.setFillColor(sf::Color::Green);
    kfText.setPosition(200, 800); // Позиция слева сверху

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tText;
    tText.setFont(font);
    tText.setCharacterSize(24);
    tText.setFillColor(sf::Color::Green);
    tText.setPosition(200, 925); // Позиция слева сверху
    std::string tTextContent;

    // Создание текстового объекта для отображения проверки наличия строки
    sf::Text tTextrem;
    tTextrem.setFont(font);
    tTextrem.setCharacterSize(24);
    tTextrem.setFillColor(sf::Color::Red);
    tTextrem.setPosition(200, 955); // Позиция слева сверху


    sf::Text tTextremID;
    tTextremID.setFont(font);
    tTextremID.setCharacterSize(24);
    tTextremID.setFillColor(sf::Color::White);
    tTextremID.setPosition(200, 975); // Позиция слева сверху

    std::deque<std::string> lines;
    std::string line;
    int counter = 0;
    int maxLines = 20; // Максимальное количество строк, которые будут отображаться одновременно
    std::vector<sf::Text> linesText(maxLines);
    for (auto& text : linesText)
    {
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
    }

    background_opt.setTexture(&texture_opt);

    std::ifstream file("1000_SalesRecords.csv");
    if (!file.is_open())
    {
        std::cerr << "Unable to open file" << std::endl;
        return;
    }

    //переменная для отслеживания ввода

    int startDisplaying = 0;
    bool fl = 0;
    bool alllinesAdded = 0;

    size_t filterSize = 10000;
    //CuckooFilter<std::string> filter(filterSize);
    //filter.addHashFunction(hashFunction1);
    //filter.addHashFunction(hashFunction2);
    //filter.addHashFunction(murmurHash);

    ///
    CuckooFilterN filter(filterSize);
    ///

    std::unordered_set<std::string> addedLines;
    std::vector<std::string> uniqueLines; // Контейнер для хранения уникальных строк


    while (Options.isOpen())
    {
        sf::Event event;

        while (Options.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) Options.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) Options.close();
                if (event.key.code == sf::Keyboard::Num1)
                {
                    // Проверка нажатия кнопки 1
                    startDisplaying = 1; // Изменение состояния на начало вывода строк
                    

                }
                else if (event.key.code == sf::Keyboard::Num2)
                {
                    startDisplaying = 2;

                }
                else if (event.key.code == sf::Keyboard::Num3)
                {
                    startDisplaying = 3;
                }
            }

        }

        Options.clear();
        Options.draw(background_opt);

        if (startDisplaying == 1)
        {

            if (std::getline(file, line)) 
            {

                filter.insert(line); // Добавление строки в фильтр Блума
                counter++;
                if (counter % 10000 == 0) {
                    counterText.setString(std::to_string(counter) + " added");
                    Options.draw(counterText);
                }

                // Проверяем, была ли строка уже добавлена
                if (addedLines.find(line) == addedLines.end()) {
                    // Если строка еще не была добавлена, добавляем ее в уникальные строки
                    uniqueLines.push_back(line);
                    addedLines.insert(line);
                }
            }
            else 
            {
                // Если достигнут конец файла, закрываем его и выходим из цикла
                file.close();
                alllinesAdded = true;
            }

            // Обновляем linesText только с уникальными строками
            for (int i = 0; i < maxLines && i < uniqueLines.size(); ++i) {
                linesText[i].setString(uniqueLines[i]);
                linesText[i].setPosition(200, 200 + i * 30); // Пример позиции, сдвигаем вниз
            }

            // Отрисовка текстовых объектов
            for (const auto& text : linesText) {
                Options.draw(text);
                counterText.setString(std::to_string(counter) + " added");
                Options.draw(counterText);
            }



            // Проверка на наличие слова "apple" после добавления всех строк
            if (alllinesAdded && !fl)
            {

                isText.setString(L"Был ли заказ из Португалии в 2014 году?: " + std::to_string(filter.contains("Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52")));
                Options.draw(isText);
                //kfText.setString(L"Число хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                Options.draw(kfText);

                sf::Clock timer;
                bool exists = filter.contains("Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                sf::Time elapsed = timer.getElapsedTime();
                tText.setString("Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds");
                tTextContent = "Check time O(1) [microsec] : " + std::to_string(elapsed.asMicroseconds()) + " microseconds";
                Options.draw(tText);

                fl = true;


            }
            // Обновление tText только если измерение времени было выполнено
            if (fl) {
                isText.setString(L"Был ли заказ из Греции в 2016 году?: " + std::to_string(filter.contains("Europe,Greece,Household,Online,C,11/17/2016,702186715,12/22/2016,1508,668.27,502.54,1007751.16,757830.32,249920.84")));
                Options.draw(isText);
                kfText.setString(L"Число хеш-функций: 3" );
                Options.draw(kfText);

                tText.setString(tTextContent);
                Options.draw(tText);

                filter.remove("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36");

                tTextrem.setString(L"Есть ли удаленная строка?: " + std::to_string(filter.contains("Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36")));




                Options.draw(tTextrem);
               




            }
            // Отображение текста и "кнопки"
            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 2)
        {
            std::map<double, double> diagrammap = calculateCollisions(5000);
            std::map<double, double> abscolli = calculateCollisionsABS(5000);
            sf::Text titul, title;

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;
            std::vector <sf::Text> abscollisions;

            int columnIndex = 0;
            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + columnIndex * 300, 790 - columnHeight);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::White); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                sf::Text prob;

                prob.setFont(font1);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Yellow); // Цвет текста
                prob.setString("P = " + std::to_string(pow((1 - exp(-pair.first * 1000 / 50000)), pair.first))); // Текст подписи
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);


                columnIndex++;
                if (columnIndex >= 5) break;
            }

            int x = 348, y = 860;
            for (const auto& pair : abscolli)
            {


                // Создание подписи для столбца
                sf::Text abscollision;
                abscollision.setFont(font);
                abscollision.setCharacterSize(24); // Размер шрифта
                abscollision.setFillColor(sf::Color::White); // Цвет текста
                abscollision.setString(std::to_string(std::ceil(pair.first)) + " - " + std::to_string(std::ceil(pair.second))); // Текст подписи
                abscollision.setPosition(x, y); // Позиционирование подписи под столбцом
                abscollisions.push_back(abscollision);
                x += 300;


            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n[Вероятность ложноположительного срабатывания] \n[Число хеш-функций] - [Абсолютное количество коллизий]");
            titul.setPosition(700, 250);
            title.setFont(font);
            title.setCharacterSize(50); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"    Зависимость уровня коллизий от числа хеш-функций \nСравнение вероятностей ложноположительного результата");
            title.setPosition(Options.getSize().x / 2 - 650, 120);
            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }
            for (const auto& abscollision : abscollisions)
            {
                Options.draw(abscollision);
            }



            Options.draw(titul);
            Options.draw(title);
            // Отображение текста

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }
        else if (startDisplaying == 3)
        {
            sf::Text titul, title;
            std::map<int, int> diagrammap = calculateCollisions_size(3);

            sf::Text info;
            sf::Text tp;
            tp.setFont(font);
            tp.setCharacterSize(35); // Размер шрифта
            tp.setFillColor(sf::Color::Red);
            tp.setPosition(245, 703);
            tp.setString("P =");

            info.setFont(font);
            info.setCharacterSize(30); // Размер шрифта
            info.setFillColor(sf::Color::Green); // Цвет текста
            info.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + L" : оптимальный размер ФБ для получения ложноположительного срабатывания с P = 0.001, n = 1000");
            info.setPosition(450, 840);

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            std::vector<sf::Text> probs;

            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture;
            if (!gifTexture.loadFromFile("image/white.jpg"))
            {
                return;
            }

            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSprite(gifTexture);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник
            gifSprite.setScale(1800.0f / gifTexture.getSize().x, 400.0f / gifTexture.getSize().y);
            gifSprite.setPosition(200, 370);

            // Создание прямоугольника
            sf::RectangleShape rectangle(sf::Vector2f(1800, 400));
            rectangle.setPosition(200, 370);
            rectangle.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle.setOutlineThickness(5); // Установка толщины рамки
            rectangle.setOutlineColor(sf::Color::Red); // Установка цвета рамки



            // Создание прямоугольника для формулы m
            sf::RectangleShape rectangle4(sf::Vector2f(210, 100));
            rectangle4.setPosition(200, 800);
            rectangle4.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle4.setOutlineThickness(5); // Установка толщины рамки
            rectangle4.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture2;
            if (!gifTexture2.loadFromFile("image/f3.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef(gifTexture2);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef.setPosition(200, 800);


            // Создание прямоугольника для формулы оптимальной p
            sf::RectangleShape rectangle5(sf::Vector2f(311, 76));
            rectangle5.setPosition(1200, 550);
            rectangle5.setFillColor(sf::Color::Blue);

            // Настройка рамки прямоугольника
            rectangle5.setOutlineThickness(5); // Установка толщины рамки
            rectangle5.setOutlineColor(sf::Color::Red); // Установка цвета рамки


            // Загрузка гиф-анимации в текстуру
            sf::Texture gifTexture3;
            if (!gifTexture3.loadFromFile("image/f_optimalHF.png"))
            {
                return;
            }
            // Создание спрайта для отображения гиф-анимации
            sf::Sprite gifSpritef2(gifTexture3);
            // Установка размеров и позиции спрайта для вписывания в прямоугольник

            gifSpritef2.setPosition(1200, 550);


            int columnIndex = 0;
            int f = 0;

            for (const auto& pair : diagrammap)
            {
                double columnHeight = pair.second / 25;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + columnIndex * 300, 1000 - columnHeight - 50 - 80 - 50 - 50 - 100);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                label.setFillColor(sf::Color::Black); // Цвет текста
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                double probValue = pow((1 - exp(-3.0 * 1000.0 / pair.first)), 3);


                sf::Text prob;
                prob.setFont(font);
                prob.setCharacterSize(24); // Размер шрифта
                prob.setFillColor(sf::Color::Red); // Цвет текста
                prob.setString(std::to_string(probValue)); // Текст подписи

                prob.setPosition(column.getPosition().x, label.getPosition().y + 30); // Позиционирование подписи вероятности под столбцом
                probs.push_back(prob);

                columnIndex++;



            }



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            titul.setPosition(750, 300);
            title.setFont(font);
            title.setCharacterSize(40); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title.setPosition(Options.getSize().x / 2 - 600, 120);


            // Отрисовка прямоугольника
            Options.draw(rectangle);
            Options.draw(gifSprite);
            Options.draw(rectangle4);
            Options.draw(gifSpritef);
            Options.draw(rectangle5);
            Options.draw(gifSpritef2);


            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                Options.draw(column);
            }
            for (const auto& label : labels)
            {
                Options.draw(label);
            }
            for (const auto& prob : probs)
            {
                Options.draw(prob);
            }


            // Отображение отрисованного на экране

            Options.draw(info);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(titul);
            Options.draw(title);
            Options.draw(tp);


            // Отображение текста и "кнопок" - указателей

            Options.draw(rectangle1);
            Options.draw(gifSprite1);
            Options.draw(rect_text1);
            Options.draw(rect_text2);
            Options.draw(rect_text3);
            Options.draw(back_sprite);
            Options.draw(sprite1);
            Options.draw(sprite2);
            Options.draw(sprite3);
            Options.draw(sprite4);
            Options.draw(b1label);
            Options.draw(b2label);
            Options.draw(b3label);
            Options.draw(b4label);
        }


        Options.display();
    }

}


//Узел skip-list
class SkipNode 
{
public:
    int value;
    SkipNode** forward;
    SkipNode(int level, int& value) 
    {
        forward = new SkipNode * [level + 1];
        memset(forward, 0, sizeof(SkipNode*) * (level + 1));
        this->value = value;
    }
    ~SkipNode() {
        delete[] forward;
    }
};

class SkipList {
    int MAXLVL;
    float P;
    int level;
    SkipNode* header;
public:
    SkipList(int max_level, float p) 
    {
        MAXLVL = max_level;
        P = p;
        level = 0;
        header = new SkipNode(MAXLVL, *(new int(-1)));
    }
    ~SkipList() {
        delete header;
    }

    int randomLevel() {
        float r = (float)rand() / RAND_MAX;
        int lvl = 0;
        while (r < P && lvl < MAXLVL) {
            lvl++;
            r = (float)rand() / RAND_MAX;
        }
        return lvl;
    }
    SkipNode* createNode(int& value, int level) {
        SkipNode* n = new SkipNode(level, value);
        return n;
    }
    void insertElement(int& value) {
        SkipNode* current = header;
        // const int size = MAXLVL + 1;
        SkipNode* update[1000];
        memset(update, 0, sizeof(SkipNode*) * (MAXLVL + 1));
        for (int i = level; i >= 0; i--) {
            while (current->forward[i] != nullptr && current->forward[i]->value < value) {
                current = current->forward[i];
            }
            update[i] = current;
        }
        current = current->forward[0];
        if (current == nullptr || current->value != value) {
            int rlevel = randomLevel();
            if (rlevel > level) {
                for (int i = level + 1; i < rlevel + 1; i++) {
                    update[i] = header;
                }
                level = rlevel;
            }
            SkipNode* n = createNode(value, rlevel);
            for (int i = 0; i <= rlevel; i++) {
                n->forward[i] = update[i]->forward[i];
                update[i]->forward[i] = n;
            }
        }
    }
    void displayList() 
    {
        std::cout << "\nSkip list level wise\n";
        for (int i = 0; i <= level; i++) {
            SkipNode* node = header->forward[i];
            std::cout << "Level " << i << ": ";
            while (node != nullptr) {
                std::cout << node->value << " ";
                node = node->forward[i];
            }
            std::cout << "\n";
        }
    }
};

// Функция для генерации случайного числа
int generateRandomNumber(int min, int max) 
{
    return rand() % (max - min + 1) + min;
}

// Функция для генерации случайной строки
std::string generateRandomString(int length) {
    std::string result;
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < length; ++i) {
        result += charset[generateRandomNumber(0, sizeof(charset) - 2)];
    }
    return result;
}

void SQL_Bloom()
{
    sf::RenderWindow SQLM(sf::VideoMode::getDesktopMode(), L"SQL", sf::Style::Fullscreen);
    sf::RectangleShape background_ab(sf::Vector2f(sf::VideoMode::getDesktopMode().width, sf::VideoMode::getDesktopMode().height));
    sf::Texture texture_ab;
    if (!texture_ab.loadFromFile("image/sqlm.jpg")) exit(3);
    background_ab.setTexture(&texture_ab);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) {
        std::cerr << "Не удалось загрузить шрифт" << std::endl;
        return;
    }

    int startDisplaying = 0;
    

    std::string line;
    int counter = 0;
    int maxLines = 30; // Максимальное количество строк, которые будут отображаться одновременно
    std::vector<sf::Text> linesText(maxLines);
    for (auto& text : linesText) {
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::White);
    }


    // Создание текстового объекта для отображения счетчика строк
    sf::Text counterText;
    counterText.setFont(font);
    counterText.setCharacterSize(24);
    counterText.setFillColor(sf::Color::White);
    counterText.setPosition(200, 1340); 

    sf::Text text_abscollision;
    text_abscollision.setFont(font);
    text_abscollision.setCharacterSize(24);
    text_abscollision.setFillColor(sf::Color::White);
    text_abscollision.setPosition(180, 1305); 


    sf::Text thf;
    thf.setFont(font);
    thf.setCharacterSize(24);
    thf.setFillColor(sf::Color::White);
    thf.setPosition(180, 1270); 

    sf::Text tpf;
    tpf.setFont(font);
    tpf.setCharacterSize(24);
    tpf.setFillColor(sf::Color::Green);
    tpf.setPosition(180, 1235);

    sf::Text tff;
    tff.setFont(font);
    tff.setCharacterSize(24);
    tff.setFillColor(sf::Color::White);
    tff.setPosition(180, 1200);


    BloomFilter<std::string> sqlfilter(10000);
    sqlfilter.addHashFunction(hashFunction1);
    sqlfilter.addHashFunction(hashFunction2);
    sqlfilter.addHashFunction(murmurHash);


    sf::Text rect_text1;
    rect_text1.setFont(font);
    rect_text1.setCharacterSize(24);
    rect_text1.setFillColor(sf::Color::White);
    rect_text1.setPosition(1170, 1090); // Позиция слева сверху
    rect_text1.setString(L"[1] - SQL* - создать таблицу в БД на 1000 строк и проверить работу фильтра блума");

    sf::Text rect_text2;
    rect_text2.setFont(font);
    rect_text2.setCharacterSize(24);
    rect_text2.setFillColor(sf::Color::White);
    rect_text2.setPosition(1170, 1120); // Позиция слева сверху
    rect_text2.setString(L"[2] - P - HF-count* - диаграмма сравнения зависимости P от числа хеш-функций, \nMURMUR-hash, SHA256, MD5, кольцевой хеш + вероятность P при оптимальных ХФ");

    sf::Text rect_text3;
    rect_text3.setFont(font);
    rect_text3.setCharacterSize(24);
    rect_text3.setFillColor(sf::Color::White);
    rect_text3.setPosition(1180, 1150); // Позиция слева сверху
    rect_text3.setString(L"[] - SBF - C-count* - диаграмма сравнения числа коллизий между ФБ с \nразными размерами");

    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения гиф-анимации
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);

    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки

    sf::Texture texture;
    if (!texture.loadFromFile("image/button1.png"))
    {
        return;
    }

    sf::Texture texture2;
    if (!texture2.loadFromFile("image/button2.png"))
    {
        return;
    }

    sf::Texture texture3;
    if (!texture3.loadFromFile("image/button3.png"))
    {
        return;
    }

    sf::Texture texture4;
    if (!texture4.loadFromFile("image/button4.png"))
    {
        return;
    }

    sf::Texture back_texture;
    if (!back_texture.loadFromFile("image/back_button2.png"))
    {
        return;
    }

    sf::Sprite sprite1(texture);
    sprite1.setPosition(100, 1130); // Установка позиции спрайта

    sf::Sprite sprite2(texture2);
    sprite2.setPosition(350, 1130);

    sf::Sprite sprite3(texture3);
    sprite3.setPosition(600, 1130);

    sf::Sprite sprite4(texture4);
    sprite4.setPosition(850, 1130);

    sf::Sprite back_sprite(back_texture);
    back_sprite.setPosition(95, 975);

    sf::Texture khf_texture;
    if (!khf_texture.loadFromFile("image/f_optimalHF.png"))
    {
        return;
    }
    sf::Sprite khf(khf_texture);
    khf.setPosition(1140, 855);
    // Создание прямоугольника
    sf::RectangleShape rectanglek(sf::Vector2f(311, 76));
    rectanglek.setPosition(1140, 855);
    rectanglek.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectanglek.setOutlineThickness(5); // Установка толщины рамки
    rectanglek.setOutlineColor(sf::Color::Red); // Установка цвета рамки


    sf::Text b1label;
    b1label.setFont(font);
    b1label.setCharacterSize(20);
    b1label.setFillColor(sf::Color::White);
    b1label.setPosition(170, 1300); // Установка позиции текста
    b1label.setString(L"Назад");

    sf::Text b2label;
    b2label.setFont(font);
    b2label.setCharacterSize(20);
    b2label.setFillColor(sf::Color::White);
    b2label.setPosition(430, 1300); // Установка позиции текста
    b2label.setString(L"SQL*");

    sf::Text b3label;
    b3label.setFont(font);
    b3label.setCharacterSize(20);
    b3label.setFillColor(sf::Color::White);
    b3label.setPosition(650, 1300); // Установка позиции текста
    b3label.setString(L"P - HF-count*");

    

    while (SQLM.isOpen())
    {
        sf::Event event;

        while (SQLM.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) SQLM.close();
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) SQLM.close();
                if (event.key.code == sf::Keyboard::Num1)
                {
                    startDisplaying = 1;
                }
                if (event.key.code == sf::Keyboard::Num2)
                {
                    startDisplaying = 3;
                }
            }
        }

        SQLM.clear();
        SQLM.draw(background_ab);

        if (startDisplaying == 1)
        {
            
            sqlite3* db;
            int rc = sqlite3_open("datausers.db", &db);

            // Запрос для выборки последних 20 строк из таблицы
            const char* sql = "SELECT * FROM users2 ORDER BY id DESC LIMIT 30;";
            sqlite3_stmt* stmt2;
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, nullptr);
            if (rc != SQLITE_OK)
            {
                std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_close(db);
                return;
            }

            // Выполнение запроса и извлечение данных
            int row = 0;
            while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW)
            {
                std::stringstream ss; // Используем stringstream для сборки строки
                int columns = sqlite3_column_count(stmt2);
                for (int i = 0; i < columns; i++) {
                    const unsigned char* column_text = sqlite3_column_text(stmt2, i);
                    if (column_text) {
                        ss << reinterpret_cast<const char*>(column_text);
                        if (i < columns - 1) {
                            ss << " , "; // Добавляем разделитель между столбцами, если это не последний столбец
                        }
                    }
                }
                std::string line = ss.str(); // Получаем собранную строку

                // Обновление позиций текстовых объектов
                if (row < maxLines) 
                {
                    linesText[row].setString(line);
                    linesText[row].setPosition(180, 170 + row * 30); // Пример позиции, сдвигаем вниз
                    SQLM.draw(linesText[row]);
                }
                row++;
            }

            if (rc != SQLITE_DONE)
            {
                std::cerr << "Ошибка выполнения запроса: " << sqlite3_errmsg(db) << std::endl;
            }

            // SQL-запрос для выборки всех строк из таблицы
            sql = "SELECT name FROM users2;";


            // Подготовка запроса
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_close(db);
                return;
            }

            // Вставка элементов в фильтр Блума
            while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW) {
                std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0));
                sqlfilter.insert(name);
            }

            if (rc != SQLITE_DONE)
            {
                std::cerr << "Ошибка выполнения запроса: " << sqlite3_errmsg(db) << std::endl;
            }

          

            
            // Пример использования фильтра Блума
            std::string testName = "User29"; // Пример имени для проверки
            if (sqlfilter.exists(testName))
            {
                tff.setString(L"Пользователь User29 есть ли в списке? - 1");
            }
            else
            {
                tff.setString(L"Пользователь User29 есть ли в списке? - 0");
            }

            
            float prob = pow((1.0 - exp(-3000.0 / 10000.0)), 3.0);
            text_abscollision.setString(L"Абсолютное число коллизий: " + std::to_string(sqlfilter.retFullCollisions()));
            thf.setString(L"Число хеш-функций: " + std::to_string(sqlfilter.getHashFunctionsCount()));
            tpf.setString(L"P := [1 - e^(-k*n/m)] = " + std::to_string(prob));

            sqlite3_finalize(stmt2);
            sqlite3_close(db);

            SQLM.draw(text_abscollision);
            SQLM.draw(thf);
            SQLM.draw(tpf);
            SQLM.draw(tff);


            
        }
        else if (startDisplaying == 3)
        {

            std::map<int, double > my_p_h;
            
            for (double i = 2; i <= 10; i++)
            {
                double p = pow((1 - exp(-i * 1000 / 10000)), i);
                my_p_h[i] = p;
            }
            
            sf::Text titul, title;

            // Создание столбцов
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels; // Для хранения подписей
            

            int columnIndex = 0;
            for (const auto& pair : my_p_h)
            {
                double columnHeight = pair.second * 17000;
                sf::RectangleShape column(sf::Vector2f(120, columnHeight));
                if(columnIndex == 5)column.setFillColor(sf::Color::Black);
                else
                column.setFillColor(sf::Color::Green);
                column.setPosition(250 + columnIndex * 200, 790 - columnHeight);
                columns.push_back(column);

                // Создание подписи для столбца
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24); // Размер шрифта
                if(columnIndex == 5)
                    label.setFillColor(sf::Color::Green); // Цвет текста
                else label.setFillColor(sf::Color::White);
                label.setString(std::to_string(pair.first) + " - " + std::to_string(pair.second)); // Текст подписи
                label.setPosition(column.getPosition().x, column.getPosition().y + columnHeight + 10); // Позиционирование подписи под столбцом
                labels.push_back(label);

                

                columnIndex++;
                
            }

            



            titul.setFont(font);
            titul.setCharacterSize(30); // Размер шрифта
            titul.setFillColor(sf::Color::Yellow); // Цвет текста
            titul.setString(L"     [Число хеш-функций] - [Вероятность ложного срабатывания]");
            titul.setPosition(700, 250);
            title.setFont(font);
            title.setCharacterSize(50); // Размер шрифта
            title.setFillColor(sf::Color::White); // Цвет текста
            title.setString(L"Зависимость вероятности ложного срабатывания  от числа хеш-функций \n        Сравнение вероятностей ложноположительного результата");

            title.setPosition(SQLM.getSize().x / 2 - 650, 120);
            // Отрисовка столбцов
            for (const auto& column : columns)
            {
                SQLM.draw(column);
            }
            for (const auto& label : labels)
            {
                SQLM.draw(label);
            }
            



            SQLM.draw(titul);
            SQLM.draw(title);
            // Отображение текста

            
            SQLM.draw(back_sprite);
            SQLM.draw(sprite1);
            SQLM.draw(sprite2);
            SQLM.draw(sprite3);
           
            SQLM.draw(b1label);
            SQLM.draw(b2label);
            SQLM.draw(b3label);
            SQLM.draw(rectanglek);
            SQLM.draw(khf);
            
            
            }

        
        SQLM.draw(rectangle1);
        SQLM.draw(gifSprite1);
        SQLM.draw(rect_text2);
        SQLM.draw(rect_text1);

        SQLM.display();
    }
}

void ComparisonYBloom()
{

    sf::RenderWindow Play(sf::VideoMode::getDesktopMode(), L"Сравнение Y-дерева и Динамического фильтра Блума", sf::Style::Fullscreen);

    // Загрузка шрифта
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    sf::RectangleShape background_play(sf::Vector2f(Play.getSize().x, Play.getSize().y));
    sf::Texture texture_window;
    if (!texture_window.loadFromFile("image/bloomimg.jpg")) exit(1);
    background_play.setTexture(&texture_window);

    // Создание объекта sf::Text
    sf::Text text;
    // Установка шрифта
    text.setFont(font);
    text.setString(L" [ESC] - вернуться назад \n [1] - визуализация DBF и Y-fast trie \n [2] - вычислить число коллизий и вероятноть ЛПС \n [3] - Диаграмма сравнения по памяти \n [4] - время вставки элемента) \n [5] - диаграмма сравнения по времени поиска элемента \n [A] - добавить слово 'apple' \n [B] - добавить слово 'hello' \n [Q] - добавить случайно-сгенерированное слово на 20 символов");
    text.setCharacterSize(26);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(1180, 1070));

    sf::Text text2;
    text2.setFont(font);
    text2.setCharacterSize(26);
    text2.setFillColor(sf::Color::White);
    text2.setPosition(sf::Vector2f(180, 1090));


    // Создание прямоугольника
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle1.setOutlineThickness(5); // Установка толщины рамки
    rectangle1.setOutlineColor(sf::Color::White); // Установка цвета рамки


    // Загрузка гиф-анимации в текстуру
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png"))
    {
        return;
    }

    // Создание спрайта для отображения фона под текст-управления
    sf::Sprite gifSprite1(gifTexture1);
    // Установка размеров и позиции спрайта для вписывания в прямоугольник
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);


    //Формулы:

    //1 - оптимальное число ХФ

    // Создание прямоугольника
    sf::RectangleShape rectangle2(sf::Vector2f(311, 76));
    rectangle2.setPosition(20000, 15000);
    rectangle2.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle2.setOutlineThickness(5); // Установка толщины рамки
    rectangle2.setOutlineColor(sf::Color::Red); // Установка цвета рамки
    sf::Texture f1;
    if (!f1.loadFromFile("image/f_optimalHF.png"))
    {
        return;
    }
    sf::Sprite Spritef1(f1);
    Spritef1.setPosition(20000, 15000);

    sf::Text f1_optimalhf_text;
    f1_optimalhf_text.setFont(font);
    f1_optimalhf_text.setCharacterSize(30);
    f1_optimalhf_text.setFillColor(sf::Color::Red);
    f1_optimalhf_text.setPosition(530, 170); // Позиция слева сверху

    sf::Text f1_optimalhftitle;
    f1_optimalhftitle.setFont(font);
    f1_optimalhftitle.setCharacterSize(34);
    f1_optimalhftitle.setFillColor(sf::Color::White);
    f1_optimalhftitle.setPosition(350, 30);
    f1_optimalhftitle.setString(L"Динамический фильтр Блума");

    //2 - вероятность ложноположительного срабатывания

    // Создание прямоугольника
    sf::RectangleShape rectangle3(sf::Vector2f(303, 99));
    rectangle3.setPosition(12000, 15000);
    rectangle3.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangle3.setOutlineThickness(5); // Установка толщины рамки
    rectangle3.setOutlineColor(sf::Color::Red); // Установка цвета рамки
    sf::Texture f2;
    if (!f2.loadFromFile("image/f_prob.png"))
    {
        return;
    }
    sf::Sprite Spritef2(f2);
    Spritef2.setPosition(12000, 15000);

    sf::Text f2_prob;
    f2_prob.setFont(font);
    f2_prob.setCharacterSize(30);
    f2_prob.setFillColor(sf::Color::Red);
    f2_prob.setPosition(1540, 185); // Позиция справа

    sf::Text f2_ftitle;
    f2_ftitle.setFont(font);
    f2_ftitle.setCharacterSize(34);
    f2_ftitle.setFillColor(sf::Color::White);
    f2_ftitle.setPosition(1400, 200);
    f2_ftitle.setString(L" Y-fast trie ");


    sf::Texture fvs;
    if (!fvs.loadFromFile("image/vs.png"))
    {
        return;
    }
    sf::Sprite vs(fvs);
    vs.setPosition(11050, 1840);


    sf::Texture diag_size;
    if (!diag_size.loadFromFile("image/diag2_sizes.png"))
    {
        return;
    }
    sf::Sprite diag1(diag_size);
    diag1.setPosition(11050, 1840);
    diag1.setScale(1400.0f / diag_size.getSize().x, 900.0f / diag_size.getSize().y);
    
    // Создание прямоугольника
    sf::RectangleShape rectangled(sf::Vector2f(1400, 900));
    rectangled.setPosition(4000, 13000);
    rectangled.setFillColor(sf::Color::Blue);

    // Настройка рамки прямоугольника
    rectangled.setOutlineThickness(10); // Установка толщины рамки
    rectangled.setOutlineColor(sf::Color::Red); // Установка цвета рамки

    // Создание вектора для хранения квадратов
    std::vector<sf::RectangleShape> squares;
    std::vector<sf::Text> numbers; // Вектор для хранения текстовых объектов

    DynBloomFilter<std::string> filter(-3, 0.001);

    //filter.addHashFunction(hashFunction1);
    //filter.addHashFunction(hashFunction2);
    //filter.addHashFunction(hashSHA256);
    //filter.addHashFunction(murmurHash);

    sf::ConvexShape bigTriangle;
    sf::ConvexShape smallTriangle1;
    sf::ConvexShape smallTriangle2;
    sf::VertexArray line1(sf::Lines, 2);
    sf::VertexArray line2(sf::Lines, 2);
    sf::Text bigTriangleText;
    sf::Text smallTriangle1Text;
    sf::Text smallTriangle2Text;


    while (Play.isOpen())
    {
        sf::Event event;
        while (Play.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                Play.close();
            else if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape) { Play.close(); }
                
                else if (event.key.code == sf::Keyboard::Num1) // 
                {
                    f1_optimalhftitle.setPosition(350, 30);
                    f2_ftitle.setPosition(1400, 200);
                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");
                    // Создание 20 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    // Расчет общего отступа для 20 квадратов с учетом их размеров и желаемого отступа
                    float totalHeight = 20 * 50 + (20 - 1) * 10; // 20 квадратов * 50 пикселей в высоте каждого + 19 отступов * 10 пикселей каждый
                    float startY = (Play.getSize().y - totalHeight) / 2; // Начальная позиция Y для первого квадрата

                    for (int i = 0; i < 20; ++i)
                    {
                        sf::RectangleShape square(sf::Vector2f(50, 50)); // Размер квадрата
                        square.setFillColor(sf::Color::Black); // Цвет квадрата
                        square.setPosition(Play.getSize().x / 2 - Play.getSize().x / 4, startY + i * (50 + 10)); // Позиционирование квадрата с учетом отступа
                        squares.push_back(square);

                        // Создание текстового объекта для каждого квадрата
                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24); // Размер шрифта
                        number.setFillColor(sf::Color::Yellow); // Цвет текста
                        number.setPosition(square.getPosition().x + square.getSize().x / 2 - number.getLocalBounds().width / 2, square.getPosition().y + square.getSize().y / 2 - number.getLocalBounds().height / 2); // Центрирование текста внутри квадрата
                        numbers.push_back(number);
                    }

                    // Создание большого треугольника
                    
                    int t = 1100;
                    int ty = 250;
                    bigTriangle.setPointCount(3);
                    bigTriangle.setPoint(0, sf::Vector2f(400+t, 100+ty)); // Верхняя вершина
                    bigTriangle.setPoint(1, sf::Vector2f(200+t, 500+ty)); // Левая нижняя вершина
                    bigTriangle.setPoint(2, sf::Vector2f(600+t, 500+ty)); // Правая нижняя вершина
                    bigTriangle.setFillColor(sf::Color::White);

                    // Создание маленьких треугольников
                    
                    smallTriangle1.setPointCount(3);
                    smallTriangle1.setPoint(0, sf::Vector2f(200+t, 600+ty)); // Верхняя вершина
                    smallTriangle1.setPoint(1, sf::Vector2f(100+t, 650+ty)); // Левая нижняя вершина
                    smallTriangle1.setPoint(2, sf::Vector2f(300+t, 650+ty)); // Правая нижняя вершина
                    smallTriangle1.setFillColor(sf::Color::Red);

                    
                    smallTriangle2.setPointCount(3);
                    smallTriangle2.setPoint(0, sf::Vector2f(600+t, 600+ty)); // Верхняя вершина
                    smallTriangle2.setPoint(1, sf::Vector2f(500+t, 650+ty)); // Левая нижняя вершина
                    smallTriangle2.setPoint(2, sf::Vector2f(700+t, 650+ty)); // Правая нижняя вершина
                    smallTriangle2.setFillColor(sf::Color::Blue);

                    // Создание линий для соединения нижних вершин маленьких треугольников с верхней вершиной большого треугольника
                   
                    line1[0].position = smallTriangle1.getPoint(0); // Левая нижняя вершина маленького треугольника
                    line1[1].position = bigTriangle.getPoint(1); // Верхняя вершина большого треугольника

                    
                    line2[0].position = smallTriangle2.getPoint(0); // Левая нижняя вершина второго маленького треугольника
                    line2[1].position = bigTriangle.getPoint(2); // Верхняя вершина большого треугольника

                    bigTriangleText.setFont(font);
                    bigTriangleText.setString("x-fast trie");
                    bigTriangleText.setCharacterSize(24);
                    bigTriangleText.setFillColor(sf::Color::Black);
                    // Центрирование текста внутри большого треугольника
                    sf::FloatRect bounds = bigTriangleText.getLocalBounds();
                    bigTriangleText.setPosition(bigTriangle.getPoint(0).x - bounds.width / 2, bigTriangle.getPoint(0).y - bounds.height / 2 + 300);

                    smallTriangle1Text.setFont(font);
                    smallTriangle1Text.setString("O(logM)");
                    smallTriangle1Text.setCharacterSize(24);
                    smallTriangle1Text.setFillColor(sf::Color::White);
                    // Центрирование текста внутри маленького треугольника 1
                    bounds = smallTriangle1Text.getLocalBounds();
                    smallTriangle1Text.setPosition(smallTriangle1.getPoint(0).x - bounds.width / 2, smallTriangle1.getPoint(0).y - bounds.height / 2 + 30);

                  
                    smallTriangle2Text.setFont(font);
                    smallTriangle2Text.setString("O(logM)");
                    smallTriangle2Text.setCharacterSize(24);
                    smallTriangle2Text.setFillColor(sf::Color::White);
                    // Центрирование текста внутри маленького треугольника 2
                    bounds = smallTriangle2Text.getLocalBounds();
                    smallTriangle2Text.setPosition(smallTriangle2.getPoint(0).x - bounds.width / 2, smallTriangle2.getPoint(0).y - bounds.height / 2 + 30);

                    vs.setScale(300.0f / fvs.getSize().x, 300.0f / fvs.getSize().y);
                    vs.setPosition(850, 640);

                }
                else if (event.key.code == sf::Keyboard::A) //apple 
                {


                    filter.insert("apple");
                    std::vector<size_t> idxs = filter.getIndices("apple");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Green); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                    smallTriangle1.setFillColor(sf::Color::Black);
                }
                else if (event.key.code == sf::Keyboard::B) //banana
                {


                    filter.insert("banana");
                    std::vector<size_t> idxs = filter.getIndices("banana");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Red); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                    smallTriangle2.setFillColor(sf::Color::Red);
                }
                else if (event.key.code == sf::Keyboard::Q) //qwertyuiop
                {


                    filter.insert("qwertyjuiceapplemasterpiece");
                    std::vector<size_t> idxs = filter.getIndices("qwertyjuiceapplemasterpiece");

                    // Изменение цвета квадратов и текста для индексов, указанных в idxs
                    for (size_t idx : idxs)
                    {
                        if (idx < squares.size()) // Проверка, чтобы избежать выхода за пределы вектора
                        {
                            squares[idx].setFillColor(sf::Color::Magenta); // Изменение цвета квадрата на зеленый
                            numbers[idx].setString("1"); // Изменение текста на "1"
                        }
                    }
                    bigTriangle.setFillColor(sf::Color(249, 159, 159));
                }
                else if (event.key.code == sf::Keyboard::Num2)
                {
                    bigTriangle.setFillColor(sf::Color::Transparent);
                    smallTriangle1.setFillColor(sf::Color::Transparent);
                    smallTriangle2.setFillColor(sf::Color::Transparent);
                    
                    smallTriangle2Text.setString("");
                    smallTriangle1Text.setString("");
                    bigTriangleText.setString("");
                    line1[0].color = sf::Color::Transparent;
                    line1[1].color = sf::Color::Transparent;

                    line2[0].color = sf::Color::Transparent;
                    line2[1].color = sf::Color::Transparent;

                    f2_ftitle.setPosition(10000, 10000);
                    f1_optimalhftitle.setPosition(10000, 10000);
                    f2_prob.setString("");
                    f1_optimalhf_text.setString("");

                    rectangle2.setPosition(1200, 350);
                    Spritef1.setPosition(1200, 350);

                    //формула для вычисления Р_ЛПС
                    rectangle3.setPosition(1200, 160);
                    Spritef2.setPosition(1200, 160);

                    vs.setPosition(11050, 1840);
                    text2.setString(L" Число коллизий: " + std::to_string(filter.countCollisions()) + L"\n Количество хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                    //с округлением до ближайшего целого числа


                    f1_optimalhf_text.setPosition(1540, 370);
                    f1_optimalhf_text.setString(" = " + std::to_string(0.6931 * 20 / 3) + " ~ " + std::to_string(std::ceil(0.6931 * 20 / 3)));
                    f2_prob.setString(" = " + std::to_string(0.06717));

                }

                else if (event.key.code == sf::Keyboard::Num3)
                {
                    bigTriangle.setFillColor(sf::Color::Transparent);
                    smallTriangle1.setFillColor(sf::Color::Transparent);
                    smallTriangle2.setFillColor(sf::Color::Transparent);
                    f1_optimalhftitle.setPosition(3500, 30);
                    smallTriangle2Text.setString("");
                    smallTriangle1Text.setString("");
                    bigTriangleText.setString("");
                    rectangle2.setPosition(12000, 350);
                    Spritef1.setPosition(12000, 350);

                    //формула для вычисления Р_ЛПС
                    rectangle3.setPosition(12000, 160);
                    Spritef2.setPosition(12000, 160);
                    line1[0].color = sf::Color::Transparent;
                    line1[1].color = sf::Color::Transparent;

                    line2[0].color = sf::Color::Transparent;
                    line2[1].color = sf::Color::Transparent;

                    vs.setPosition(4000, 13000);
                    rectangled.setPosition(400, 110);
                    diag1.setPosition(400, 110);


                    // Создание 20 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    text2.setPosition(200, 1070);
                    text2.setString(L"Dynamic Bloom Filter: 256 bytes \nY-fast trie: 128 bytes \nLinked List из ЛР2 прошлого семестра: 8 bytes");

                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");


                }
                else if (event.key.code == sf::Keyboard::Num4)
                {
                    bigTriangle.setFillColor(sf::Color::Transparent);
                    smallTriangle1.setFillColor(sf::Color::Transparent);
                    smallTriangle2.setFillColor(sf::Color::Transparent);
                    f1_optimalhftitle.setPosition(3500, 30);
                    smallTriangle2Text.setString("");
                    smallTriangle1Text.setString("");
                    bigTriangleText.setString("");
                    rectangle2.setPosition(12000, 350);
                    Spritef1.setPosition(12000, 350);

                    //формула для вычисления Р_ЛПС
                    rectangle3.setPosition(12000, 160);
                    Spritef2.setPosition(12000, 160);
                    line1[0].color = sf::Color::Transparent;
                    line1[1].color = sf::Color::Transparent;

                    line2[0].color = sf::Color::Transparent;
                    line2[1].color = sf::Color::Transparent;

                    vs.setPosition(4000, 13000);
                    rectangled.setPosition(400, 110);
                    diag1.setPosition(400, 110);

                    if (!diag_size.loadFromFile("image/diag1_inserttime.png"))
                    {
                        return;
                    }
                    sf::Sprite diag1(diag_size);


                    // Создание 20 квадратов
                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    text2.setPosition(200, 1070);
                    text2.setString(L"Dynamic Bloom Filter: 46 микросекунд \nY-fast trie: 269 микросекунд \nLinked List из ЛР2 прошлого семестра: 0 микросекунд");

                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");


                    }

                else if (event.key.code == sf::Keyboard::Num5)
                {
                    bigTriangle.setFillColor(sf::Color::Transparent);
                    smallTriangle1.setFillColor(sf::Color::Transparent);
                    smallTriangle2.setFillColor(sf::Color::Transparent);
                    f1_optimalhftitle.setPosition(3500, 30);
                    smallTriangle2Text.setString("");
                    smallTriangle1Text.setString("");
                    bigTriangleText.setString("");
                    rectangle2.setPosition(12000, 350);
                    Spritef1.setPosition(12000, 350);

                    //формула для вычисления Р_ЛПС
                    rectangle3.setPosition(12000, 160);
                    Spritef2.setPosition(12000, 160);
                    line1[0].color = sf::Color::Transparent;
                    line1[1].color = sf::Color::Transparent;

                    line2[0].color = sf::Color::Transparent;
                    line2[1].color = sf::Color::Transparent;

                    vs.setPosition(4000, 13000);
                    rectangled.setPosition(400, 110);
                    diag1.setPosition(400, 110);

                    if (!diag_size.loadFromFile("image/diag3_findtime.png"))
                    {

                        return;
                    }
                    sf::Sprite diag1(diag_size);

                    squares.clear(); // Очистка предыдущих квадратов
                    numbers.clear(); // Очистка предыдущих текстовых объектов

                    text2.setPosition(200, 1070);
                    text2.setString(L"Dynamic Bloom Filter: 1 микросекунда \nY-fast trie: 6 микросекунд \nLinked List из ЛР2 прошлого семестра: 820 микросекунд");

                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");


                    }


            }
            else if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    Play.clear();

                }
            }

        }

        Play.clear();
        Play.draw(background_play);
        Play.draw(rectangle1);
        Play.draw(bigTriangle);
        Play.draw(smallTriangle1);
        Play.draw(smallTriangle2);
        Play.draw(bigTriangleText);
        Play.draw(smallTriangle1Text);
        Play.draw(smallTriangle2Text);
        Play.draw(vs);
        Play.draw(rectangled);
        Play.draw(diag1);
        Play.draw(line1);
        Play.draw(line2);
        Play.draw(gifSprite1);
        Play.draw(rectangle2);
        Play.draw(rectangle3);
        Play.draw(Spritef1);
        Play.draw(Spritef2);
        Play.draw(f1_optimalhf_text);
        Play.draw(f1_optimalhftitle);
        Play.draw(f2_prob);
        Play.draw(f2_ftitle);
        Play.draw(text);
        Play.draw(text2);


        for (const auto& square : squares)
        {
            Play.draw(square); // Отображение квадратов
        }
        for (const auto& number : numbers)
        {
            Play.draw(number); // Отображение текстовых объектов
        }

        Play.display();
    }
}

//Меню для работы после открытия вкладки "Фильтр Блумма"
void BloomMenu()
{
    
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), L"Уровень 1", sf::Style::Fullscreen);
    String name_menu[]{ L"Простая реализация",L"Данные из .CSV",L"SQl", L"Фильтр Блума с подсчетом", L"Инверсивный фильтр Блума",L"Cuckoo фильтр Блума", L"Динамический ФБ vs Y-fast дерево"};
    // получаем текущий размер экрана
    float width = VideoMode::getDesktopMode().width;
    float height = VideoMode::getDesktopMode().height;

    // Устанавливаем фон для графического окна 
    // Создаём прямоугольник
    RectangleShape background(Vector2f(width, height));
    // Загружаем в прямоугольник текстуру с изображением menu9.jpg
    Texture texture_window;
    if (!texture_window.loadFromFile("image/menubloom.jpg"));
    background.setTexture(&texture_window);

    // Устанавливаем шрифт для названия игры
    Font font;
    if (!font.loadFromFile("fonts/times.ttf"));
    // Объект игровое меню
    MainMenu::AppMenu mymenu(window, 950, 300, 7, name_menu, 100, 120);
    // Установка цвета элементов пунктов меню
    mymenu.setColorTextMenu(Color(237, 147, 0), Color::Red, Color::Black);

    // выравнивание по центру пунктов меню 
    mymenu.AlignMenu(2, width);

    while (window.isOpen())
    {
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::KeyReleased)
            {
                
                // События выбра пунктов меню
                // нажати на клавиатуре стрелки вверх
                if (event.key.code == Keyboard::Up) { mymenu.MoveUp(); }
                // нажати на клавиатуре стрелки вниз
                if (event.key.code == Keyboard::Down) { mymenu.MoveDown(); }
                // нажати на клавиатуре клавиши Enter
                if (event.key.code == Keyboard::Return)
                {
                    // Переходим на выбранный пункт меню
                    switch (mymenu.getSelectedMenuNumber())
                    {
                    case 0:BasicBloom();   break;
                    case 1:CSV_BLOOM();     break;
                    case 2:SQL_Bloom();  break;
                    case 3: CountingBF(); break;
                    case 4: InvertibleBF(); break;
                    case 5: CuckooBF(); break;
                    case 6: ComparisonYBloom(); break;
                    case 7: window.close(); break;

                    }

                }
                // Проверка нажатия клавиши Escape
                if (event.key.code == Keyboard::Left)
                {
                    window.close(); // Закрытие окна
                }
            }
        }

        window.clear();
        window.draw(background);

        mymenu.draw();
        window.display();
    }
}


///////////////////////////////////////////////////////////////

// Предположим, что у вас есть класс Node, представляющий узел B-дерева
// Предположим, что у нас есть простой класс Node для B-дерева





// Мьютекс для защиты счетчика строк
//std::mutex mtx;
//int lineCount = 0; // Счетчик строк


// Функция обработчика, которая читает данные из файла и добавляет их в фильтр Блума
void processFile(const std::string& filePath, BloomFilter<std::string>& filter, std::mutex& mutex) 
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) 
    {
        // Предполагается, что каждая строка файла представляет собой отдельный элемент для добавления в фильтр
        std::lock_guard<std::mutex> lock(mutex); // Блокировка для синхронизации доступа к фильтру
        filter.insert(line);
    }
}

//namespace plt = matplotlibcpp;

#include <cstdlib>

int main() 
{
    //plt::plot({ 1,2,3,4 }, "*");
    //plt::show();
    //plt::detail::_interpreter::kill();

    //system("\"D:\\_ВУЗ\\Университет 4 семестр\\Структуры и алгоритмы С++\\CourseLab\\Matplotlibproject\\x64\\Release\\Matplotlibproject.exe\"");
    
    setlocale(LC_ALL, "rus");

    int key_0;
    std::cout << "Фильтр Блума - 1 " << std::endl;
    std::cout << "HyperLoglog - 2 " << std::endl;
    do
    {
    std::cout << "1 - BF, 2 - HLL" << std::endl;
    std::cin >> key_0;
    if (key_0 == 1)
    {

        int fb_num = 0;
        std::cout << "1 - Фильтр Блума [Классика]" << std::endl;
        std::cout << "2 - Фильтр Блума с подсчетом" << std::endl;
        std::cout << "3 - Инверсивный фильтр Блума" << std::endl;
        std::cout << "4 - Сравнение Фильтра Блума с Y-fast trie" << std::endl;
        std::cout << "5 - Cuckoo-фильтр" << std::endl;
        std::cin >> fb_num;

        if (fb_num == 1)
        {
            std::cout << "Выберите режим работы программы: " << std::endl;
            std::cout << "1 - консоль" << std::endl;
            std::cout << "2 - threads" << std::endl;
            std::cout << "3 - интерфейс SFML" << std::endl;

            int key_1;
            std::cin >> key_1;

            if (key_1 == 1)
            {
                int key_2;
                std::cout << "1 - CSV, 2 - SQL" << std::endl;
                std::cin >> key_2;

                if (key_2 == 1)
                {
                    double k = 0.0; // число хеш функций
                    double n = 0.0; //предполагаемое количество элементов в фильтре
                    std::cout << "Bloom Filter" << std::endl;

                    size_t filterSize;
                    std::cout << "Введите размер фильтра Блума: ";
                    std::cin >> filterSize;
                    double m = filterSize;

                    BloomFilter<std::string> filter(filterSize);

                    filter.addHashFunction(hashFunction1);
                    filter.addHashFunction(hashFunction2);
                    k = 2.0;
                    filter.addHashFunction(hashSHA256);
                    filter.addHashFunction(murmurHash);
                    k = 4.0;

                    // Считывание данных из CSV-файла
                    std::cout << "Выберите набор данных:" << std::endl;
                    std::cout << "1 - csv на 100 строк" << std::endl;
                    std::cout << "2 - csv на 100000 строк" << std::endl;
                    std::cout << "3 - csv на 5 миллионов строк" << std::endl;
                    int key_0 = 0;
                    std::cin >> key_0;
                    std::ifstream file;



                    if (key_0 == 1)
                    {
                        file.open("100_SalesRecords.csv");
                        n = 100.0;
                    }
                    else if (key_0 == 2)
                    {
                        file.open("100000_SalesRecords.csv");
                        n = 100000.0;
                    }
                    else if (key_0 == 3)
                    {
                        file.open("5m_SalesRecords.csv");
                        n = 5000000.0;
                    }

                    //std::ifstream file("100_SalesRecords.csv");
                    if (!file.is_open())
                    {
                        std::cerr << "Unable to open file" << std::endl;
                        return 1;
                    }

                    std::string line;
                    int counter = 0;
                    while (std::getline(file, line)) { // Считывание каждой строки целиком
                        filter.insert(line); // Добавление строки в фильтр Блума

                        counter++;

                        if (counter % 10000 == 0)
                        { // Проверка, достигло ли количество слов 10 тысяч
                            std::cout << counter << " добавлены" << std::endl;
                        }
                    }
                    file.close();

                    // Проверка наличия слов в фильтре
                    std::cout << "Exists 'apple': " << filter.exists("apple") << std::endl;
                    std::cout << "Exists 'orange': " << filter.exists("orange") << std::endl;
                    std::cout << "Exists 'DateOrder': " << filter.exists("DateOrder") << std::endl;
                    std::cout << "Exists 'Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit': " << filter.exists("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit") << std::endl;


                    // Захват времени начала выполнения
                    auto start = std::chrono::high_resolution_clock::now();

                    // Выполнение метода
                    bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");

                    // Захват времени окончания выполнения
                    auto end = std::chrono::high_resolution_clock::now();

                    // Вычисление разницы между временем начала и окончания
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                    std::cout << "Exists 'Europe2,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40': " << exists << std::endl;
                    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
                    std::cout << "Размер фильтра блума(байты): " << filter.getSizeInBytes() << std::endl;
                    std::cout << "Коллизии: " << filter.countCollisions() << std::endl;
                    std::cout << "Число использованных хеш функций: " << k << std::endl;

                    double m1 = 10000.0; // Пример значения m
                    double k1 = 2.0; // Пример значения k
                    double n1 = 100.0; // Пример значения n
                    double prob = pow((1 - exp(-k * n / m)), k);

                    double optimal_k_functions = log(2.0) * m / n;
                    std::cout << "Оптимальное число хеш-функций: " << ceil(optimal_k_functions) << std::endl;
                    std::cout << std::fixed << std::setprecision(10) << "Вероятность ложного срабатывания(вернуть 1 при отсутствии элемента) := [1 - e^(-k*n/m)]^k = " << prob << std::endl;




                    std::cout << "Абсюлютное число коллизи: [количетсво ХФ] - [число коллизий]" << std::endl;
                    std::map<double, double> mymap = calculateCollisionsABS(5000);

                    for (const auto& element : mymap)
                    {
                        std::cout << element.first << " - " << element.second << std::endl;
                    }




                }
                else if (key_2 == 2)
                {
                    std::cout << "Введите размер для фильтра Блума: " << std::endl;
                    int m = 0;
                    std::cin >> m;
                    BloomFilter<std::string> bloomfilter(m);
                    double n = m;

                    bloomfilter.addHashFunction(hashFunction1);
                    bloomfilter.addHashFunction(hashFunction2);
                    bloomfilter.addHashFunction(murmurHash);

                    sqlite3* db;
                    int rc = sqlite3_open("datausers.db", &db);

                    if (rc) {
                        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
                        return 0;
                    }
                    else {
                        std::cout << "База данных успешно открыта" << std::endl;
                    }

                    // Создание таблицы, если она ещё не существует
                    const char* createTableSql = "CREATE TABLE IF NOT EXISTS users2 ("
                        "id INTEGER PRIMARY KEY, "
                        "name TEXT, "
                        "phone TEXT, "
                        "column3 TEXT, "
                        "column4 TEXT, "
                        "column5 TEXT, "
                        "column6 TEXT, "
                        "column7 TEXT, "
                        "column8 TEXT, "
                        "column9 TEXT, "
                        "column10 TEXT);";
                    char* errMsg = 0;
                    rc = sqlite3_exec(db, createTableSql, 0, 0, &errMsg);
                    if (rc != SQLITE_OK) {
                        std::cerr << "Ошибка создания таблицы: " << errMsg << std::endl;
                        sqlite3_free(errMsg);
                    }

                    // Проверка, пуста ли таблица
                    const char* checkTableSql = "SELECT COUNT(*) FROM users2;";
                    sqlite3_stmt* stmt;
                    rc = sqlite3_prepare_v2(db, checkTableSql, -1, &stmt, nullptr);
                    if (rc != SQLITE_OK) {
                        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
                        sqlite3_close(db);
                        return 0;
                    }

                    rc = sqlite3_step(stmt);
                    if (rc == SQLITE_ROW) {
                        int count = sqlite3_column_int(stmt, 0);
                        if (count == 0) {
                            // Таблица пуста, вставляем данные
                            srand(time(0)); // Инициализация генератора случайных чисел
                            for (int i = 0; i < 1000; i++) {
                                std::string insertQuery = "INSERT INTO users2 (name, phone, column3, column4, column5, column6, column7, column8, column9, column10) VALUES ('User" + std::to_string(i + 1) + "', '+123456789" + std::to_string(i + 1) + "', 'Data3', 'Data4', 'Data5', 'Data6', 'Data7', 'Data8', 'Data9', 'Data10');";
                                rc = sqlite3_exec(db, insertQuery.c_str(), 0, 0, nullptr);
                                if (rc != SQLITE_OK) {
                                    std::cerr << "Ошибка вставки данных: " << sqlite3_errmsg(db) << std::endl;
                                }
                            }
                        }
                        else {
                            std::cout << "Таблица уже содержит данные, вставка не производится." << std::endl;
                        }
                    }

                    sqlite3_finalize(stmt);

                    // SQL-запрос для выборки всех строк из таблицы
                    const char* sql = "SELECT * FROM users2;";
                    sqlite3_stmt* stmt2;

                    // Подготовка запроса
                    rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, nullptr);
                    if (rc != SQLITE_OK) {
                        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
                        sqlite3_close(db);
                        return 0;
                    }

                    // Выполнение запроса и вывод результатов
                    while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW) {
                        int columns = sqlite3_column_count(stmt2);
                        for (int i = 0; i < columns; i++) {
                            std::cout << sqlite3_column_text(stmt2, i) << " ";
                        }
                        std::cout << std::endl;
                    }

                    if (rc != SQLITE_DONE) {
                        std::cerr << "Ошибка выполнения запроса: " << sqlite3_errmsg(db) << std::endl;
                    }

                    // SQL-запрос для выборки всех строк из таблицы
                    sql = "SELECT name FROM users2;";


                    // Подготовка запроса
                    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
                    if (rc != SQLITE_OK) {
                        std::cerr << "Ошибка подготовки запроса: " << sqlite3_errmsg(db) << std::endl;
                        sqlite3_close(db);
                        return 0;
                    }

                    // Вставка элементов в фильтр Блума
                    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                        bloomfilter.insert(name);
                    }

                    if (rc != SQLITE_DONE)
                    {
                        std::cerr << "Ошибка выполнения запроса: " << sqlite3_errmsg(db) << std::endl;
                    }

                    sqlite3_finalize(stmt);
                    sqlite3_close(db);

                    std::cout << std::endl;
                    // Пример использования фильтра Блума
                    std::string testName = "User29"; // Пример имени для проверки
                    if (bloomfilter.exists(testName))
                    {
                        std::cout << "Элемент " << testName << " возможно присутствует в таблице." << std::endl;
                    }
                    else
                    {
                        std::cout << "Элемент " << testName << " точно отсутствует в таблице." << std::endl;
                    }

                    float prob = pow((1.0 - exp(-3000.0 / n)), 3.0);
                    std::cout << "Абсолютное число коллизий: " << bloomfilter.retFullCollisions() << std::endl;
                    std::cout << "Число хеш-функций: " << bloomfilter.getHashFunctionsCount() << std::endl;
                    std::cout << "Вероятность ложноположительного срабатывания при " << n << " - размере Фильтра Блума := [1 - e^(-k*n/m)]^k = " << prob << std::endl;

                    sqlite3_finalize(stmt2);
                    sqlite3_close(db);
                }


            }

            else if (key_1 == 2)
            {
                // Создание фильтра Блума
                size_t filterSize;
                std::cout << "Введите размер фильтра Блума: ";
                std::cin >> filterSize;

                BloomFilter<std::string> filter1(filterSize);
                filter1.addHashFunction(hashFunction1);
                filter1.addHashFunction(hashFunction2);
                //filter1.addHashFunction(hashSHA256);

                // Список файловых частей для обработки
                std::vector<std::string> fileParts = { "customers-100000_3.csv", "customers-100000_2.csv","customers-100000_4.csv", "customers-100000_5.csv", "customers-100000.csv" };

                //инициализация вектора потоков и мьютекса
                std::vector<std::thread> threads;
                std::mutex mutex;

                //запуск потоков для обработки файлов
                for (const auto& filePath : fileParts) {
                    threads.emplace_back(processFile, filePath, std::ref(filter1), std::ref(mutex));
                }

                //ожидание завершения всех потоков
                for (auto& thread : threads) {
                    thread.join();
                }
                // Проверка наличия слов в фильтре
                std::cout << "Exists 'apple': " << filter1.exists("apple") << std::endl;
                std::cout << "Exists 'orange': " << filter1.exists("orange") << std::endl;
                std::cout << "Exists 'DateOrder': " << filter1.exists("DateOrder") << std::endl;
                std::cout << "Exists '1138,A67B9eeFB8d47eB,Christina,Cunningham,Curtis Inc,Farrellchester,Tajikistan,101-363-4725x504,234.130.0609x126,fmorse@archer.biz,2021-04-01,http://www.craig.org/': " << filter1.exists("1138,A67B9eeFB8d47eB,Christina,Cunningham,Curtis Inc,Farrellchester,Tajikistan,101-363-4725x504,234.130.0609x126,fmorse@archer.biz,2021-04-01,http://www.craig.org/") << std::endl;


                // Захват времени начала выполнения
                auto start = std::chrono::high_resolution_clock::now();

                // Выполнение метода
                bool exists = filter1.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");

                // Захват времени окончания выполнения
                auto end = std::chrono::high_resolution_clock::now();

                // Вычисление разницы между временем начала и окончания
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                std::cout << "Exists 'Europe2,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40': " << exists << std::endl;
                std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
                //std::cout << "Обработано строк: " << lineCount << std::endl;
                std::cout << "Размер фильтра блума(байты): " << filter1.getSizeInBytes() << std::endl;
                std::cout << "Коллизии: " << filter1.countCollisions() << std::endl;
            }

            else if (key_1 == 3)
            {
                SkipList list(3, 0.5);
                list.insertElement(*(new int(3)));
                list.insertElement(*(new int(6)));
                list.insertElement(*(new int(7)));
                list.insertElement(*(new int(9)));
                list.insertElement(*(new int(12)));
                list.insertElement(*(new int(19)));
                list.insertElement(*(new int(17)));
                list.insertElement(*(new int(26)));
                list.insertElement(*(new int(21)));
                list.insertElement(*(new int(25)));
                list.displayList();

                std::cout << "Bloom Filter" << std::endl;

                BloomFilter<std::string> filter(20);
                filter.addHashFunction(hashFunction1);
                filter.addHashFunction(hashFunction2);

                filter.insert("apple");
                filter.insert("banana");
                filter.insert("cherry");

                std::cout << "Exists 'apple': " << filter.exists("apple") << std::endl;
                std::cout << "Exists 'orange': " << filter.exists("orange") << std::endl;
                std::cout << "Exists 'banana': " << filter.exists("banana") << std::endl;

                // Создаём окно windows
                RenderWindow window;
                // Параметры: размер окна установить согласно текущему разрешению экрана
                // название моя игра, развернуть графическое окно на весь размер экрана
                window.create(VideoMode::getDesktopMode(), L"Курсовая работа ", Style::Fullscreen);

                //отключаем видимость курсора
                //window.setMouseCursorVisible(false);

                // получаем текущий размер экрана
                float width = VideoMode::getDesktopMode().width;
                float height = VideoMode::getDesktopMode().height;

                // Устанавливаем фон для графического окна 
                // Создаём прямоугольник
                RectangleShape background(Vector2f(width, height));
                // Загружаем в прямоугольник текстуру с изображением menu9.jpg
                Texture texture_window;
                if (!texture_window.loadFromFile("image/menu9.jpg")) return 4;
                background.setTexture(&texture_window);

                // Устанавливаем шрифт для названия игры
                Font font;
                if (!font.loadFromFile("fonts/times.ttf")) return 5;
                Text Titul1;
                Titul1.setFont(font);
                Text Titul2;
                Titul2.setFont(font);

                // Текст с названием игры
                InitText(Titul1, width, height, L"Реализация, анализ, сравнение", 150, Color(237, 147, 0), 3);
                InitText(Titul2, width, height + 800, L"вероятностных структур данных", 150, Color(237, 147, 0), 3);


                // Название пунктов меню
                
                String name_menu[]{ L"Фильтр Блума",L"HyperLogLog",L"Выход" };

                // Объект игровое меню
                MainMenu::AppMenu mymenu(window, 950, 400, 3, name_menu, 100, 120);

                // Установка цвета элементов пунктов меню
                mymenu.setColorTextMenu(Color(255, 0, 127), Color::Green, Color::Black);

                // выравнивание по центру пунктов меню 
                mymenu.AlignMenu(2, width);

                while (window.isOpen())
                {
                    Event event;
                    while (window.pollEvent(event))
                    {
                        if (event.type == Event::KeyReleased)
                        {
                            // События выбора пунктов меню
                            // нажати на клавиатуре стрелки вверх
                            if (event.key.code == Keyboard::Up) { mymenu.MoveUp(); }
                            // нажати на клавиатуре стрелки вниз
                            if (event.key.code == Keyboard::Down) { mymenu.MoveDown(); }
                            // нажати на клавиатуре клавиши Enter
                            if (event.key.code == Keyboard::Return)
                            {
                                // Переходим на выбранный пункт меню
                                switch (mymenu.getSelectedMenuNumber())
                                {
                                
                                case 0: BloomMenu(); break;
                                case 1:Options();     break;
                                case 2:window.close(); break;

                                }

                            }
                        }
                    }

                    window.clear();
                    window.draw(background);
                    window.draw(Titul1);
                    window.draw(Titul2);
                    mymenu.draw();
                    window.display();
                }
            }
        }
        else if (fb_num == 2)
        {
            double k = 0.0; // число хеш функций
            double n = 0.0; //предполагаемое количество элементов в фильтре
            std::cout << "Counting Bloom Filter" << std::endl;

            size_t filterSize;
            std::cout << "Введите размер ФБ с подсчетом: ";
            std::cin >> filterSize;
           
            double m = filterSize;

            CountingBloomFilter<std::string> filter(filterSize);

            filter.addHashFunction(hashFunction1);
            filter.addHashFunction(hashFunction2);
            k = 2.0;
            filter.addHashFunction(hashSHA256);
            filter.addHashFunction(murmurHash);
            k = 4.0;

            // Считывание данных из CSV-файла
            std::cout << "Выберите набор данных:" << std::endl;
            std::cout << "1 - csv на 100 строк" << std::endl;
            std::cout << "2 - csv на 100000 строк" << std::endl;
            std::cout << "3 - csv на 5 миллионов строк" << std::endl;
            int key_0 = 0;
            std::cin >> key_0;
            std::ifstream file;



            if (key_0 == 1)
            {
                file.open("100_SalesRecords.csv");
                n = 100.0;
            }
            else if (key_0 == 2)
            {
                file.open("100000_SalesRecords.csv");
                n = 100000.0;
            }
            else if (key_0 == 3)
            {
                file.open("5m_SalesRecords.csv");
                n = 5000000.0;
            }

            //std::ifstream file("100_SalesRecords.csv");
            if (!file.is_open())
            {
                std::cerr << "Unable to open file" << std::endl;
                return 1;
            }

            std::string line;
            int counter = 0;
            while (std::getline(file, line)) { // Считывание каждой строки целиком
                filter.insert(line); // Добавление строки в фильтр Блума

                counter++;

                if (counter % 10000 == 0)
                { // Проверка, достигло ли количество слов 10 тысяч
                    std::cout << counter << " добавлены" << std::endl;
                }
            }
            file.close();

            // Проверка наличия слов в фильтре
            std::cout << "Exists 'apple': " << filter.exists("apple") << std::endl;
            std::cout << "Exists 'orange': " << filter.exists("orange") << std::endl;
            std::cout << "Exists 'DateOrder': " << filter.exists("DateOrder") << std::endl;
            std::cout << "Exists 'Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit': " << filter.exists("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit") << std::endl;

            std::cout << "ГЛАВНОЕ ОТЛИЧИЕ ФБ С ПОДСЧЕТОМ: " << std::endl;

            std::cout << "Вы хотите удалить эту строку: Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12" << std::endl;

            bool a;
            std::cout << "1 - да, 0 - нет" << std::endl;
            std::cin >> a;
            if (a)
            {
                filter.remove("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12");
                std::cout << "Exists 'Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12': " << filter.exists("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12") << std::endl;
            }


            // Захват времени начала выполнения
            auto start = std::chrono::high_resolution_clock::now();

            // Выполнение метода
            bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");

            // Захват времени окончания выполнения
            auto end = std::chrono::high_resolution_clock::now();

            // Вычисление разницы между временем начала и окончания
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "Exists 'Europe2,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40': " << exists << std::endl;
            std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
            std::cout << "Размер фильтра блума(байты): " << filter.getSizeInBytes() << std::endl;
            std::cout << "Коллизии: " << filter.countCollisions() << std::endl;
            std::cout << "Число использованных хеш функций: " << k << std::endl;

            double m1 = 10000.0; // Пример значения m
            double k1 = 2.0; // Пример значения k
            double n1 = 100.0; // Пример значения n
            double prob = pow((1 - exp(-k * n / m)), k);

            double optimal_k_functions = log(2.0) * m / n;
            std::cout << "Оптимальное число хеш-функций: " << ceil(optimal_k_functions) << std::endl;
            std::cout << std::fixed << std::setprecision(10) << "Вероятность ложного срабатывания(вернуть 1 при отсутствии элемента) := [1 - e^(-k*n/m)]^k = " << prob << std::endl;




            std::cout << "Абсюлютное число коллизи: [количетсво ХФ] - [число коллизий]" << std::endl;
            std::map<double, double> mymap = calculateCollisionsABS(5000);

            for (const auto& element : mymap)
            {
                std::cout << element.first << " - " << element.second << std::endl;
            }
        }
        else if (fb_num == 3)
        {
            double k = 0.0; // число хеш функций
            double n = 0.0; //предполагаемое количество элементов в фильтре
            std::cout << "Invertible Bloom Filter" << std::endl;

            size_t filterSize;
            std::cout << "Введите размер ИНВЕРСИВНОГО ФБ: ";
            std::cin >> filterSize;

            double m = filterSize;

            InvertibleBloomFilter<std::string> filter(filterSize);

            filter.addHashFunction(hashFunction1);
            filter.addHashFunction(hashFunction2);
            k = 2.0;
            filter.addHashFunction(hashSHA256);
            filter.addHashFunction(murmurHash);
            k = 4.0;

            // Считывание данных из CSV-файла
            std::cout << "Выберите набор данных:" << std::endl;
            std::cout << "1 - csv на 100 строк" << std::endl;
            std::cout << "2 - csv на 100000 строк" << std::endl;
            std::cout << "3 - csv на 5 миллионов строк" << std::endl;
            int key_0 = 0;
            std::cin >> key_0;
            std::ifstream file;



            if (key_0 == 1)
            {
                file.open("100_SalesRecords.csv");
                n = 100.0;
            }
            else if (key_0 == 2)
            {
                file.open("100000_SalesRecords.csv");
                n = 100000.0;
            }
            else if (key_0 == 3)
            {
                file.open("5m_SalesRecords.csv");
                n = 5000000.0;
            }

            //std::ifstream file("100_SalesRecords.csv");
            if (!file.is_open())
            {
                std::cerr << "Unable to open file" << std::endl;
                return 1;
            }

            std::string line;
            int counter = 0;
            while (std::getline(file, line)) { // Считывание каждой строки целиком
                filter.insert(line); // Добавление строки в фильтр Блума

                counter++;

                if (counter % 10000 == 0)
                { // Проверка, достигло ли количество слов 10 тысяч
                    std::cout << counter << " добавлены" << std::endl;
                }
            }
            file.close();

            // Проверка наличия слов в фильтре
            std::cout << "Exists 'apple': " << filter.exists("apple") << std::endl;
            std::cout << "Exists 'orange': " << filter.exists("orange") << std::endl;
            std::cout << "Exists 'DateOrder': " << filter.exists("DateOrder") << std::endl;
            std::cout << "Exists 'Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit': " << filter.exists("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit") << std::endl;

            std::cout << "ГЛАВНОЕ ОТЛИЧИЕ ИНВЕРСИВНОГО ФБ: " << std::endl;

            std::cout << "Вы хотите удалить эту строку: Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12" << std::endl;

            bool a;
            std::cout << "1 - да, 0 - нет" << std::endl;
            std::cin >> a;
            if (a)
            {
                filter.remove("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12");
                std::cout << "Exists 'Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12': " << filter.exists("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12") << std::endl;
            }

            std::cout << std::endl;

            std::cout << "Вернуть вектор индексов этой записи: 'Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit' " << std::endl;

            std::vector<size_t> idxs = filter.getIndices("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit");

            for (const auto& element : idxs) 
            {
                std::cout << element << " ";
            }

            std::cout << std::endl;
            std::cout << std::endl;

            std::cout << "Строка по индексу 192: " << filter.getValue(192) << std::endl;



            // Захват времени начала выполнения
            auto start = std::chrono::high_resolution_clock::now();

            // Выполнение метода
            bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");

            // Захват времени окончания выполнения
            auto end = std::chrono::high_resolution_clock::now();

            // Вычисление разницы между временем начала и окончания
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            std::cout << "Exists 'Europe2,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40': " << exists << std::endl;
            std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
            std::cout << "Размер фильтра блума(байты): " << filter.getSizeInBytes() << std::endl;
            std::cout << "Коллизии: " << filter.countCollisions() << std::endl;
            std::cout << "Число использованных хеш функций: " << k << std::endl;

            double m1 = 10000.0; // Пример значения m
            double k1 = 2.0; // Пример значения k
            double n1 = 100.0; // Пример значения n
            double prob = pow((1 - exp(-k * n / m)), k);

            double optimal_k_functions = log(2.0) * m / n;
            std::cout << "Оптимальное число хеш-функций: " << ceil(optimal_k_functions) << std::endl;
            std::cout << std::fixed << std::setprecision(10) << "Вероятность ложного срабатывания(вернуть 1 при отсутствии элемента) := [1 - e^(-k*n/m)]^k = " << prob << std::endl;




            std::cout << "Абсюлютное число коллизи: [количетсво ХФ] - [число коллизий]" << std::endl;
            std::map<double, double> mymap = calculateCollisionsABS(5000);

            for (const auto& element : mymap)
            {
                std::cout << element.first << " - " << element.second << std::endl;
            }
            }
        else if (fb_num == 4)
        {

            std::cout << "Создание и заполнение YFast trie" << std::endl;

            YfastTrie trie(1 << 5);
            std::cout << "insert 1, 5, 11, 12\n";
            trie.insert(5, 2);
           trie.insert(11, 2);
           trie.insert(12, 2);
            trie.insert(1, 2);


            int timefindY = 0;

            std::cout << "Successor of key 2:\n";
            int tmp = trie.successor(2);
            if (tmp != -1) {
                auto start = std::chrono::high_resolution_clock::now();
                //std::cout << tmp << '\n'<< "value stored = " << trie.exists(tmp) << '\n';
                bool a = trie.exists(tmp);
                    auto stop = std::chrono::high_resolution_clock::now();

                    // Вычислите продолжительность
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

                    // Выведите результат
                    std::cout << "Время проверки элемента в Y-Trie: " << duration.count() << " microseconds" << std::endl;
                     timefindY = duration.count();
            }

            std::cout << "Predecessor of key 13:\n";
            tmp = trie.predecessor(13);
            if (tmp != -1) {
                std::cout << tmp << '\n'
                    << "value stored = " << trie.find(tmp) << '\n';
            }

            auto start1 = std::chrono::high_resolution_clock::now();
            trie.insert(12, 2);
            auto stop1 = std::chrono::high_resolution_clock::now();
            auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
            std::cout << "Время вставки элемента Y-Trie: " << duration1.count() << " microseconds" << std::endl;
            int timeinsertY = duration1.count();


            double probability = 0.001;
            int counters = -4;

            DynBloomFilter<std::string> filter(counters, probability); 

            //filter.addHashFunction(hashFunction1);
            //filter.addHashFunction(hashFunction2);

            filter.insert("1");
            filter.insert("6");
            filter.insert("11");
            filter.insert("12");

            auto start = std::chrono::high_resolution_clock::now();
            bool a = filter.exists("12");
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);



            int timefindDFB = duration.count();
            std::cout << "Время проверки элемента в Динамическом фильтре Блума " << duration.count() << " microseconds" << std::endl;
            std::cout << "Есть ли элемент 12 в ФБ: " << a << std::endl;

            start = std::chrono::high_resolution_clock::now();

            filter.insert("hello dear friend!");

            stop = std::chrono::high_resolution_clock::now();

            duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

            std::cout << "Время вставки элемента в ФБ: " << duration.count() << " microseconds" << std::endl;

            int timeinsertDFB = duration.count();

  
            LinkedList list;
            list.insert(2);
            list.insert(3);
            list.insert(4);
            list.insert(5);

            // Заполнение списка элементами
            start = std::chrono::high_resolution_clock::now();
            list.insert(1);
            stop = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);


            // Выведите результат
            std::cout << "Время вставки элемента в линейный связный список: " << duration.count() << " microseconds" << std::endl;
            int timeinsertList = duration.count();
        

            // Проверка наличия элемента в списке
            start = std::chrono::high_resolution_clock::now();
            std::cout << "Exists 3 in the list? " << (list.exists(3) ? "Yes" : "No") << std::endl;
            stop = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);


            // Выведите результат
            std::cout << "Время проверки наличия элемента в Linked List'e: " << duration.count() << " microseconds" << std::endl;
            int timefindList = duration.count();

            start = std::chrono::high_resolution_clock::now();
            std::cout << "Exists 6 in the list? " << (list.exists(6) ? "Yes" : "No") << std::endl;
            stop = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);


            // Выведите результат
            std::cout << "Время проверки наличия отсутствующего элемента в Linked List'e: " << duration.count() << " microseconds" << std::endl;

            //Вычисляем размеры и записываем в файл
            int sizeDBF = sizeof(filter);
            int sizeYtrie = sizeof(trie);
            int sizeList = sizeof(list);

            std::ofstream myfile;
            myfile.open("diag2.txt");

            if (myfile.is_open()) {
                myfile << "Size of DBF: " << sizeDBF << std::endl;
                myfile << "Size of Ytrie: " << sizeYtrie << std::endl;
                myfile << "Size of List: " << sizeList << std::endl;
                myfile.close();
            }
            else {
                std::cout << "Unable to open file" << std::endl;
            }


            std::cout << "Размер фильтра блума: " << sizeof(filter) << " - Размер Y-trie: " << sizeof(trie) << " - Размер LinkedList: " << sizeof(list) << std::endl;

            sf::RenderWindow window1(sf::VideoMode(1200, 780), "Size-comparison");
            sf::Texture texture1;
            if (!texture1.loadFromFile("image/diag2_sizes.png")) {
                std::cout << "Не удалось загрузить первое изображение" << std::endl;
                return -1;
            }
            sf::Sprite sprite1(texture1);

            // Отображение первого изображения
            while (window1.isOpen()) {
                sf::Event event;
                while (window1.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window1.close();
                    }
                }
                window1.clear();
                window1.draw(sprite1);
                window1.display();
            }

            std::cout << "Время вставки ФБ: " << timeinsertDFB << " - Время вставки Y-дерева " << timeinsertY << " - Время вставки LinkedList " << timeinsertList << std::endl;

            sf::RenderWindow window2(sf::VideoMode(1200, 780), "Insert-comparison");
            sf::Texture texture2;
            if (!texture2.loadFromFile("image/diag1_inserttime.png")) {
                std::cout << "Не удалось загрузить первое изображение" << std::endl;
                return -1;
            }
            sf::Sprite sprite2(texture2);

            // Отображение первого изображения
            while (window2.isOpen()) {
                sf::Event event;
                while (window2.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window2.close();
                    }
                }
                window2.clear();
                window2.draw(sprite2);
                window2.display();
            }

            std::cout << "Время вставки ФБ: " << timefindDFB << " - Время вставки Y-дерева " << timefindY << " - Время вставки LinkedList " << timefindList << std::endl;

            sf::RenderWindow window3(sf::VideoMode(1200, 780), "Find-comparison");
            sf::Texture texture3;
            if (!texture3.loadFromFile("image/diag3_findtime.png")) {
                std::cout << "Не удалось загрузить первое изображение" << std::endl;
                return -1;
            }
            sf::Sprite sprite3(texture3);

            // Отображение первого изображения
            while (window3.isOpen()) {
                sf::Event event;
                while (window3.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window3.close();
                    }
                }
                window3.clear();
                window3.draw(sprite3);
                window3.display();
            }
            
            myfile.open("diag1.txt");

            if (myfile.is_open()) 
            {
                myfile << "Insert time of DBF: " << timeinsertDFB - 5 << std::endl;
                myfile << "Insert time of Ytrie: " << timeinsertY << std::endl;
                myfile << "Insert time of List: " << timeinsertList << std::endl;
                myfile.close();
            }
            else 
            {
                std::cout << "Unable to open file" << std::endl;
            }

            myfile.open("diag3.txt");

            if (myfile.is_open()) 
            {
                myfile << "Find time of DBF: " << timefindDFB << std::endl;
                myfile << "Fins time of Ytrie: " << timefindY<< std::endl;
                myfile << "Find time of List: " << timefindList << std::endl;
                myfile.close();
            }
            else 
            {
                std::cout << "Unable to open file" << std::endl;
            }
        }
        else if (fb_num == 5)
        {
            std::cout << "--- Создание Cuckoo-фильтра ---" << std::endl;
            std::cout << " - Успешно - " << std::endl;
            size_t filterSize = 10000;
            CuckooFilterN filter(filterSize);
            //filter.addHashFunction(hashFunction1);
            //filter.addHashFunction(hashFunction2);
            //filter.addHashFunction(murmurHash);

            // Считывание данных из CSV-файла
            std::cout << "Выберите набор данных:" << std::endl;
            std::cout << "1 - csv на 100 строк" << std::endl;
            std::cout << "2 - csv на 100000 строк" << std::endl;
            std::cout << "3 - csv на 5 миллионов строк" << std::endl;
            int key_0 = 0;
            std::cin >> key_0;
            std::ifstream file;
            

            if (key_0 == 1)
            {
                file.open("100_SalesRecords.csv");
                
            }
            else if (key_0 == 2)
            {
                file.open("100000_SalesRecords.csv");
               
            }
            else if (key_0 == 3)
            {
                file.open("5m_SalesRecords.csv");
                
            }

            //std::ifstream file("100_SalesRecords.csv");
            if (!file.is_open())
            {
                std::cerr << "Unable to open file" << std::endl;
                return 1;
            }

            std::string line;
            int counter = 0;
            while (std::getline(file, line)) { // Считывание каждой строки целиком
                filter.insert(line); // Добавление строки в фильтр Блума

                counter++;

                if (counter % 10000 == 0)
                { // Проверка, достигло ли количество слов 10 тысяч
                    std::cout << counter << " добавлены" << std::endl;
                }
            }
            file.close();

            //filter.print();

            // Проверка наличия слов в фильтре
            std::cout << "Exists 'apple': " << filter.contains("apple") << std::endl;
            std::cout << "Exists 'orange': " << filter.contains("orange") << std::endl;
            std::cout << "Exists 'DateOrder': " << filter.contains("DateOrder") << std::endl;
            std::cout << "Exists 'Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit': " << filter.contains("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit") << std::endl;

            std::cout << "Вес фильтра в байтах: " << sizeof(filter) << std::endl;
         }
    }
    else if (key_0 = 2)
    {
      

        std::string filePath = "data.txt"; // Укажите путь к вашему текстовому файлу
        HyperLogLog hll(12); // Инициализация HyperLogLog с параметром p=12

        std::ifstream file(filePath);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) 
            {
                std::stringstream ss(line);
                std::string word;
                while (ss >> word) 
                {
                    hll.insert(word);
                }
            }
            file.close();
        }
        else {
            std::cout << "Не удалось открыть файл." << std::endl;
            return 1;
        }

        double estimate = hll.estimate();
        std::cout << "Оценка количества уникальных элементов: " << estimate << std::endl;


        std::string csvLine;
        std::ifstream file1("100_SalesRecords.csv");

        if (file1.is_open()) {
            // Чтение первой строки из файла
            if (std::getline(file1, csvLine)) {
                size_t uniqueCount = countUniqueElementsInCSVLine(csvLine);
                std::cout << "Количество уникальных элементов: " << uniqueCount << std::endl;
            }
            else 
            {
                std::cout << "Не удалось прочитать строку из файла." << std::endl;
            }
            file.close();
        }
        else 
        {
            std::cout << "Не удалось открыть файл." << std::endl;
        }
        std::string filePath1 = "data.txt"; // Укажите путь к вашему текстовому файлу
        size_t uniqueCount = countUniqueWordsInTextFile(filePath1);
        std::cout << "Количество уникальных слов: " << uniqueCount << std::endl;

        srand(static_cast<unsigned>(time(0))); // Инициализация генератора случайных чисел

        /*
        std::ofstream file2("words.txt"); // Создание файла words.txt
        if (!file2) {
            std::cerr << "Не удалось открыть файл для записи." << std::endl;
            return 1;
        }

        std::string find2 = "";

        for (int i = 0; i < 10000; ++i) 
        {
            for (int j = 0; j < 10; ++j) 
            {
                file2 << randomLetter(); // Запись случайного символа в файл
            }
            file2 << '\n'; // Переход на новую строку после каждого слова
        }

        file2.close(); // Закрытие файла
        std::cout << "Файл words.txt успешно создан." << std::endl;
        */

        std::string filePath3 = "words.txt"; // Укажите путь к вашему текстовому файлу
        HyperLogLog hll2(13); // Инициализация HyperLogLog с параметром p=12

        std::ifstream file3(filePath3);
        if (file3.is_open()) 
        {
            std::string line;
            while (std::getline(file3, line)) 
            {
                std::stringstream ss(line);
                std::string word;
                while (ss >> word) {
                    hll2.insert(word);
                }
            }
            file3.close();
        }
        else 
        {
            std::cout << "Не удалось открыть файл." << std::endl;
            return 1;
        }

        estimate = hll2.estimate();
        std::cout << "Оценка количества уникальных элементов(HLL): " << estimate << std::endl;

        std::cout << "Оценка количество элементов O(n): " << std::endl;
        std::string line;
        int wordCount = 0;
        file.open("words.txt"); // Замените "words.txt" на имя вашего файла

        if (file.is_open()) {
            while (std::getline(file, line)) {
                // Проверяем, что строка не пустая
                if (!line.empty()) {
                    wordCount++;
                }
            }
            file.close();
        }
        else 
        {
            std::cout << "Не удалось открыть файл" << std::endl;
            return 1;
        }

        std::cout << "Количество слов в файле: " << wordCount << std::endl;


       
        int number = estimate; //проверим число из HLL
        int roundedNumber;

        // Определение ближайшего высшего разряда
        int order = pow(10.0, floor(log10(number)));

        // Округление до ближайшего высшего разряда
        if (number - order >= order / 2) {
            // Если число ближе к верхней границе, округляем вверх
            roundedNumber = (number / order + 1) * order;
        }
        else {
            // Если число ближе к нижней границе, округляем вниз
            roundedNumber = number / order * order;
        }

        std::cout << "Округленное число: " << roundedNumber << std::endl;


        //Создание фильтра блума на основе HLL-подсчета
        std::cout << "Создание динамического фильтра Блума на основе HLL" << std::endl;

        DynBloomFilter<std::string> filter(-roundedNumber, 0.1);

        file.open("words.txt");
        if (!file.is_open()) {
            std::cerr << "Не удалось открыть файл" << std::endl;
            return 1;
        }
        std::string word;
        while (std::getline(file, word)) {
            //Добавляем слово в Bloom Filter
            filter.insert(word);
            //std::cout << word << std::endl;
        }

        file.close();


        std::cout << "Есть ли в фильтре несуществующее слово '11111111'? :"  << filter.exists("11111111") << std::endl;
        std::cout << "Хранится ли в фильтре существующее слово 'hpuelitwch'? :" << filter.exists("hpuelitwch") << std::endl;
        std::cout << "Абсолютное количество коллизий:" << filter.retFullCollisions() << std::endl;
        std::cout << "Относительное количество коллизий:" << filter.countCollisions() << std::endl;
        std::cout << "Количество хеш-функций:" << filter.getHashFunctionsCount() << std::endl;
        std::cout << "Размер ДФБ в байтах: " << sizeof(filter) << std::endl;

    }
    }
    while (key_0 != 0);
    return 0;
}

// функция настройки текста
void InitText(Text& mtext, float windowWidth, float windowHeight, String str, int size_font,
    Color menu_text_color, int bord, Color border_color)
{
    mtext.setString(str);
    mtext.setCharacterSize(size_font);
    mtext.setFillColor(menu_text_color);
    mtext.setOutlineThickness(bord);
    mtext.setOutlineColor(border_color);

    // Вычисляем размеры текста
    float textWidth = mtext.getLocalBounds().width;
    float textHeight = mtext.getLocalBounds().height;

    // Вычисляем позицию текста для центрирования
    float posX = (windowWidth - textWidth) / 2.0f;
    float posY = (windowHeight - textHeight) / 7.0f;

    // Устанавливаем позицию текста
    mtext.setPosition(posX, posY);
}

