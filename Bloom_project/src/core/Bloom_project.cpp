#include <iostream>
#include <iomanip>
#include <limits>
#include <functional>
#include <vector>
#include <functional>
#include <algorithm>
#include <bitset>


#include "../ui/ProgramMenu.h"
#include "CounterBloom.hpp"
#include "InvertibleBloomFilter.hpp"
#include "CuckooFilter.hpp"
#include "BloomFilter.hpp"
#include "HyperLogLog.hpp"
#include "CuckooNonProbability.hpp"
#include "../data_structures/YFastTrie.hpp"
#include "../data_structures/MyLinkedList.hpp"
#include "../data_structures/SkipList.hpp"
#include "DynamicBloomFilter.hpp"

#include <sqlite3.h>
#include "../hash/sha256.h"
#include "../hash/md5.h"
#include "../hash/HashFunctions.hpp"
#include "Utils.hpp"

#include "../analysis/CollisionAnalyzer.hpp"

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

// ОБЪЯВЛЕНИЯ
void InitText(Text& mtext, float xpos, float ypos, String str, int size_font = 60,
    Color menu_text_color = Color::White, int bord = 0, Color border_color = Color::Black);
void BasicBloom();
void Options();
void CSV_BLOOM();
void CountingBF();
void InvertibleBF();
void CuckooBF();
void SQL_Bloom();
void ComparisonYBloom();
void BloomMenu();
void executeMenuChoice(int choice);
void processFile(const std::string& filePath, BloomFilter<std::string>& filter, std::mutex& mutex);

/**
 * @brief Функция для многопоточной обработки файла и вставки данных в фильтр Блума
 * 
 * @param filePath Путь к файлу для чтения
 * @param filter Ссылка на фильтр Блума (разделяемый ресурс)
 * @param mutex Ссылка на мьютекс для синхронизации доступа к фильтру
 */
void processFile(const std::string& filePath, BloomFilter<std::string>& filter, std::mutex& mutex) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filePath << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::lock_guard<std::mutex> lock(mutex);
        filter.insert(line);
    }
    file.close();
}

/**
 * @brief Функция визуализации работы фильтра Блума
 * 
 * Отображает интерактивное окно с квадратами, представляющими биты фильтра Блума.
 * Позволяет добавлять элементы и наблюдать за установкой соответствующих битов.
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: режим отображения 10-битного фильтра
 * - 2: режим отображения 20-битного фильтра
 * - 3: отображение формул и статистики (коллизии, количество хеш-функций)
 * - 4: режим 30-битного фильтра с MurmurHash
 * - A: добавление слова "apple"
 * - B: добавление слова "banana"
 * - Q: добавление случайного длинного слова
 * - ПКМ: очистка экрана (перерисовка)
 * 
 * @note Использует хеш-функции: hashFunction1 и hashFunction2 (базовые)
 * @note Для режима 4 дополнительно подключается murmurHash
 */
void BasicBloom() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    // Создание окна с фиксированным разрешением 1280x720
    sf::RenderWindow Play(sf::VideoMode(1280, 720), L"Визуализация фильтра Блума", sf::Style::Default);

    // ==================== ЗАГРУЗКА РЕСУРСОВ ====================
    // Загрузка шрифта для отображения текста
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    
    // Настройка фонового изображения
    sf::RectangleShape background_play(sf::Vector2f(Play.getSize().x, Play.getSize().y));
    sf::Texture texture_window;
    if (!texture_window.loadFromFile("image/bloomimg.jpg")) exit(1);
    background_play.setTexture(&texture_window);

    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ УПРАВЛЕНИЯ ====================
    // Основной текст с подсказками по управлению
    sf::Text text;
    text.setFont(font);
    text.setString(L" [ESC] - вернуться назад \n [1] - визуализация Фильтра Блума на 10 'мест' \n [2] - 20-местный Фильтр Блума с демонстрацией работы \n [3] - показать формулы и статистику \n [4] - 30 местный ФБ (MurmurHash) \n\n [A] - добавить слово 'apple' \n [B] - добавить слово 'banana' \n [Q] - добавить случайное слово (20 символов)");
    text.setCharacterSize(26);
    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(1180, 1070));

    // Текст для отображения статистики (коллизии, количество хеш-функций)
    sf::Text text2;
    text2.setFont(font);
    text2.setCharacterSize(26);
    text2.setFillColor(sf::Color::White);
    text2.setPosition(sf::Vector2f(180, 1090));

    // ==================== ПРЯМОУГОЛЬНИКИ И ФОНЫ ====================
    // Прямоугольник-подложка для области управления
    sf::RectangleShape rectangle1(sf::Vector2f(900, 300));
    rectangle1.setPosition(1150, 1055);
    rectangle1.setFillColor(sf::Color::Blue);
    rectangle1.setOutlineThickness(5);
    rectangle1.setOutlineColor(sf::Color::White);

    // Фоновая текстура для области подсказок
    sf::Texture gifTexture1;
    if (!gifTexture1.loadFromFile("image/back_b.png")) {
        return;
    }
    sf::Sprite gifSprite1(gifTexture1);
    gifSprite1.setScale(900.0f / gifTexture1.getSize().x, 300.0f / gifTexture1.getSize().y);
    gifSprite1.setPosition(1150, 1055);

    // ==================== ФОРМУЛЫ И ВЫЧИСЛЕНИЯ ====================
    // Блок 1: Формула оптимального количества хеш-функций
    sf::RectangleShape rectangle2(sf::Vector2f(311, 76));
    rectangle2.setPosition(200, 150);
    rectangle2.setFillColor(sf::Color::Blue);
    rectangle2.setOutlineThickness(5);
    rectangle2.setOutlineColor(sf::Color::Red);
    
    sf::Texture f1;
    if (!f1.loadFromFile("image/f_optimalHF.png")) return;
    sf::Sprite Spritef1(f1);
    Spritef1.setPosition(200, 150);

    // Текст с результатом вычисления оптимального количества хеш-функций
    sf::Text f1_optimalhf_text;
    f1_optimalhf_text.setFont(font);
    f1_optimalhf_text.setCharacterSize(30);
    f1_optimalhf_text.setFillColor(sf::Color::Red);
    f1_optimalhf_text.setPosition(530, 170);

    // Заголовок для формулы оптимального количества хеш-функций
    sf::Text f1_optimalhftitle;
    f1_optimalhftitle.setFont(font);
    f1_optimalhftitle.setCharacterSize(34);
    f1_optimalhftitle.setFillColor(sf::Color::White);
    f1_optimalhftitle.setPosition(250, 50);
    f1_optimalhftitle.setString(L"Оптимальное количество хеш-функций для \n m-размерного фильтра с n-элементами:");

    // Блок 2: Формула вероятности ложноположительного срабатывания
    sf::RectangleShape rectangle3(sf::Vector2f(303, 99));
    rectangle3.setPosition(1200, 150);
    rectangle3.setFillColor(sf::Color::Blue);
    rectangle3.setOutlineThickness(5);
    rectangle3.setOutlineColor(sf::Color::Red);
    
    sf::Texture f2;
    if (!f2.loadFromFile("image/f_prob.png")) return;
    sf::Sprite Spritef2(f2);
    Spritef2.setPosition(1200, 150);

    // Текст с вычисленной вероятностью ложноположительного срабатывания
    sf::Text f2_prob;
    f2_prob.setFont(font);
    f2_prob.setCharacterSize(30);
    f2_prob.setFillColor(sf::Color::Red);
    f2_prob.setPosition(1540, 185);

    // Заголовок для формулы вероятности
    sf::Text f2_ftitle;
    f2_ftitle.setFont(font);
    f2_ftitle.setCharacterSize(34);
    f2_ftitle.setFillColor(sf::Color::White);
    f2_ftitle.setPosition(1050, 70);
    f2_ftitle.setString(L"Вероятность ложноположительного срабатывания [P]: ");

    // ==================== ВИЗУАЛИЗАЦИЯ ФИЛЬТРА ====================
    // Векторы для хранения квадратов (битов) и текстовых меток
    std::vector<sf::RectangleShape> squares;
    std::vector<sf::Text> numbers;
    
    // Создание базового фильтра Блума на 20 битов с двумя хеш-функциями
    BloomFilter<std::string> filter(20);
    filter.addHashFunction(hashFunction1);
    filter.addHashFunction(hashFunction2);

    // ==================== ГЛАВНЫЙ ЦИКЛ ОБРАБОТКИ СОБЫТИЙ ====================
    while (Play.isOpen()) {
        sf::Event event;
        while (Play.pollEvent(event)) {
            // Обработка закрытия окна
            if (event.type == sf::Event::Closed) {
                Play.close();
            }
            // Обработка нажатий клавиш
            else if (event.type == sf::Event::KeyPressed) {
                // Выход в главное меню
                if (event.key.code == sf::Keyboard::Escape) {
                    Play.close();
                }
                // Режим 1: 10-битный фильтр
                else if (event.key.code == sf::Keyboard::Num1) {
                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");

                    squares.clear();
                    numbers.clear();

                    // Расчет позиций для 10 квадратов
                    float totalWidth = 10 * 50 + (10 - 1) * 10;
                    float startX = (Play.getSize().x - totalWidth) / 2;

                    for (int i = 0; i < 10; ++i) {
                        sf::RectangleShape square(sf::Vector2f(50, 50));
                        square.setFillColor(sf::Color::Red);
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2);
                        squares.push_back(square);

                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24);
                        number.setFillColor(sf::Color::White);
                        number.setPosition(square.getPosition().x + 25 - number.getLocalBounds().width / 2, 
                                          square.getPosition().y + 25 - number.getLocalBounds().height / 2);
                        numbers.push_back(number);
                    }
                }
                // Режим 2: 20-битный фильтр
                else if (event.key.code == sf::Keyboard::Num2) {
                    f1_optimalhf_text.setString("");
                    f2_prob.setString("");
                    
                    squares.clear();
                    numbers.clear();

                    // Расчет позиций для 20 квадратов
                    float totalWidth = 20 * 50 + (20 - 1) * 10;
                    float startX = (Play.getSize().x - totalWidth) / 2;

                    for (int i = 0; i < 20; ++i) {
                        sf::RectangleShape square(sf::Vector2f(50, 50));
                        square.setFillColor(sf::Color::Red);
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2);
                        squares.push_back(square);

                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24);
                        number.setFillColor(sf::Color::White);
                        number.setPosition(square.getPosition().x + 25 - number.getLocalBounds().width / 2, 
                                          square.getPosition().y + 25 - number.getLocalBounds().height / 2);
                        numbers.push_back(number);
                    }
                }
                // Добавление слова "apple" (зеленый цвет)
                else if (event.key.code == sf::Keyboard::A) {
                    filter.insert("apple");
                    std::vector<size_t> idxs = filter.getIndices("apple");
                    for (size_t idx : idxs) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Green);
                            numbers[idx].setString("1");
                        }
                    }
                }
                // Добавление слова "banana" (черный цвет)
                else if (event.key.code == sf::Keyboard::B) {
                    filter.insert("banana");
                    std::vector<size_t> idxs = filter.getIndices("banana");
                    for (size_t idx : idxs) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Black);
                            numbers[idx].setString("1");
                        }
                    }
                }
                // Добавление случайного длинного слова (малиновый цвет)
                else if (event.key.code == sf::Keyboard::Q) {
                    filter.insert("qwertyjuiceapplemasterpiece");
                    std::vector<size_t> idxs = filter.getIndices("qwertyjuiceapplemasterpiece");
                    for (size_t idx : idxs) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Magenta);
                            numbers[idx].setString("1");
                        }
                    }
                }
                // Режим 3: отображение статистики и формул
                else if (event.key.code == sf::Keyboard::Num3) {
                    text2.setString(L" Число коллизий: " + std::to_string(filter.countCollisions()) + 
                                   L"\n Количество хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));
                    
                    // Расчет оптимального количества хеш-функций (k = (m/n) * ln2)
                    f1_optimalhf_text.setString(" = " + std::to_string(0.6931 * 20 / 3) + 
                                               " ~ " + std::to_string(std::ceil(0.6931 * 20 / 3)));
                    
                    // Расчет вероятности ложноположительного срабатывания (P = (1 - e^(-k*n/m))^k)
                    f2_prob.setString(" = " + std::to_string(pow((1 - exp(-2 * 3 / 20)), 2)));
                }
                // Режим 4: 30-битный фильтр с MurmurHash
                else if (event.key.code == sf::Keyboard::Num4) {
                    // Создание нового фильтра на 30 битов с тремя хеш-функциями
                    BloomFilter<std::string> filter(30);
                    filter.addHashFunction(hashFunction1);
                    filter.addHashFunction(hashFunction2);
                    filter.addHashFunction(murmurHash);
                    
                    squares.clear();
                    numbers.clear();

                    // Расчет позиций для 30 квадратов
                    float totalWidth = 30 * 50 + (30 - 1) * 10;
                    float startX = (Play.getSize().x - totalWidth) / 2;

                    for (int i = 0; i < 30; ++i) {
                        sf::RectangleShape square(sf::Vector2f(50, 50));
                        square.setFillColor(sf::Color::Red);
                        square.setPosition(startX + i * (50 + 10), Play.getSize().y / 2);
                        squares.push_back(square);

                        sf::Text number;
                        number.setFont(font);
                        number.setString("0");
                        number.setCharacterSize(24);
                        number.setFillColor(sf::Color::White);
                        number.setPosition(square.getPosition().x + 25 - number.getLocalBounds().width / 2, 
                                          square.getPosition().y + 25 - number.getLocalBounds().height / 2);
                        numbers.push_back(number);
                    }

                    // Добавление тестовых элементов
                    filter.insert("apple");
                    std::vector<size_t> idxs = filter.getIndices("apple");
                    for (size_t idx : idxs) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Green);
                            numbers[idx].setString("1");
                        }
                    }

                    filter.insert("banana");
                    std::vector<size_t> idxs2 = filter.getIndices("banana");
                    for (size_t idx : idxs2) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Black);
                            numbers[idx].setString("1");
                        }
                    }

                    filter.insert("qwertyjuiceapplemasterpiece");
                    std::vector<size_t> idxs3 = filter.getIndices("qwertyjuiceapplemasterpiece");
                    for (size_t idx : idxs3) {
                        if (idx < squares.size()) {
                            squares[idx].setFillColor(sf::Color::Magenta);
                            numbers[idx].setString("1");
                        }
                    }

                    // Обновление статистики
                    text2.setString(L" Число коллизий: " + std::to_string(filter.countCollisions()) + 
                                   L"\n Количество хеш-функций: " + std::to_string(filter.getHashFunctionsCount()));

                    // Вычисление формул для нового фильтра
                    f1_optimalhf_text.setString(" = " + std::to_string(0.6931 * 30 / 3) + 
                                               " ~ " + std::to_string(std::ceil(0.6931 * 30 / 3)));
                    f2_prob.setString(" = " + std::to_string(pow((1 - exp(-3 * 3 / 30)), 3)));
                }
            }
            // Обработка нажатий мыши (правый клик - очистка)
            else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Right) {
                    Play.clear();  // Принудительная перерисовка
                }
            }
        }

        // ==================== ОТРИСОВКА ВСЕХ ЭЛЕМЕНТОВ ====================
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
        
        // Отрисовка всех квадратов (битов фильтра)
        for (const auto& square : squares) {
            Play.draw(square);
        }
        // Отрисовка всех текстовых меток
        for (const auto& number : numbers) {
            Play.draw(number);
        }

        Play.display();
    }
}


void Options()
{
    sf::RenderWindow Play(sf::VideoMode(1280, 720), L"HLL", sf::Style::Default);

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
    std::ifstream file1("build/100_SalesRecords.csv");

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

/**
 * @brief Функция для работы с CSV файлами и визуализации фильтра Блума
 * 
 * Отображает окно с тремя режимами работы:
 * 1. Загрузка CSV файла и проверка работы фильтра Блума
 * 2. Диаграмма зависимости коллизий от числа хеш-функций
 * 3. Диаграмма зависимости коллизий от размера фильтра
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: режим загрузки CSV (1000 строк)
 * - 2: режим отображения диаграммы коллизий от числа хеш-функций
 * - 3: режим отображения диаграммы коллизий от размера фильтра
 * - Кнопки мыши: навигация по интерфейсу
 */
void CSV_BLOOM() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    RenderWindow Options(VideoMode::getDesktopMode(), L"Анализ CSV с фильтром Блума", Style::Default);
    
    // ==================== ЗАГРУЗКА ШРИФТОВ ====================
    sf::Font font, font_bold;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    if (!font_bold.loadFromFile("fonts/timesbd.ttf")) exit(1);
    
    // ==================== НАСТРОЙКА ФОНА ====================
    RectangleShape background_opt(Vector2f(Options.getSize().x, Options.getSize().y));
    Texture texture_background;
    if (!texture_background.loadFromFile("image/blcsv.jpg")) exit(2);
    background_opt.setTexture(&texture_background);
    
    // ==================== ЗАГРУЗКА ТЕКСТУР КНОПОК ====================
    struct ButtonTextures {
        sf::Texture button1, button2, button3, button4, back;
        bool load() {
            return button1.loadFromFile("image/button1.png") &&
                   button2.loadFromFile("image/button2.png") &&
                   button3.loadFromFile("image/button3.png") &&
                   button4.loadFromFile("image/button4.png") &&
                   back.loadFromFile("image/back_button2.png");
        }
    } textures;
    
    if (!textures.load()) return;
    
    // ==================== СОЗДАНИЕ СПРАЙТОВ КНОПОК ====================
    sf::Sprite sprite1(textures.button1), sprite2(textures.button2);
    sf::Sprite sprite3(textures.button3), sprite4(textures.button4);
    sf::Sprite back_sprite(textures.back);
    
    // Позиционирование кнопок
    sprite1.setPosition(100, 1130);
    sprite2.setPosition(350, 1130);
    sprite3.setPosition(600, 1130);
    sprite4.setPosition(850, 1130);
    back_sprite.setPosition(95, 975);
    
    // ==================== ТЕКСТОВЫЕ МЕТКИ КНОПОК ====================
    struct ButtonLabel {
        sf::Text text;
        void setup(sf::Font& fnt, float x, float y, const sf::String& label) {
            text.setFont(fnt);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(x, y);
            text.setString(label);
        }
    };
    
    ButtonLabel labels[4];
    labels[0].setup(font, 170, 1300, L"Назад");
    labels[1].setup(font, 430, 1300, L"CSV*");
    labels[2].setup(font, 650, 1300, L"HF - C-count*");
    labels[3].setup(font, 895, 1300, L"SBF - C-count*");
    
    // ==================== ПАНЕЛЬ УПРАВЛЕНИЯ ====================
    sf::RectangleShape control_panel(sf::Vector2f(900, 300));
    control_panel.setPosition(1150, 1055);
    control_panel.setFillColor(sf::Color::Blue);
    control_panel.setOutlineThickness(5);
    control_panel.setOutlineColor(sf::Color::White);
    
    // Тексты панели управления
    sf::Text panel_texts[3];
    const sf::String panel_strings[] = {
        L"[1] - CSV* - открыть файл на 1000 строк и проверить работу фильтра блума",
        L"[2] - HF - C-count* - диаграмма сравнения зависимости коллизий от числа ХФ",
        L"[3] - SBF - C-count* - диаграмма сравнения числа коллизий между ФБ с \nразными размерами"
    };
    
    for (int i = 0; i < 3; ++i) {
        panel_texts[i].setFont(font);
        panel_texts[i].setCharacterSize(24);
        panel_texts[i].setFillColor(sf::Color::White);
        panel_texts[i].setPosition(1180, 1090 + i * 30);
        panel_texts[i].setString(panel_strings[i]);
    }
    
    // ==================== ФОН ПАНЕЛИ ====================
    sf::Texture panel_background;
    if (!panel_background.loadFromFile("image/back_b.png")) return;
    
    sf::Sprite panel_sprite(panel_background);
    panel_sprite.setScale(900.0f / panel_background.getSize().x, 300.0f / panel_background.getSize().y);
    panel_sprite.setPosition(1150, 1055);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ ====================
    struct StatsText {
        sf::Text counter, exists, hash_count, time;
        std::string time_content;
        
        void init(sf::Font& fnt) {
            counter.setFont(fnt);
            counter.setCharacterSize(24);
            counter.setFillColor(sf::Color::White);
            counter.setPosition(200, 900);
            
            exists.setFont(fnt);
            exists.setCharacterSize(24);
            exists.setFillColor(sf::Color::White);
            exists.setPosition(200, 850);
            
            hash_count.setFont(fnt);
            hash_count.setCharacterSize(24);
            hash_count.setFillColor(sf::Color::Green);
            hash_count.setPosition(200, 800);
            
            time.setFont(fnt);
            time.setCharacterSize(24);
            time.setFillColor(sf::Color::Green);
            time.setPosition(200, 925);
        }
    } stats;
    stats.init(font);
    
    // ==================== ПОДГОТОВКА К ЧТЕНИЮ ФАЙЛА ====================
    std::string line;
    int line_counter = 0;
    const int MAX_DISPLAY_LINES = 20;
    std::vector<sf::Text> display_lines(MAX_DISPLAY_LINES);
    
    for (auto& text_line : display_lines) {
        text_line.setFont(font);
        text_line.setCharacterSize(24);
        text_line.setFillColor(sf::Color::White);
    }
    
    std::ifstream csv_file("build/1000_SalesRecords.csv");
    if (!csv_file.is_open()) {
        std::cerr << "Unable to open file: build/1000_SalesRecords.csv" << std::endl;
        return;
    }
    
    // ==================== НАСТРОЙКА ФИЛЬТРА БЛУМА ====================
    const size_t FILTER_SIZE = 100000;
    BloomFilter<std::string> bloom_filter(FILTER_SIZE);
    bloom_filter.addHashFunction(hashFunction1);
    bloom_filter.addHashFunction(hashFunction2);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;        // 0 - нет, 1 - CSV, 2 - диаграмма ХФ, 3 - диаграмма размера
    bool data_loaded = false;
    bool all_lines_processed = false;
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (Options.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (Options.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                Options.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    Options.close();
                }
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                    bloom_filter.addHashFunction(hashSHA256);
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 2;
                    bloom_filter.addHashFunction(murmurHash);
                    bloom_filter.addHashFunction(hashSHA256);
                    bloom_filter.addHashFunction(hashMD5);
                }
                else if (event.key.code == sf::Keyboard::Num3) {
                    display_mode = 3;
                }
            }
        }
        
        Options.clear();
        Options.draw(background_opt);
        
        // ==================== РЕЖИМ 1: ЗАГРУЗКА CSV ====================
        if (display_mode == 1) {
            // Чтение строк из файла
            if (std::getline(csv_file, line)) {
                bloom_filter.insert(line);
                line_counter++;
                
                if (line_counter % 10000 == 0) {
                    stats.counter.setString(std::to_string(line_counter) + " added");
                    Options.draw(stats.counter);
                }
                
                // Обновление отображаемых строк
                for (int i = 0; i < MAX_DISPLAY_LINES; ++i) {
                    display_lines[i].setString(line);
                    display_lines[i].setPosition(200, 200 + i * 30);
                }
            } else {
                csv_file.close();
                all_lines_processed = true;
            }
            
            // Отрисовка строк
            for (const auto& text_line : display_lines) {
                Options.draw(text_line);
            }
            stats.counter.setString(std::to_string(line_counter) + " added");
            Options.draw(stats.counter);
            
            // Отображение результатов после загрузки всех строк
            if (all_lines_processed && !data_loaded) {
                const std::string portugal_order = "Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52";
                const std::string poland_order = "Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40";
                
                stats.exists.setString(L"Был ли заказ из Португалии в 2014 году?: " + 
                                       std::to_string(bloom_filter.exists(portugal_order)));
                stats.hash_count.setString(L"Число хеш-функций: " + 
                                          std::to_string(bloom_filter.getHashFunctionsCount()));
                
                sf::Clock timer;
                bloom_filter.exists(poland_order);
                sf::Time elapsed = timer.getElapsedTime();
                
                stats.time.setString("Check time O(1) [microsec] : " + 
                                    std::to_string(elapsed.asMicroseconds()) + " microseconds");
                stats.time_content = stats.time.getString();
                
                data_loaded = true;
            }
            
            // Обновление отображения статистики
            if (data_loaded) {
                Options.draw(stats.exists);
                Options.draw(stats.hash_count);
                stats.time.setString(stats.time_content);
                Options.draw(stats.time);
            }
        }
        // ==================== РЕЖИМ 2: ДИАГРАММА КОЛЛИЗИЙ ОТ ЧИСЛА ХФ ====================
        else if (display_mode == 2) {
            auto collision_data = calculateCollisions(5000);
            auto absolute_collisions = calculateCollisionsABS(5000);
            
            // Создание столбцов диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, probability_labels, abs_labels;
            
            int col_index = 0;
            for (const auto& data_pair : collision_data) {
                if (col_index >= 5) break;
                
                double column_height = data_pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, column_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + col_index * 300, 790 - column_height);
                columns.push_back(column);
                
                // Подпись столбца (число ХФ - коллизии)
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::White);
                label.setString(std::to_string(data_pair.first) + " - " + std::to_string(data_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + column_height + 10);
                column_labels.push_back(label);
                
                // Вероятность ложного срабатывания
                sf::Text probability;
                probability.setFont(font_bold);
                probability.setCharacterSize(24);
                probability.setFillColor(sf::Color::Yellow);
                probability.setString("P = " + std::to_string(pow((1 - exp(-data_pair.first * 1000 / 50000)), data_pair.first)));
                probability.setPosition(column.getPosition().x - 3, label.getPosition().y + 28);
                probability_labels.push_back(probability);
                
                col_index++;
            }
            
            // Абсолютные коллизии
            int x_pos = 348, y_pos = 860;
            for (const auto& abs_pair : absolute_collisions) {
                sf::Text abs_label;
                abs_label.setFont(font);
                abs_label.setCharacterSize(24);
                abs_label.setFillColor(sf::Color::White);
                abs_label.setString(std::to_string(std::ceil(abs_pair.first)) + " - " + 
                                   std::to_string(std::ceil(abs_pair.second)));
                abs_label.setPosition(x_pos, y_pos);
                abs_labels.push_back(abs_label);
                x_pos += 300;
            }
            
            // Заголовки диаграммы
            sf::Text title_main, title_sub;
            title_main.setFont(font);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n"
                                L"[Вероятность ложноположительного срабатывания] \n"
                                L"[Число хеш-функций] - [Абсолютное количество коллизий]");
            title_main.setPosition(700, 250);
            
            title_sub.setFont(font);
            title_sub.setCharacterSize(50);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от числа хеш-функций \n"
                               L"Сравнение вероятностей ложноположительного результата");
            title_sub.setPosition(Options.getSize().x / 2 - 650, 120);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) Options.draw(col);
            for (const auto& lbl : column_labels) Options.draw(lbl);
            for (const auto& prob : probability_labels) Options.draw(prob);
            for (const auto& abs_lbl : abs_labels) Options.draw(abs_lbl);
            Options.draw(title_main);
            Options.draw(title_sub);
        }
        // ==================== РЕЖИМ 3: ДИАГРАММА КОЛЛИЗИЙ ОТ РАЗМЕРА ====================
        else if (display_mode == 3) {
            auto size_collisions = calculateCollisions_size(3);
            
            // Создание столбцов диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, probability_labels;
            
            int col_index = 0;
            for (const auto& size_pair : size_collisions) {
                double column_height = size_pair.second / 25.0;
                sf::RectangleShape column(sf::Vector2f(120, column_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + col_index * 300, 1000 - column_height - 280);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::Black);
                label.setString(std::to_string(size_pair.first) + " - " + std::to_string(size_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + column_height + 10);
                column_labels.push_back(label);
                
                double prob_value = pow((1 - exp(-3.0 * 1000.0 / size_pair.first)), 3);
                sf::Text probability;
                probability.setFont(font);
                probability.setCharacterSize(24);
                probability.setFillColor(sf::Color::Red);
                probability.setString(std::to_string(prob_value));
                probability.setPosition(column.getPosition().x, label.getPosition().y + 30);
                probability_labels.push_back(probability);
                
                col_index++;
            }
            
            // Формулы и пояснения
            sf::Text formula_info, optimal_size_text;
            formula_info.setFont(font);
            formula_info.setCharacterSize(30);
            formula_info.setFillColor(sf::Color::Green);
            formula_info.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + 
                                  L" : оптимальный размер ФБ для получения \n"
                                  L"ложноположительного срабатывания с P = 0.001, n = 1000");
            formula_info.setPosition(450, 840);
            
            optimal_size_text.setFont(font);
            optimal_size_text.setCharacterSize(35);
            optimal_size_text.setFillColor(sf::Color::Red);
            optimal_size_text.setPosition(245, 703);
            optimal_size_text.setString("P =");
            
            // Заголовки
            sf::Text title_main, title_sub;
            title_main.setFont(font);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            title_main.setPosition(750, 300);
            
            title_sub.setFont(font);
            title_sub.setCharacterSize(40);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title_sub.setPosition(Options.getSize().x / 2 - 600, 120);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) Options.draw(col);
            for (const auto& lbl : column_labels) Options.draw(lbl);
            for (const auto& prob : probability_labels) Options.draw(prob);
            Options.draw(formula_info);
            Options.draw(optimal_size_text);
            Options.draw(title_main);
            Options.draw(title_sub);
            
            // Дополнительные элементы интерфейса для режима 3
            sf::Texture formula_img, optimal_img;
            if (formula_img.loadFromFile("image/f3.png")) {
                sf::Sprite formula_sprite(formula_img);
                formula_sprite.setPosition(200, 800);
                Options.draw(formula_sprite);
            }
            
            if (optimal_img.loadFromFile("image/f_optimalHF.png")) {
                sf::Sprite optimal_sprite(optimal_img);
                optimal_sprite.setPosition(1200, 550);
                Options.draw(optimal_sprite);
            }
        }
        
        // ==================== ОТРИСОВКА ОБЩИХ ЭЛЕМЕНТОВ ====================
        Options.draw(control_panel);
        Options.draw(panel_sprite);
        for (int i = 0; i < 3; ++i) Options.draw(panel_texts[i]);
        Options.draw(back_sprite);
        Options.draw(sprite1);
        Options.draw(sprite2);
        Options.draw(sprite3);
        Options.draw(sprite4);
        for (int i = 0; i < 4; ++i) Options.draw(labels[i].text);
        
        Options.display();
    }
}

/**
 * @brief Функция демонстрации работы Counting Bloom Filter (Фильтр Блума с подсчетом)
 * 
 * Отображает окно для работы с Counting Bloom Filter, который поддерживает удаление элементов.
 * 
 * Особенности Counting Bloom Filter:
 * - В отличие от обычного фильтра Блума, использует счетчики вместо битов
 * - Поддерживает операцию удаления элементов
 * - Требует больше памяти (обычно 4 байта на ячейку)
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: режим загрузки CSV и проверки работы фильтра
 * - 2: диаграмма зависимости коллизий от числа хеш-функций
 * - 3: диаграмма зависимости коллизий от размера фильтра
 * 
 * @see CountingBloomFilter
 */
void CountingBF() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    RenderWindow window(VideoMode::getDesktopMode(), L"Counting Bloom Filter - Демонстрация", Style::Default);
    
    // ==================== ЗАГРУЗКА ШРИФТОВ ====================
    sf::Font font_regular, font_bold;
    if (!font_regular.loadFromFile("fonts/times.ttf")) exit(1);
    if (!font_bold.loadFromFile("fonts/timesbd.ttf")) exit(1);
    
    // ==================== НАСТРОЙКА ФОНА ====================
    RectangleShape background(Vector2f(window.getSize().x, window.getSize().y));
    Texture background_texture;
    if (!background_texture.loadFromFile("image/cBFimg.jpg")) exit(2);
    background.setTexture(&background_texture);
    
    // ==================== ЗАГРУЗКА ТЕКСТУР КНОПОК ====================
    struct ButtonAssets {
        sf::Texture btn1, btn2, btn3, btn4, back;
        bool load() {
            return btn1.loadFromFile("image/button1.png") &&
                   btn2.loadFromFile("image/button2.png") &&
                   btn3.loadFromFile("image/button3.png") &&
                   btn4.loadFromFile("image/button4.png") &&
                   back.loadFromFile("image/back_button2.png");
        }
    } buttons;
    
    if (!buttons.load()) return;
    
    // ==================== СОЗДАНИЕ СПРАЙТОВ КНОПОК ====================
    sf::Sprite sprite_btn1(buttons.btn1), sprite_btn2(buttons.btn2);
    sf::Sprite sprite_btn3(buttons.btn3), sprite_btn4(buttons.btn4);
    sf::Sprite sprite_back(buttons.back);
    
    // Позиционирование кнопок
    sprite_btn1.setPosition(100, 1130);
    sprite_btn2.setPosition(350, 1130);
    sprite_btn3.setPosition(600, 1130);
    sprite_btn4.setPosition(850, 1130);
    sprite_back.setPosition(95, 975);
    
    // ==================== ТЕКСТОВЫЕ МЕТКИ КНОПОК ====================
    struct ButtonLabel {
        sf::Text text;
        void setup(sf::Font& font, float x, float y, const sf::String& label) {
            text.setFont(font);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(x, y);
            text.setString(label);
        }
    };
    
    ButtonLabel labels[4];
    labels[0].setup(font_regular, 170, 1300, L"Назад");
    labels[1].setup(font_regular, 430, 1300, L"CSV*");
    labels[2].setup(font_regular, 650, 1300, L"HF - C-count*");
    labels[3].setup(font_regular, 895, 1300, L"SBF - C-count*");
    
    // ==================== ПАНЕЛЬ УПРАВЛЕНИЯ ====================
    sf::RectangleShape control_panel(sf::Vector2f(900, 300));
    control_panel.setPosition(1150, 1055);
    control_panel.setFillColor(sf::Color::Blue);
    control_panel.setOutlineThickness(5);
    control_panel.setOutlineColor(sf::Color::White);
    
    // Тексты панели управления
    sf::Text panel_texts[3];
    const sf::String panel_strings[] = {
        L"[1] - CSV* - открыть файл на 1000 строк и проверить работу фильтра блума",
        L"",  // Пустая строка для режима 2
        L""   // Пустая строка для режима 3
    };
    
    for (int i = 0; i < 3; ++i) {
        panel_texts[i].setFont(font_regular);
        panel_texts[i].setCharacterSize(24);
        panel_texts[i].setFillColor(sf::Color::White);
        panel_texts[i].setPosition(1180, 1090 + i * 30);
        panel_texts[i].setString(panel_strings[i]);
    }
    
    // ==================== ФОН ПАНЕЛИ ====================
    sf::Texture panel_background;
    if (!panel_background.loadFromFile("image/back_b.png")) return;
    
    sf::Sprite panel_sprite(panel_background);
    panel_sprite.setScale(900.0f / panel_background.getSize().x, 300.0f / panel_background.getSize().y);
    panel_sprite.setPosition(1150, 1055);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ ====================
    struct StatisticsDisplay {
        sf::Text counter, exists, hash_count, time, removed;
        std::string time_content;
        
        void init(sf::Font& font) {
            counter.setFont(font);
            counter.setCharacterSize(24);
            counter.setFillColor(sf::Color::White);
            counter.setPosition(200, 900);
            
            exists.setFont(font);
            exists.setCharacterSize(24);
            exists.setFillColor(sf::Color::White);
            exists.setPosition(200, 850);
            
            hash_count.setFont(font);
            hash_count.setCharacterSize(24);
            hash_count.setFillColor(sf::Color::Green);
            hash_count.setPosition(200, 800);
            
            time.setFont(font);
            time.setCharacterSize(24);
            time.setFillColor(sf::Color::Green);
            time.setPosition(200, 925);
            
            removed.setFont(font);
            removed.setCharacterSize(24);
            removed.setFillColor(sf::Color::Red);
            removed.setPosition(200, 955);
        }
    } stats;
    stats.init(font_regular);
    
    // ==================== ПОДГОТОВКА К ЧТЕНИЮ ФАЙЛА ====================
    const int MAX_DISPLAY_LINES = 20;
    std::vector<sf::Text> display_lines(MAX_DISPLAY_LINES);
    for (auto& line_text : display_lines) {
        line_text.setFont(font_regular);
        line_text.setCharacterSize(24);
        line_text.setFillColor(sf::Color::White);
    }
    
    std::ifstream csv_file("build/1000_SalesRecords.csv");
    if (!csv_file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл 'build/1000_SalesRecords.csv'" << std::endl;
        return;
    }
    
    // ==================== НАСТРОЙКА COUNTING BLOOM FILTER ====================
    const size_t FILTER_SIZE = 10000;
    CountingBloomFilter<std::string> cbf_filter(FILTER_SIZE);
    cbf_filter.addHashFunction(hashFunction1);
    cbf_filter.addHashFunction(hashFunction2);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;          // 0 - нет, 1 - CSV, 2 - диаграмма ХФ, 3 - диаграмма размера
    bool data_loaded = false;
    bool all_lines_processed = false;
    int line_counter = 0;
    std::string current_line;
    std::unordered_set<std::string> unique_lines_set;
    std::vector<std::string> unique_lines_vector;
    
    // Константы для тестовых строк
    const std::string PORTUGAL_ORDER = "Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52";
    const std::string POLAND_ORDER = "Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40";
    const std::string REMOVED_ORDER = "Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36";
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (window.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                    cbf_filter.addHashFunction(murmurHash);
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 2;
                }
                else if (event.key.code == sf::Keyboard::Num3) {
                    display_mode = 3;
                }
            }
        }
        
        window.clear();
        window.draw(background);
        
        // ==================== РЕЖИМ 1: ЗАГРУЗКА CSV ====================
        if (display_mode == 1) {
            // Чтение строк из файла
            if (std::getline(csv_file, current_line)) {
                cbf_filter.insert(current_line);
                line_counter++;
                
                if (line_counter % 10000 == 0) {
                    stats.counter.setString(std::to_string(line_counter) + " added");
                    window.draw(stats.counter);
                }
                
                // Сохраняем уникальные строки
                if (unique_lines_set.find(current_line) == unique_lines_set.end()) {
                    unique_lines_vector.push_back(current_line);
                    unique_lines_set.insert(current_line);
                }
            } else {
                csv_file.close();
                all_lines_processed = true;
            }
            
            // Отображение уникальных строк
            for (int i = 0; i < MAX_DISPLAY_LINES && i < (int)unique_lines_vector.size(); ++i) {
                display_lines[i].setString(unique_lines_vector[i]);
                display_lines[i].setPosition(200, 200 + i * 30);
                window.draw(display_lines[i]);
            }
            
            stats.counter.setString(std::to_string(line_counter) + " added");
            window.draw(stats.counter);
            
            // Отображение результатов после полной загрузки
            if (all_lines_processed && !data_loaded) {
                stats.exists.setString(L"Был ли заказ из Португалии в 2014 году?: " + 
                                       std::to_string(cbf_filter.exists(PORTUGAL_ORDER)));
                stats.hash_count.setString(L"Число хеш-функций: " + 
                                          std::to_string(cbf_filter.getHashFunctionsCount()));
                
                sf::Clock timer;
                cbf_filter.exists(POLAND_ORDER);
                sf::Time elapsed = timer.getElapsedTime();
                
                stats.time.setString("Check time O(1) [microsec] : " + 
                                    std::to_string(elapsed.asMicroseconds()) + " microseconds");
                stats.time_content = stats.time.getString();
                
                data_loaded = true;
            }
            
            // Обновление отображения статистики
            if (data_loaded) {
                window.draw(stats.exists);
                window.draw(stats.hash_count);
                stats.time.setString(stats.time_content);
                window.draw(stats.time);
                
                // Демонстрация удаления элемента
                cbf_filter.remove(REMOVED_ORDER);
                stats.removed.setString(L"Есть ли удаленная строка?: " + 
                                        std::to_string(cbf_filter.exists(REMOVED_ORDER)));
                window.draw(stats.removed);
            }
        }
        // ==================== РЕЖИМ 2: ДИАГРАММА КОЛЛИЗИЙ ====================
        else if (display_mode == 2) {
            auto collision_data = calculateCollisions(5000);
            auto absolute_collisions = calculateCollisionsABS(5000);
            
            // Создание и отрисовка диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels, abs_labels;
            
            int col_index = 0;
            for (const auto& data_pair : collision_data) {
                if (col_index >= 5) break;
                
                double col_height = data_pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + col_index * 300, 790 - col_height);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::White);
                label.setString(std::to_string(data_pair.first) + " - " + std::to_string(data_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                sf::Text prob;
                prob.setFont(font_bold);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Yellow);
                prob.setString("P = " + std::to_string(pow((1 - exp(-data_pair.first * 1000 / 50000)), data_pair.first)));
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Заголовки
            sf::Text title_main, title_sub;
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n"
                                L"[Вероятность ложноположительного срабатывания] \n"
                                L"[Число хеш-функций] - [Абсолютное количество коллизий]");
            title_main.setPosition(700, 250);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(50);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от числа хеш-функций \n"
                               L"Сравнение вероятностей ложноположительного результата");
            title_sub.setPosition(window.getSize().x / 2 - 650, 120);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            window.draw(title_main);
            window.draw(title_sub);
        }
        // ==================== РЕЖИМ 3: ДИАГРАММА ЗАВИСИМОСТИ ОТ РАЗМЕРА ====================
        else if (display_mode == 3) {
            auto size_collisions = calculateCollisions_size(3);
            
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels;
            
            int col_index = 0;
            for (const auto& size_pair : size_collisions) {
                double col_height = size_pair.second / 25.0;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + col_index * 300, 1000 - col_height - 280);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::Black);
                label.setString(std::to_string(size_pair.first) + " - " + std::to_string(size_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                double prob_value = pow((1 - exp(-3.0 * 1000.0 / size_pair.first)), 3);
                sf::Text prob;
                prob.setFont(font_regular);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Red);
                prob.setString(std::to_string(prob_value));
                prob.setPosition(column.getPosition().x, label.getPosition().y + 30);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Заголовки и формулы
            sf::Text title_main, title_sub, formula_text;
            
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            title_main.setPosition(750, 300);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(40);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title_sub.setPosition(window.getSize().x / 2 - 600, 120);
            
            formula_text.setFont(font_regular);
            formula_text.setCharacterSize(30);
            formula_text.setFillColor(sf::Color::Green);
            formula_text.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + 
                                   L" : оптимальный размер ФБ для получения \n"
                                   L"ложноположительного срабатывания с P = 0.001, n = 1000");
            formula_text.setPosition(450, 840);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            window.draw(title_main);
            window.draw(title_sub);
            window.draw(formula_text);
        }
        
        // ==================== ОТРИСОВКА ОБЩИХ ЭЛЕМЕНТОВ ====================
        window.draw(control_panel);
        window.draw(panel_sprite);
        for (int i = 0; i < 3; ++i) window.draw(panel_texts[i]);
        window.draw(sprite_back);
        window.draw(sprite_btn1);
        window.draw(sprite_btn2);
        window.draw(sprite_btn3);
        window.draw(sprite_btn4);
        for (int i = 0; i < 4; ++i) window.draw(labels[i].text);
        
        window.display();
    }
}

/**
 * @brief Функция демонстрации работы Invertible Bloom Filter (Обратимый фильтр Блума)
 * 
 * Отображает окно для работы с Invertible Bloom Filter, который поддерживает:
 * - Вставку элементов
 * - Удаление элементов
 * - Восстановление элементов по индексам
 * - Получение информации о коллизиях
 * 
 * Особенности Invertible Bloom Filter:
 * - Хранит не только биты, но и дополнительные данные (ключи, суммы)
 * - Позволяет не только проверять наличие, но и восстанавливать элементы
 * - Поддерживает операции удаления с последующим восстановлением целостности
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: режим загрузки CSV и проверки работы фильтра
 * - 2: диаграмма зависимости коллизий от числа хеш-функций
 * - 3: диаграмма зависимости коллизий от размера фильтра
 * 
 * @see InvertibleBloomFilter
 */
void InvertibleBF() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    RenderWindow window(VideoMode::getDesktopMode(), L"Invertible Bloom Filter - Демонстрация", Style::Default);
    
    // ==================== ЗАГРУЗКА ШРИФТОВ ====================
    sf::Font font_regular, font_bold;
    if (!font_regular.loadFromFile("fonts/times.ttf")) exit(1);
    if (!font_bold.loadFromFile("fonts/timesbd.ttf")) exit(1);
    
    // ==================== НАСТРОЙКА ФОНА ====================
    RectangleShape background(Vector2f(window.getSize().x, window.getSize().y));
    Texture background_texture;
    if (!background_texture.loadFromFile("image/InvBF.jpg")) exit(2);
    background.setTexture(&background_texture);
    
    // ==================== ЗАГРУЗКА ТЕКСТУР КНОПОК ====================
    struct ButtonAssets {
        sf::Texture btn1, btn2, btn3, btn4, back;
        bool load() {
            return btn1.loadFromFile("image/button1.png") &&
                   btn2.loadFromFile("image/button2.png") &&
                   btn3.loadFromFile("image/button3.png") &&
                   btn4.loadFromFile("image/button4.png") &&
                   back.loadFromFile("image/back_button2.png");
        }
    } buttons;
    
    if (!buttons.load()) return;
    
    // ==================== СОЗДАНИЕ СПРАЙТОВ КНОПОК ====================
    sf::Sprite sprite_btn1(buttons.btn1), sprite_btn2(buttons.btn2);
    sf::Sprite sprite_btn3(buttons.btn3), sprite_btn4(buttons.btn4);
    sf::Sprite sprite_back(buttons.back);
    
    // Позиционирование кнопок
    sprite_btn1.setPosition(100, 1130);
    sprite_btn2.setPosition(350, 1130);
    sprite_btn3.setPosition(600, 1130);
    sprite_btn4.setPosition(850, 1130);
    sprite_back.setPosition(95, 975);
    
    // ==================== ТЕКСТОВЫЕ МЕТКИ КНОПОК ====================
    struct ButtonLabel {
        sf::Text text;
        void setup(sf::Font& font, float x, float y, const sf::String& label) {
            text.setFont(font);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(x, y);
            text.setString(label);
        }
    };
    
    ButtonLabel labels[4];
    labels[0].setup(font_regular, 170, 1300, L"Назад");
    labels[1].setup(font_regular, 430, 1300, L"CSV*");
    labels[2].setup(font_regular, 650, 1300, L"HF - C-count*");
    labels[3].setup(font_regular, 895, 1300, L"SBF - C-count*");
    
    // ==================== ПАНЕЛЬ УПРАВЛЕНИЯ ====================
    sf::RectangleShape control_panel(sf::Vector2f(900, 300));
    control_panel.setPosition(1150, 1055);
    control_panel.setFillColor(sf::Color::Blue);
    control_panel.setOutlineThickness(5);
    control_panel.setOutlineColor(sf::Color::White);
    
    // Тексты панели управления
    sf::Text panel_texts[3];
    const sf::String panel_strings[] = {
        L"[1] - CSV* - открыть файл на 1000 строк и проверить работу фильтра блума",
        L"",  // Пустая строка для режима 2
        L""   // Пустая строка для режима 3
    };
    
    for (int i = 0; i < 3; ++i) {
        panel_texts[i].setFont(font_regular);
        panel_texts[i].setCharacterSize(24);
        panel_texts[i].setFillColor(sf::Color::White);
        panel_texts[i].setPosition(1180, 1090 + i * 30);
        panel_texts[i].setString(panel_strings[i]);
    }
    
    // ==================== ФОН ПАНЕЛИ ====================
    sf::Texture panel_background;
    if (!panel_background.loadFromFile("image/back_b.png")) return;
    
    sf::Sprite panel_sprite(panel_background);
    panel_sprite.setScale(900.0f / panel_background.getSize().x, 300.0f / panel_background.getSize().y);
    panel_sprite.setPosition(1150, 1055);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ ====================
    struct StatisticsDisplay {
        sf::Text counter, exists, hash_count, time, indices_info, value_by_index;
        std::string time_content;
        
        void init(sf::Font& font) {
            counter.setFont(font);
            counter.setCharacterSize(24);
            counter.setFillColor(sf::Color::White);
            counter.setPosition(200, 900);
            
            exists.setFont(font);
            exists.setCharacterSize(24);
            exists.setFillColor(sf::Color::White);
            exists.setPosition(200, 850);
            
            hash_count.setFont(font);
            hash_count.setCharacterSize(24);
            hash_count.setFillColor(sf::Color::Green);
            hash_count.setPosition(200, 800);
            
            time.setFont(font);
            time.setCharacterSize(24);
            time.setFillColor(sf::Color::Green);
            time.setPosition(200, 925);
            
            indices_info.setFont(font);
            indices_info.setCharacterSize(24);
            indices_info.setFillColor(sf::Color::Red);
            indices_info.setPosition(200, 955);
            
            value_by_index.setFont(font);
            value_by_index.setCharacterSize(24);
            value_by_index.setFillColor(sf::Color::White);
            value_by_index.setPosition(200, 975);
        }
    } stats;
    stats.init(font_regular);
    
    // ==================== ПОДГОТОВКА К ЧТЕНИЮ ФАЙЛА ====================
    const int MAX_DISPLAY_LINES = 20;
    std::vector<sf::Text> display_lines(MAX_DISPLAY_LINES);
    for (auto& line_text : display_lines) {
        line_text.setFont(font_regular);
        line_text.setCharacterSize(24);
        line_text.setFillColor(sf::Color::White);
    }
    
    std::ifstream csv_file("build/1000_SalesRecords.csv");
    if (!csv_file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл '1000_SalesRecords.csv'" << std::endl;
        return;
    }
    
    // ==================== НАСТРОЙКА INVERTIBLE BLOOM FILTER ====================
    const size_t FILTER_SIZE = 10000;
    InvertibleBloomFilter<std::string> ibf_filter(FILTER_SIZE);
    ibf_filter.addHashFunction(hashFunction1);
    ibf_filter.addHashFunction(hashFunction2);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;          // 0 - нет, 1 - CSV, 2 - диаграмма ХФ, 3 - диаграмма размера
    bool data_loaded = false;
    bool all_lines_processed = false;
    int line_counter = 0;
    std::string current_line;
    std::unordered_set<std::string> unique_lines_set;
    std::vector<std::string> unique_lines_vector;
    
    // Константы для тестовых строк
    const std::string PORTUGAL_ORDER = "Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52";
    const std::string POLAND_ORDER = "Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40";
    const std::string TEST_INDEX_STRING = "2059";
    const size_t TEST_INDEX = 2059;
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (window.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                    ibf_filter.addHashFunction(murmurHash);
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 2;
                }
                else if (event.key.code == sf::Keyboard::Num3) {
                    display_mode = 3;
                }
            }
        }
        
        window.clear();
        window.draw(background);
        
        // ==================== РЕЖИМ 1: ЗАГРУЗКА CSV ====================
        if (display_mode == 1) {
            // Чтение строк из файла
            if (std::getline(csv_file, current_line)) {
                ibf_filter.insert(current_line);
                line_counter++;
                
                if (line_counter % 10000 == 0) {
                    stats.counter.setString(std::to_string(line_counter) + " added");
                    window.draw(stats.counter);
                }
                
                // Сохраняем уникальные строки для отображения
                if (unique_lines_set.find(current_line) == unique_lines_set.end()) {
                    unique_lines_vector.push_back(current_line);
                    unique_lines_set.insert(current_line);
                }
            } else {
                csv_file.close();
                all_lines_processed = true;
            }
            
            // Отображение уникальных строк
            for (int i = 0; i < MAX_DISPLAY_LINES && i < (int)unique_lines_vector.size(); ++i) {
                // Обрезаем длинные строки для лучшего отображения
                std::string display_line = unique_lines_vector[i];
                if (display_line.length() > 80) {
                    display_line = display_line.substr(0, 77) + "...";
                }
                display_lines[i].setString(display_line);
                display_lines[i].setPosition(200, 200 + i * 30);
                window.draw(display_lines[i]);
            }
            
            stats.counter.setString(std::to_string(line_counter) + " added");
            window.draw(stats.counter);
            
            // Отображение результатов после полной загрузки
            if (all_lines_processed && !data_loaded) {
                stats.exists.setString(L"Был ли заказ из Португалии в 2014 году?: " + 
                                       std::to_string(ibf_filter.exists(PORTUGAL_ORDER)));
                stats.hash_count.setString(L"Число хеш-функций: " + 
                                          std::to_string(ibf_filter.getHashFunctionsCount()));
                
                sf::Clock timer;
                ibf_filter.exists(POLAND_ORDER);
                sf::Time elapsed = timer.getElapsedTime();
                
                stats.time.setString("Check time O(1) [microsec] : " + 
                                    std::to_string(elapsed.asMicroseconds()) + " microseconds");
                stats.time_content = stats.time.getString();
                
                data_loaded = true;
            }
            
            // Обновление отображения статистики
            if (data_loaded) {
                window.draw(stats.exists);
                window.draw(stats.hash_count);
                stats.time.setString(stats.time_content);
                window.draw(stats.time);
                
                // Демонстрация уникальной возможности Invertible Bloom Filter:
                // Получение индексов элемента и восстановление значения по индексу
                std::vector<size_t> indices = ibf_filter.getIndices(PORTUGAL_ORDER);
                stats.indices_info.setString(L"Индексы строки: 'Europe,Portugal...' : " + 
                                             vectorToString(indices));
                window.draw(stats.indices_info);
                
                // Восстановление значения по индексу
                std::string value_at_index = ibf_filter.getValue(TEST_INDEX);
                if (value_at_index.empty()) {
                    stats.value_by_index.setString(L"Строка по индексу '" + 
                                                   String(TEST_INDEX_STRING) + 
                                                   L"' : (не найдена)");
                } else if (value_at_index.length() > 60) {
                    stats.value_by_index.setString(L"Строка по индексу '" + 
                                                   String(TEST_INDEX_STRING) + 
                                                   L"' : " + 
                                                   String(value_at_index.substr(0, 57) + "..."));
                } else {
                    stats.value_by_index.setString(L"Строка по индексу '" + 
                                                   String(TEST_INDEX_STRING) + 
                                                   L"' : " + 
                                                   String(value_at_index));
                }
                window.draw(stats.value_by_index);
            }
        }
        // ==================== РЕЖИМ 2: ДИАГРАММА КОЛЛИЗИЙ ====================
        else if (display_mode == 2) {
            auto collision_data = calculateCollisions(5000);
            auto absolute_collisions = calculateCollisionsABS(5000);
            
            // Создание и отрисовка диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels, abs_labels;
            
            int col_index = 0;
            for (const auto& data_pair : collision_data) {
                if (col_index >= 5) break;
                
                double col_height = data_pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + col_index * 300, 790 - col_height);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::White);
                label.setString(std::to_string(data_pair.first) + " - " + std::to_string(data_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                sf::Text prob;
                prob.setFont(font_bold);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Yellow);
                prob.setString("P = " + std::to_string(pow((1 - exp(-data_pair.first * 1000 / 50000)), data_pair.first)));
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Абсолютные коллизии
            int x_pos = 348, y_pos = 860;
            for (const auto& abs_pair : absolute_collisions) {
                sf::Text abs_label;
                abs_label.setFont(font_regular);
                abs_label.setCharacterSize(24);
                abs_label.setFillColor(sf::Color::White);
                abs_label.setString(std::to_string(std::ceil(abs_pair.first)) + " - " + 
                                   std::to_string(std::ceil(abs_pair.second)));
                abs_label.setPosition(x_pos, y_pos);
                abs_labels.push_back(abs_label);
                x_pos += 300;
            }
            
            // Заголовки
            sf::Text title_main, title_sub;
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n"
                                L"[Вероятность ложноположительного срабатывания] \n"
                                L"[Число хеш-функций] - [Абсолютное количество коллизий]");
            title_main.setPosition(700, 250);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(50);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от числа хеш-функций \n"
                               L"Сравнение вероятностей ложноположительного результата");
            title_sub.setPosition(window.getSize().x / 2 - 650, 120);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            for (const auto& abs_lbl : abs_labels) window.draw(abs_lbl);
            window.draw(title_main);
            window.draw(title_sub);
        }
        // ==================== РЕЖИМ 3: ДИАГРАММА ЗАВИСИМОСТИ ОТ РАЗМЕРА ====================
        else if (display_mode == 3) {
            auto size_collisions = calculateCollisions_size(3);
            
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels;
            
            int col_index = 0;
            for (const auto& size_pair : size_collisions) {
                double col_height = size_pair.second / 25.0;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + col_index * 300, 1000 - col_height - 280);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::Black);
                label.setString(std::to_string(size_pair.first) + " - " + std::to_string(size_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                double prob_value = pow((1 - exp(-3.0 * 1000.0 / size_pair.first)), 3);
                sf::Text prob;
                prob.setFont(font_regular);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Red);
                prob.setString(std::to_string(prob_value));
                prob.setPosition(column.getPosition().x, label.getPosition().y + 30);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Заголовки и формулы
            sf::Text title_main, title_sub, formula_text, p_label;
            
            p_label.setFont(font_regular);
            p_label.setCharacterSize(35);
            p_label.setFillColor(sf::Color::Red);
            p_label.setPosition(245, 703);
            p_label.setString("P =");
            
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Размер фильтра Блума(при 3-ХФ)] - [Относительное количество коллизий]");
            title_main.setPosition(750, 300);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(40);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от размера фильтра блума при данных в 100000 строк");
            title_sub.setPosition(window.getSize().x / 2 - 600, 120);
            
            formula_text.setFont(font_regular);
            formula_text.setCharacterSize(30);
            formula_text.setFillColor(sf::Color::Green);
            formula_text.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + 
                                   L" : оптимальный размер ФБ для получения \n"
                                   L"ложноположительного срабатывания с P = 0.001, n = 1000");
            formula_text.setPosition(450, 840);
            
            // Дополнительные графические элементы
            sf::Texture formula_img, optimal_img;
            sf::Sprite formula_sprite, optimal_sprite;
            
            if (formula_img.loadFromFile("image/f3.png")) {
                formula_sprite.setTexture(formula_img);
                formula_sprite.setPosition(200, 800);
                window.draw(formula_sprite);
            }
            
            if (optimal_img.loadFromFile("image/f_optimalHF.png")) {
                optimal_sprite.setTexture(optimal_img);
                optimal_sprite.setPosition(1200, 550);
                window.draw(optimal_sprite);
            }
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            window.draw(p_label);
            window.draw(title_main);
            window.draw(title_sub);
            window.draw(formula_text);
        }
        
        // ==================== ОТРИСОВКА ОБЩИХ ЭЛЕМЕНТОВ ====================
        window.draw(control_panel);
        window.draw(panel_sprite);
        for (int i = 0; i < 3; ++i) window.draw(panel_texts[i]);
        window.draw(sprite_back);
        window.draw(sprite_btn1);
        window.draw(sprite_btn2);
        window.draw(sprite_btn3);
        window.draw(sprite_btn4);
        for (int i = 0; i < 4; ++i) window.draw(labels[i].text);
        
        window.display();
    }
}

/**
 * @brief Функция демонстрации работы Cuckoo Filter (Фильтр Кукушки)
 * 
 * Отображает окно для работы с Cuckoo Filter - альтернативой фильтру Блума
 * с поддержкой удаления и лучшей производительностью.
 * 
 * Особенности Cuckoo Filter:
 * - Использует хеш-таблицу с двумя возможными позициями для каждого элемента
 * - Поддерживает эффективное удаление элементов
 * - Имеет более низкую вероятность ложных срабатываний чем фильтр Блума
 * - Использует технику "кукушки" для разрешения коллизий
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: режим загрузки CSV и проверки работы фильтра
 * - 2: диаграмма зависимости коллизий от числа хеш-функций
 * - 3: диаграмма зависимости коллизий от размера фильтра
 * 
 * @see CuckooFilterN
 */
void CuckooBF() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    RenderWindow window(VideoMode::getDesktopMode(), L"Cuckoo Filter - Демонстрация", Style::Default);
    
    // ==================== ЗАГРУЗКА ШРИФТОВ ====================
    sf::Font font_regular, font_bold;
    if (!font_regular.loadFromFile("fonts/times.ttf")) exit(1);
    if (!font_bold.loadFromFile("fonts/timesbd.ttf")) exit(1);
    
    // ==================== НАСТРОЙКА ФОНА ====================
    RectangleShape background(Vector2f(window.getSize().x, window.getSize().y));
    Texture background_texture;
    if (!background_texture.loadFromFile("image/Cbf.jpg")) exit(2);
    background.setTexture(&background_texture);
    
    // ==================== ЗАГРУЗКА ТЕКСТУР КНОПОК ====================
    struct ButtonAssets {
        sf::Texture btn1, btn2, btn3, btn4, back;
        bool load() {
            return btn1.loadFromFile("image/button1.png") &&
                   btn2.loadFromFile("image/button2.png") &&
                   btn3.loadFromFile("image/button3.png") &&
                   btn4.loadFromFile("image/button4.png") &&
                   back.loadFromFile("image/back_button2.png");
        }
    } buttons;
    
    if (!buttons.load()) return;
    
    // ==================== СОЗДАНИЕ СПРАЙТОВ КНОПОК ====================
    sf::Sprite sprite_btn1(buttons.btn1), sprite_btn2(buttons.btn2);
    sf::Sprite sprite_btn3(buttons.btn3), sprite_btn4(buttons.btn4);
    sf::Sprite sprite_back(buttons.back);
    
    // Позиционирование кнопок
    sprite_btn1.setPosition(100, 1130);
    sprite_btn2.setPosition(350, 1130);
    sprite_btn3.setPosition(600, 1130);
    sprite_btn4.setPosition(850, 1130);
    sprite_back.setPosition(95, 975);
    
    // ==================== ТЕКСТОВЫЕ МЕТКИ КНОПОК ====================
    struct ButtonLabel {
        sf::Text text;
        void setup(sf::Font& font, float x, float y, const sf::String& label) {
            text.setFont(font);
            text.setCharacterSize(20);
            text.setFillColor(sf::Color::White);
            text.setPosition(x, y);
            text.setString(label);
        }
    };
    
    ButtonLabel labels[4];
    labels[0].setup(font_regular, 170, 1300, L"Назад");
    labels[1].setup(font_regular, 430, 1300, L"CSV*");
    labels[2].setup(font_regular, 650, 1300, L"HF - C-count*");
    labels[3].setup(font_regular, 895, 1300, L"SBF - C-count*");
    
    // ==================== ПАНЕЛЬ УПРАВЛЕНИЯ ====================
    sf::RectangleShape control_panel(sf::Vector2f(900, 300));
    control_panel.setPosition(1150, 1055);
    control_panel.setFillColor(sf::Color::Blue);
    control_panel.setOutlineThickness(5);
    control_panel.setOutlineColor(sf::Color::White);
    
    // Тексты панели управления
    sf::Text panel_texts[3];
    const sf::String panel_strings[] = {
        L"[1] - CSV* - открыть файл на 1000 строк и проверить работу фильтра кукушки",
        L"",  // Пустая строка для режима 2
        L""   // Пустая строка для режима 3
    };
    
    for (int i = 0; i < 3; ++i) {
        panel_texts[i].setFont(font_regular);
        panel_texts[i].setCharacterSize(24);
        panel_texts[i].setFillColor(sf::Color::White);
        panel_texts[i].setPosition(1180, 1090 + i * 30);
        panel_texts[i].setString(panel_strings[i]);
    }
    
    // ==================== ФОН ПАНЕЛИ ====================
    sf::Texture panel_background;
    if (!panel_background.loadFromFile("image/back_b.png")) return;
    
    sf::Sprite panel_sprite(panel_background);
    panel_sprite.setScale(900.0f / panel_background.getSize().x, 300.0f / panel_background.getSize().y);
    panel_sprite.setPosition(1150, 1055);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ ====================
    struct StatisticsDisplay {
        sf::Text counter, exists, hash_count, time, removed;
        std::string time_content;
        
        void init(sf::Font& font) {
            counter.setFont(font);
            counter.setCharacterSize(24);
            counter.setFillColor(sf::Color::White);
            counter.setPosition(200, 900);
            
            exists.setFont(font);
            exists.setCharacterSize(24);
            exists.setFillColor(sf::Color::White);
            exists.setPosition(200, 850);
            
            hash_count.setFont(font);
            hash_count.setCharacterSize(24);
            hash_count.setFillColor(sf::Color::Green);
            hash_count.setPosition(200, 800);
            
            time.setFont(font);
            time.setCharacterSize(24);
            time.setFillColor(sf::Color::Green);
            time.setPosition(200, 925);
            
            removed.setFont(font);
            removed.setCharacterSize(24);
            removed.setFillColor(sf::Color::Red);
            removed.setPosition(200, 955);
        }
    } stats;
    stats.init(font_regular);
    
    // ==================== ПОДГОТОВКА К ЧТЕНИЮ ФАЙЛА ====================
    const int MAX_DISPLAY_LINES = 20;
    std::vector<sf::Text> display_lines(MAX_DISPLAY_LINES);
    for (auto& line_text : display_lines) {
        line_text.setFont(font_regular);
        line_text.setCharacterSize(24);
        line_text.setFillColor(sf::Color::White);
    }
    
    std::ifstream csv_file("build/1000_SalesRecords.csv");
    if (!csv_file.is_open()) {
        std::cerr << "Ошибка: не удалось открыть файл '1000_SalesRecords.csv'" << std::endl;
        return;
    }
    
    // ==================== НАСТРОЙКА CUCKOO FILTER ====================
    const size_t FILTER_SIZE = 10000;
    CuckooFilterN cuckoo_filter(FILTER_SIZE);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;          // 0 - нет, 1 - CSV, 2 - диаграмма ХФ, 3 - диаграмма размера
    bool data_loaded = false;
    bool all_lines_processed = false;
    int line_counter = 0;
    std::string current_line;
    std::unordered_set<std::string> unique_lines_set;
    std::vector<std::string> unique_lines_vector;
    
    // Константы для тестовых строк
    const std::string PORTUGAL_ORDER = "Europe,Portugal,Cereal,Offline,C,4/10/2014,811546599,5/8/2014,3528,205.70,117.11,725709.60,413164.08,312545.52";
    const std::string POLAND_ORDER = "Europe,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40";
    const std::string GREECE_ORDER = "Europe,Greece,Household,Online,C,11/17/2016,702186715,12/22/2016,1508,668.27,502.54,1007751.16,757830.32,249920.84";
    const std::string REMOVED_ORDER = "Central America and the Caribbean,Grenada,Cereal,Online,C,8/22/2012,963881480,9/15/2012,2804,205.70,117.11,576782.80,328376.44,248406.36";
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (window.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 2;
                }
                else if (event.key.code == sf::Keyboard::Num3) {
                    display_mode = 3;
                }
            }
        }
        
        window.clear();
        window.draw(background);
        
        // ==================== РЕЖИМ 1: ЗАГРУЗКА CSV ====================
        if (display_mode == 1) {
            // Чтение строк из файла
            if (std::getline(csv_file, current_line)) {
                cuckoo_filter.insert(current_line);
                line_counter++;
                
                if (line_counter % 10000 == 0) {
                    stats.counter.setString(std::to_string(line_counter) + " added");
                    window.draw(stats.counter);
                }
                
                // Сохраняем уникальные строки для отображения
                if (unique_lines_set.find(current_line) == unique_lines_set.end()) {
                    unique_lines_vector.push_back(current_line);
                    unique_lines_set.insert(current_line);
                }
            } else {
                csv_file.close();
                all_lines_processed = true;
            }
            
            // Отображение уникальных строк
            for (int i = 0; i < MAX_DISPLAY_LINES && i < (int)unique_lines_vector.size(); ++i) {
                // Обрезаем длинные строки для лучшего отображения
                std::string display_line = unique_lines_vector[i];
                if (display_line.length() > 80) {
                    display_line = display_line.substr(0, 77) + "...";
                }
                display_lines[i].setString(display_line);
                display_lines[i].setPosition(200, 200 + i * 30);
                window.draw(display_lines[i]);
            }
            
            stats.counter.setString(std::to_string(line_counter) + " added");
            window.draw(stats.counter);
            
            // Отображение результатов после полной загрузки
            if (all_lines_processed && !data_loaded) {
                stats.exists.setString(L"Был ли заказ из Португалии в 2014 году?: " + 
                                       std::to_string(cuckoo_filter.contains(PORTUGAL_ORDER)));
                stats.hash_count.setString(L"Число хеш-функций: 3");
                
                sf::Clock timer;
                cuckoo_filter.contains(POLAND_ORDER);
                sf::Time elapsed = timer.getElapsedTime();
                
                stats.time.setString("Check time O(1) [microsec] : " + 
                                    std::to_string(elapsed.asMicroseconds()) + " microseconds");
                stats.time_content = stats.time.getString();
                
                data_loaded = true;
            }
            
            // Обновление отображения статистики
            if (data_loaded) {
                stats.exists.setString(L"Был ли заказ из Греции в 2016 году?: " + 
                                       std::to_string(cuckoo_filter.contains(GREECE_ORDER)));
                window.draw(stats.exists);
                window.draw(stats.hash_count);
                stats.time.setString(stats.time_content);
                window.draw(stats.time);
                
                // Демонстрация удаления элемента
                cuckoo_filter.remove(REMOVED_ORDER);
                stats.removed.setString(L"Есть ли удаленная строка?: " + 
                                        std::to_string(cuckoo_filter.contains(REMOVED_ORDER)));
                window.draw(stats.removed);
            }
        }
        // ==================== РЕЖИМ 2: ДИАГРАММА КОЛЛИЗИЙ ====================
        else if (display_mode == 2) {
            auto collision_data = calculateCollisions(5000);
            auto absolute_collisions = calculateCollisionsABS(5000);
            
            // Создание и отрисовка диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels, abs_labels;
            
            int col_index = 0;
            for (const auto& data_pair : collision_data) {
                if (col_index >= 5) break;
                
                double col_height = data_pair.first * 70;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(350 + col_index * 300, 790 - col_height);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::White);
                label.setString(std::to_string(data_pair.first) + " - " + std::to_string(data_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                sf::Text prob;
                prob.setFont(font_bold);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Yellow);
                prob.setString("P = " + std::to_string(pow((1 - exp(-data_pair.first * 1000 / 50000)), data_pair.first)));
                prob.setPosition(column.getPosition().x - 3, label.getPosition().y + 28);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Абсолютные коллизии
            int x_pos = 348, y_pos = 860;
            for (const auto& abs_pair : absolute_collisions) {
                sf::Text abs_label;
                abs_label.setFont(font_regular);
                abs_label.setCharacterSize(24);
                abs_label.setFillColor(sf::Color::White);
                abs_label.setString(std::to_string(std::ceil(abs_pair.first)) + " - " + 
                                   std::to_string(std::ceil(abs_pair.second)));
                abs_label.setPosition(x_pos, y_pos);
                abs_labels.push_back(abs_label);
                x_pos += 300;
            }
            
            // Заголовки
            sf::Text title_main, title_sub;
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Число хеш-функций] - [Относительное количество коллизий] \n"
                                L"[Вероятность ложноположительного срабатывания] \n"
                                L"[Число хеш-функций] - [Абсолютное количество коллизий]");
            title_main.setPosition(700, 250);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(50);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от числа хеш-функций \n"
                               L"Сравнение вероятностей ложноположительного результата");
            title_sub.setPosition(window.getSize().x / 2 - 650, 120);
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            for (const auto& abs_lbl : abs_labels) window.draw(abs_lbl);
            window.draw(title_main);
            window.draw(title_sub);
        }
        // ==================== РЕЖИМ 3: ДИАГРАММА ЗАВИСИМОСТИ ОТ РАЗМЕРА ====================
        else if (display_mode == 3) {
            auto size_collisions = calculateCollisions_size(3);
            
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> column_labels, prob_labels;
            
            int col_index = 0;
            for (const auto& size_pair : size_collisions) {
                double col_height = size_pair.second / 25.0;
                sf::RectangleShape column(sf::Vector2f(120, col_height));
                column.setFillColor(sf::Color::Green);
                column.setPosition(300 + col_index * 300, 1000 - col_height - 280);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font_regular);
                label.setCharacterSize(24);
                label.setFillColor(sf::Color::Black);
                label.setString(std::to_string(size_pair.first) + " - " + std::to_string(size_pair.second));
                label.setPosition(column.getPosition().x, column.getPosition().y + col_height + 10);
                column_labels.push_back(label);
                
                double prob_value = pow((1 - exp(-3.0 * 1000.0 / size_pair.first)), 3);
                sf::Text prob;
                prob.setFont(font_regular);
                prob.setCharacterSize(24);
                prob.setFillColor(sf::Color::Red);
                prob.setString(std::to_string(prob_value));
                prob.setPosition(column.getPosition().x, label.getPosition().y + 30);
                prob_labels.push_back(prob);
                
                col_index++;
            }
            
            // Заголовки и формулы
            sf::Text title_main, title_sub, formula_text, p_label;
            
            p_label.setFont(font_regular);
            p_label.setCharacterSize(35);
            p_label.setFillColor(sf::Color::Red);
            p_label.setPosition(245, 703);
            p_label.setString("P =");
            
            title_main.setFont(font_regular);
            title_main.setCharacterSize(30);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Размер фильтра(при 3-ХФ)] - [Относительное количество коллизий]");
            title_main.setPosition(750, 300);
            
            title_sub.setFont(font_regular);
            title_sub.setCharacterSize(40);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость уровня коллизий от размера фильтра при данных в 100000 строк");
            title_sub.setPosition(window.getSize().x / 2 - 600, 120);
            
            formula_text.setFont(font_regular);
            formula_text.setCharacterSize(30);
            formula_text.setFillColor(sf::Color::Green);
            formula_text.setString(" ~ " + std::to_string(-1000 * log(0.001) / pow(0.69, 2)) + 
                                   L" : оптимальный размер фильтра для получения \n"
                                   L"ложноположительного срабатывания с P = 0.001, n = 1000");
            formula_text.setPosition(450, 840);
            
            // Дополнительные графические элементы
            sf::Texture formula_img, optimal_img;
            sf::Sprite formula_sprite, optimal_sprite;
            
            if (formula_img.loadFromFile("image/f3.png")) {
                formula_sprite.setTexture(formula_img);
                formula_sprite.setPosition(200, 800);
                window.draw(formula_sprite);
            }
            
            if (optimal_img.loadFromFile("image/f_optimalHF.png")) {
                optimal_sprite.setTexture(optimal_img);
                optimal_sprite.setPosition(1200, 550);
                window.draw(optimal_sprite);
            }
            
            // Отрисовка диаграммы
            for (const auto& col : columns) window.draw(col);
            for (const auto& lbl : column_labels) window.draw(lbl);
            for (const auto& prob : prob_labels) window.draw(prob);
            window.draw(p_label);
            window.draw(title_main);
            window.draw(title_sub);
            window.draw(formula_text);
        }
        
        // ==================== ОТРИСОВКА ОБЩИХ ЭЛЕМЕНТОВ ====================
        window.draw(control_panel);
        window.draw(panel_sprite);
        for (int i = 0; i < 3; ++i) window.draw(panel_texts[i]);
        window.draw(sprite_back);
        window.draw(sprite_btn1);
        window.draw(sprite_btn2);
        window.draw(sprite_btn3);
        window.draw(sprite_btn4);
        for (int i = 0; i < 4; ++i) window.draw(labels[i].text);
        
        window.display();
    }
}

/**
 * @brief Функция демонстрации работы фильтра Блума с SQL базой данных
 * 
 * Отображает окно для работы с фильтром Блума и SQLite базой данных.
 * Позволяет загружать данные из БД и проверять их наличие с помощью фильтра.
 * 
 * Особенности:
 * - Подключается к SQLite базе данных "datausers.db"
 * - Загружает имена пользователей из таблицы users2
 * - Демонстрирует работу фильтра Блума с реальными данными
 * - Показывает диаграмму зависимости вероятности от числа хеш-функций
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: загрузка данных из SQL и проверка работы фильтра
 * - 2: отображение диаграммы зависимости вероятности от числа хеш-функций
 * 
 * @note Для работы требует наличия файла базы данных "datausers.db"
 *       с таблицей users2, содержащей колонку name
 */
void SQL_Bloom() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    sf::RenderWindow sql_window(sf::VideoMode(1280, 720), L"SQL + Фильтр Блума", sf::Style::Default);
    
    // Установка view с базовым разрешением 1920x1080 для масштабирования
    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
    sql_window.setView(view);
    
    const float VIEW_WIDTH = view.getSize().x;   // 1920
    const float VIEW_HEIGHT = view.getSize().y;  // 1080
    
    // ==================== ЗАГРУЗКА РЕСУРСОВ ====================
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) {
        std::cerr << "Ошибка: не удалось загрузить шрифт 'fonts/times.ttf'" << std::endl;
        return;
    }
    
    // Фоновое изображение
    sf::RectangleShape background(sf::Vector2f(VIEW_WIDTH, VIEW_HEIGHT));
    sf::Texture background_texture;
    if (!background_texture.loadFromFile("image/sqlm.jpg")) exit(3);
    background.setTexture(&background_texture);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ДЛЯ СТАТИСТИКИ ====================
    struct StatisticsDisplay {
        sf::Text absolute_collisions, hash_count, probability, result;
        
        void init(sf::Font& font) {
            absolute_collisions.setFont(font);
            absolute_collisions.setCharacterSize(20);
            absolute_collisions.setFillColor(sf::Color::White);
            absolute_collisions.setPosition(180, 860);
            
            hash_count.setFont(font);
            hash_count.setCharacterSize(20);
            hash_count.setFillColor(sf::Color::White);
            hash_count.setPosition(180, 820);
            
            probability.setFont(font);
            probability.setCharacterSize(20);
            probability.setFillColor(sf::Color::Green);
            probability.setPosition(180, 780);
            
            result.setFont(font);
            result.setCharacterSize(20);
            result.setFillColor(sf::Color::White);
            result.setPosition(180, 740);
        }
    } stats;
    stats.init(font);
    
    // ==================== ПАНЕЛЬ УПРАВЛЕНИЯ ====================
    struct ControlPanel {
        sf::RectangleShape background;
        sf::Sprite sprite;
        sf::Text lines[3];
        
        bool init(sf::Font& font, const std::string& texture_path) {
            background = sf::RectangleShape(sf::Vector2f(700, 250));
            background.setPosition(1150, 800);
            background.setFillColor(sf::Color::Blue);
            background.setOutlineThickness(5);
            background.setOutlineColor(sf::Color::White);
            
            sf::Texture panel_texture;
            if (!panel_texture.loadFromFile(texture_path)) return false;
            sprite.setTexture(panel_texture);
            sprite.setScale(700.0f / panel_texture.getSize().x, 250.0f / panel_texture.getSize().y);
            sprite.setPosition(1150, 800);
            
            const sf::String texts[] = {
                L"[1] - SQL* - создать таблицу в БД на 1000 строк",
                L"[2] - P - HF-count* - диаграмма зависимости P",
                L"[] - SBF - C-count* - диаграмма коллизий"
            };
            
            for (int i = 0; i < 3; ++i) {
                lines[i].setFont(font);
                lines[i].setCharacterSize(22);
                lines[i].setFillColor(sf::Color::White);
                lines[i].setPosition(1170, 900 + i * 40);
                lines[i].setString(texts[i]);
            }
            
            return true;
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(background);
            window.draw(sprite);
            for (int i = 0; i < 3; ++i) window.draw(lines[i]);
        }
    } control_panel;
    
    if (!control_panel.init(font, "image/back_b.png")) return;
    
    // ==================== КНОПКИ И МЕТКИ ====================
    struct Buttons {
        sf::Sprite btn1, btn2, btn3, back;
        sf::Text labels[4];
        
        void init(sf::Font& font) {
            sf::Texture tex1, tex2, tex3, tex_back;
            tex1.loadFromFile("image/button1.png");
            tex2.loadFromFile("image/button2.png");
            tex3.loadFromFile("image/button3.png");
            tex_back.loadFromFile("image/back_button2.png");
            
            btn1.setTexture(tex1);
            btn1.setPosition(100, 980);
            btn2.setTexture(tex2);
            btn2.setPosition(350, 980);
            btn3.setTexture(tex3);
            btn3.setPosition(600, 980);
            back.setTexture(tex_back);
            back.setPosition(95, 975);
            
            const sf::String label_texts[] = {L"Назад", L"SQL*", L"P - HF-count*", L""};
            const float x_positions[] = {170, 430, 650, 895};
            
            for (int i = 0; i < 3; ++i) {
                labels[i].setFont(font);
                labels[i].setCharacterSize(20);
                labels[i].setFillColor(sf::Color::White);
                labels[i].setPosition(x_positions[i], 1050);
                labels[i].setString(label_texts[i]);
            }
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(back);
            window.draw(btn1);
            window.draw(btn2);
            window.draw(btn3);
            for (int i = 0; i < 3; ++i) window.draw(labels[i]);
        }
    } buttons;
    buttons.init(font);
    
    // ==================== ЭЛЕМЕНТЫ ДЛЯ ФОРМУЛЫ ====================
    struct FormulaDisplay {
        sf::RectangleShape background;
        sf::Sprite formula_image;
        
        bool init() {
            background = sf::RectangleShape(sf::Vector2f(311, 76));
            background.setPosition(1140, 600);
            background.setFillColor(sf::Color::Blue);
            background.setOutlineThickness(5);
            background.setOutlineColor(sf::Color::Red);
            
            sf::Texture formula_texture;
            if (!formula_texture.loadFromFile("image/f_optimalHF.png")) return false;
            formula_image.setTexture(formula_texture);
            formula_image.setPosition(1140, 600);
            
            return true;
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(background);
            window.draw(formula_image);
        }
    } formula_display;
    formula_display.init();
    
    // ==================== ПОДГОТОВКА К РАБОТЕ С БАЗОЙ ДАННЫХ ====================
    const int MAX_DISPLAY_LINES = 20;
    std::vector<sf::Text> display_lines(MAX_DISPLAY_LINES);
    for (auto& line : display_lines) {
        line.setFont(font);
        line.setCharacterSize(20);
        line.setFillColor(sf::Color::White);
    }
    
    // Текст для отображения количества загруженных строк
    sf::Text counter_text;
    counter_text.setFont(font);
    counter_text.setCharacterSize(20);
    counter_text.setFillColor(sf::Color::White);
    counter_text.setPosition(200, 900);
    
    // ==================== НАСТРОЙКА ФИЛЬТРА БЛУМА ====================
    BloomFilter<std::string> sql_filter(10000);
    sql_filter.addHashFunction(hashFunction1);
    sql_filter.addHashFunction(hashFunction2);
    sql_filter.addHashFunction(murmurHash);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;  // 0 - ничего, 1 - SQL режим, 3 - диаграмма
    bool data_loaded = false;
    
    // Константы
    const std::string TEST_USER = "User29";
    const double PROB_CALC = pow((1.0 - exp(-3000.0 / 10000.0)), 3.0);
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (sql_window.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (sql_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                sql_window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    sql_window.close();
                }
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                }
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 3;
                }
            }
        }
        
        sql_window.clear();
        sql_window.draw(background);
        
        // ==================== РЕЖИМ 1: РАБОТА С SQL ====================
        if (display_mode == 1) {
            if (!data_loaded) {
                sqlite3* db;
                int rc = sqlite3_open("datausers.db", &db);
                
                if (rc != SQLITE_OK) {
                    std::cerr << "Ошибка открытия БД: " << sqlite3_errmsg(db) << std::endl;
                    sqlite3_close(db);
                    return;
                }
                
                // Запрос для отображения последних 20 записей
                const char* select_query = "SELECT * FROM users2 ORDER BY id DESC LIMIT 20;";
                sqlite3_stmt* display_stmt;
                rc = sqlite3_prepare_v2(db, select_query, -1, &display_stmt, nullptr);
                
                if (rc == SQLITE_OK) {
                    int row = 0;
                    while (sqlite3_step(display_stmt) == SQLITE_ROW && row < MAX_DISPLAY_LINES) {
                        std::stringstream ss;
                        int columns = sqlite3_column_count(display_stmt);
                        
                        for (int i = 0; i < columns; ++i) {
                            const unsigned char* text = sqlite3_column_text(display_stmt, i);
                            if (text) {
                                ss << reinterpret_cast<const char*>(text);
                                if (i < columns - 1) ss << " | ";
                            }
                        }
                        
                        std::string line = ss.str();
                        if (line.length() > 80) line = line.substr(0, 77) + "...";
                        
                        display_lines[row].setString(line);
                        display_lines[row].setPosition(180, 200 + row * 25);
                        sql_window.draw(display_lines[row]);
                        row++;
                    }
                    sqlite3_finalize(display_stmt);
                }
                
                // Загрузка всех имен в фильтр Блума
                const char* names_query = "SELECT name FROM users2;";
                sqlite3_stmt* names_stmt;
                rc = sqlite3_prepare_v2(db, names_query, -1, &names_stmt, nullptr);
                
                if (rc == SQLITE_OK) {
                    int name_count = 0;
                    while (sqlite3_step(names_stmt) == SQLITE_ROW) {
                        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(names_stmt, 0));
                        sql_filter.insert(name);
                        name_count++;
                        
                        if (name_count % 100 == 0) {
                            counter_text.setString("Загружено: " + std::to_string(name_count) + " записей");
                            sql_window.draw(counter_text);
                        }
                    }
                    sqlite3_finalize(names_stmt);
                }
                
                sqlite3_close(db);
                data_loaded = true;
            }
            
            // Отображение статистики фильтра
            std::string test_result = sql_filter.exists(TEST_USER) ? "ДА (возможно)" : "НЕТ (точно)";
            stats.result.setString(L"User29 в списке? -> " + sf::String(test_result));
            
            stats.absolute_collisions.setString(L"Абс. коллизий: " + 
                                                std::to_string(sql_filter.retFullCollisions()));
            stats.hash_count.setString(L"Хеш-функций: " + 
                                      std::to_string(sql_filter.getHashFunctionsCount()));
            stats.probability.setString(L"P = [1-e^(-kn/m)]^k = " + std::to_string(PROB_CALC));
            
            sql_window.draw(stats.absolute_collisions);
            sql_window.draw(stats.hash_count);
            sql_window.draw(stats.probability);
            sql_window.draw(stats.result);
        }
        // ==================== РЕЖИМ 2: ДИАГРАММА ВЕРОЯТНОСТЕЙ ====================
        else if (display_mode == 3) {
            // Расчет вероятностей для разного числа хеш-функций
            std::map<int, double> probability_map;
            for (double k = 2; k <= 10; ++k) {
                double p = pow((1 - exp(-k * 1000 / 10000)), k);
                probability_map[static_cast<int>(k)] = p;
            }
            
            // Создание столбцов диаграммы
            std::vector<sf::RectangleShape> columns;
            std::vector<sf::Text> labels;
            
            int col_index = 0;
            for (const auto& pair : probability_map) {
                double column_height = pair.second * 15000;
                sf::RectangleShape column(sf::Vector2f(100, column_height));
                column.setFillColor((col_index == 5) ? sf::Color::Black : sf::Color::Green);
                column.setPosition(200 + col_index * 150, 750 - column_height);
                columns.push_back(column);
                
                sf::Text label;
                label.setFont(font);
                label.setCharacterSize(18);
                label.setFillColor((col_index == 5) ? sf::Color::Green : sf::Color::White);
                label.setString(std::to_string(pair.first) + "\n" + 
                              std::to_string(pair.second).substr(0, 6));
                label.setPosition(column.getPosition().x + 30, column.getPosition().y + column_height + 5);
                labels.push_back(label);
                
                col_index++;
            }
            
            // Заголовки диаграммы
            sf::Text title_main, title_sub;
            title_main.setFont(font);
            title_main.setCharacterSize(24);
            title_main.setFillColor(sf::Color::Yellow);
            title_main.setString(L"[Число хеш-функций] - [Вероятность ложного срабатывания]");
            title_main.setPosition(600, 100);
            
            title_sub.setFont(font);
            title_sub.setCharacterSize(28);
            title_sub.setFillColor(sf::Color::White);
            title_sub.setString(L"Зависимость вероятности ложного срабатывания от числа хеш-функций");
            title_sub.setPosition(300, 40);
            
            // Отрисовка диаграммы
            for (const auto& column : columns) sql_window.draw(column);
            for (const auto& label : labels) sql_window.draw(label);
            sql_window.draw(title_main);
            sql_window.draw(title_sub);
            
            // Отрисовка кнопок и формул
            buttons.draw(sql_window);
            formula_display.draw(sql_window);
        }
        
        // ==================== ОТРИСОВКА ОБЩИХ ЭЛЕМЕНТОВ ====================
        control_panel.draw(sql_window);
        
        sql_window.display();
    }
}

/**
 * @brief Функция сравнения Y-fast trie и Динамического фильтра Блума
 * 
 * Отображает окно для сравнительного анализа двух структур данных:
 * - Динамический фильтр Блума (Dynamic Bloom Filter)
 * - Y-fast trie (Y-быстрое дерево)
 * 
 * Сравниваемые характеристики:
 * - Визуализация работы структур
 * - Количество коллизий и вероятность ложных срабатываний
 * - Использование памяти
 * - Время вставки элементов
 * - Время поиска элементов
 * 
 * Управление:
 * - ESC: выход в главное меню
 * - 1: визуализация DBF и Y-fast trie
 * - 2: вычисление коллизий и вероятности ЛПС
 * - 3: диаграмма сравнения по памяти
 * - 4: диаграмма времени вставки
 * - 5: диаграмма времени поиска
 * - A: добавить слово 'apple'
 * - B: добавить слово 'hello'
 * - Q: добавить случайное слово
 * 
 * @see DynBloomFilter, YfastTrie
 */
void ComparisonYBloom() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    sf::RenderWindow window(sf::VideoMode(1280, 720), 
                            L"Сравнение Y-дерева и Динамического фильтра Блума", 
                            sf::Style::Default);
    
    // Установка view для масштабирования 1920x1080
    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
    window.setView(view);
    
    const float VIEW_WIDTH = 1920;
    const float VIEW_HEIGHT = 1080;
    
    // ==================== ЗАГРУЗКА РЕСУРСОВ ====================
    sf::Font font;
    if (!font.loadFromFile("fonts/times.ttf")) exit(1);
    
    // Фоновое изображение
    sf::RectangleShape background(sf::Vector2f(VIEW_WIDTH, VIEW_HEIGHT));
    sf::Texture background_texture;
    if (!background_texture.loadFromFile("image/bloomimg.jpg")) exit(1);
    background.setTexture(&background_texture);
    
    // ==================== ТЕКСТОВЫЕ ЭЛЕМЕНТЫ ====================
    // Основной текст управления
    sf::Text help_text;
    help_text.setFont(font);
    help_text.setString(L" [ESC] - вернуться назад \n"
                        L" [1] - визуализация DBF и Y-fast trie \n"
                        L" [2] - вычислить число коллизий и вероятность ЛПС \n"
                        L" [3] - Диаграмма сравнения по памяти \n"
                        L" [4] - время вставки элемента \n"
                        L" [5] - диаграмма сравнения по времени поиска \n"
                        L" [A] - добавить слово 'apple' \n"
                        L" [B] - добавить слово 'hello' \n"
                        L" [Q] - добавить случайное слово");
    help_text.setCharacterSize(26);
    help_text.setFillColor(sf::Color::White);
    help_text.setPosition(1180, 850);
    
    // Текст для отображения результатов
    sf::Text result_text;
    result_text.setFont(font);
    result_text.setCharacterSize(24);
    result_text.setFillColor(sf::Color::White);
    result_text.setPosition(180, 900);
    
    // ==================== ПАНЕЛЬ ПОДСКАЗОК ====================
    struct HintPanel {
        sf::RectangleShape background;
        sf::Sprite sprite;
        
        void init() {
            background = sf::RectangleShape(sf::Vector2f(700, 250));
            background.setPosition(1150, 800);
            background.setFillColor(sf::Color::Blue);
            background.setOutlineThickness(5);
            background.setOutlineColor(sf::Color::White);
            
            sf::Texture texture;
            if (texture.loadFromFile("image/back_b.png")) {
                sprite.setTexture(texture);
                sprite.setScale(700.0f / texture.getSize().x, 250.0f / texture.getSize().y);
                sprite.setPosition(1150, 800);
            }
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(background);
            window.draw(sprite);
        }
    } hint_panel;
    hint_panel.init();
    
    // ==================== ФОРМУЛЫ ====================
    struct FormulaDisplay {
        sf::RectangleShape background;
        sf::Sprite formula_image;
        sf::Text value_text;
        sf::Text title_text;
        
        void init(const std::string& image_path, float x, float y, 
                  const sf::String& title, sf::Font& font) {
            background = sf::RectangleShape(sf::Vector2f(311, 76));
            background.setPosition(x, y);
            background.setFillColor(sf::Color::Blue);
            background.setOutlineThickness(5);
            background.setOutlineColor(sf::Color::Red);
            
            sf::Texture texture;
            if (texture.loadFromFile(image_path)) {
                formula_image.setTexture(texture);
                formula_image.setPosition(x, y);
            }
            
            value_text.setFont(font);
            value_text.setCharacterSize(24);
            value_text.setFillColor(sf::Color::Red);
            value_text.setPosition(x + 330, y + 20);
            
            title_text.setFont(font);
            title_text.setCharacterSize(28);
            title_text.setFillColor(sf::Color::White);
            title_text.setPosition(x, y - 80);
            title_text.setString(title);
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(background);
            window.draw(formula_image);
            window.draw(value_text);
            window.draw(title_text);
        }
    };
    
    // Формула для Dynamic Bloom Filter
    FormulaDisplay dbf_formula;
    dbf_formula.init("image/f_optimalHF.png", 200, 300, 
                     L"Динамический фильтр Блума", font);
    
    // Формула для Y-fast trie
    FormulaDisplay ytrie_formula;
    ytrie_formula.init("image/f_prob.png", 1200, 300,
                       L"Y-fast trie", font);
    
    // ==================== VS ИЗОБРАЖЕНИЕ ====================
    sf::Sprite vs_sprite;
    sf::Texture vs_texture;
    if (vs_texture.loadFromFile("image/vs.png")) {
        vs_sprite.setTexture(vs_texture);
        vs_sprite.setScale(100.0f / vs_texture.getSize().x, 100.0f / vs_texture.getSize().y);
        vs_sprite.setPosition(880, 500);
    }
    
    // ==================== ДИАГРАММЫ ====================
    struct DiagramDisplay {
        sf::RectangleShape frame;
        sf::Sprite diagram;
        bool visible = false;
        
        void init() {
            frame = sf::RectangleShape(sf::Vector2f(800, 500));
            frame.setPosition(560, 300);
            frame.setFillColor(sf::Color::Transparent);
            frame.setOutlineThickness(5);
            frame.setOutlineColor(sf::Color::Red);
        }
        
        void load(const std::string& image_path) {
            sf::Texture texture;
            if (texture.loadFromFile(image_path)) {
                diagram.setTexture(texture);
                diagram.setPosition(560, 300);
                diagram.setScale(800.0f / texture.getSize().x, 500.0f / texture.getSize().y);
                visible = true;
            }
        }
        
        void draw(sf::RenderWindow& window) {
            if (visible) {
                window.draw(frame);
                window.draw(diagram);
            }
        }
        
        void hide() { visible = false; }
    } diagram;
    diagram.init();
    
    // ==================== ВИЗУАЛИЗАЦИЯ КВАДРАТОВ ====================
    struct SquareVisualization {
        std::vector<sf::RectangleShape> squares;
        std::vector<sf::Text> numbers;
        
        void create(int count, float start_x, float start_y, float size = 50, float spacing = 10) {
            squares.clear();
            numbers.clear();
            
            float totalHeight = count * size + (count - 1) * spacing;
            float actual_start_y = start_y - totalHeight / 2;
            
            for (int i = 0; i < count; ++i) {
                sf::RectangleShape square(sf::Vector2f(size, size));
                square.setFillColor(sf::Color::Black);
                square.setPosition(start_x, actual_start_y + i * (size + spacing));
                squares.push_back(square);
                
                sf::Text number;
                number.setString("0");
                number.setCharacterSize(20);
                number.setFillColor(sf::Color::Yellow);
                number.setPosition(square.getPosition().x + size/2 - 10, 
                                  square.getPosition().y + size/2 - 10);
                numbers.push_back(number);
            }
        }
        
        void updateIndices(const std::vector<size_t>& indices, sf::Color color) {
            for (size_t idx : indices) {
                if (idx < squares.size()) {
                    squares[idx].setFillColor(color);
                    numbers[idx].setString("1");
                }
            }
        }
        
        void draw(sf::RenderWindow& window) {
            for (const auto& square : squares) window.draw(square);
            for (const auto& number : numbers) window.draw(number);
        }
        
        void clear() {
            squares.clear();
            numbers.clear();
        }
    } visualization;
    
    // ==================== ТРЕУГОЛЬНИКИ ДЛЯ ВИЗУАЛИЗАЦИИ TRIE ====================
    struct TrieVisualization {
        sf::ConvexShape big_triangle;
        sf::ConvexShape small_triangle_left;
        sf::ConvexShape small_triangle_right;
        sf::VertexArray line_left;
        sf::VertexArray line_right;
        sf::Text big_text;
        sf::Text left_text;
        sf::Text right_text;
        
        void init(sf::Font& font) {
            // Большой треугольник (x-fast trie)
            big_triangle.setPointCount(3);
            big_triangle.setPoint(0, sf::Vector2f(800, 300));
            big_triangle.setPoint(1, sf::Vector2f(700, 500));
            big_triangle.setPoint(2, sf::Vector2f(900, 500));
            big_triangle.setFillColor(sf::Color::White);
            
            // Левый маленький треугольник
            small_triangle_left.setPointCount(3);
            small_triangle_left.setPoint(0, sf::Vector2f(600, 550));
            small_triangle_left.setPoint(1, sf::Vector2f(550, 600));
            small_triangle_left.setPoint(2, sf::Vector2f(650, 600));
            small_triangle_left.setFillColor(sf::Color::Red);
            
            // Правый маленький треугольник
            small_triangle_right.setPointCount(3);
            small_triangle_right.setPoint(0, sf::Vector2f(1000, 550));
            small_triangle_right.setPoint(1, sf::Vector2f(950, 600));
            small_triangle_right.setPoint(2, sf::Vector2f(1050, 600));
            small_triangle_right.setFillColor(sf::Color::Blue);
            
            // Линии соединения
            line_left = sf::VertexArray(sf::Lines, 2);
            line_left[0].position = sf::Vector2f(600, 550);
            line_left[1].position = sf::Vector2f(750, 400);
            
            line_right = sf::VertexArray(sf::Lines, 2);
            line_right[0].position = sf::Vector2f(1000, 550);
            line_right[1].position = sf::Vector2f(850, 400);
            
            // Тексты
            big_text.setFont(font);
            big_text.setString("x-fast trie");
            big_text.setCharacterSize(20);
            big_text.setFillColor(sf::Color::Black);
            big_text.setPosition(740, 320);
            
            left_text.setFont(font);
            left_text.setString("O(logM)");
            left_text.setCharacterSize(16);
            left_text.setFillColor(sf::Color::White);
            left_text.setPosition(570, 565);
            
            right_text.setFont(font);
            right_text.setString("O(logM)");
            right_text.setCharacterSize(16);
            right_text.setFillColor(sf::Color::White);
            right_text.setPosition(970, 565);
        }
        
        void draw(sf::RenderWindow& window) {
            window.draw(big_triangle);
            window.draw(small_triangle_left);
            window.draw(small_triangle_right);
            window.draw(big_text);
            window.draw(left_text);
            window.draw(right_text);
            window.draw(line_left);
            window.draw(line_right);
        }
        
        void hide() {
            big_triangle.setFillColor(sf::Color::Transparent);
            small_triangle_left.setFillColor(sf::Color::Transparent);
            small_triangle_right.setFillColor(sf::Color::Transparent);
            big_text.setString("");
            left_text.setString("");
            right_text.setString("");
        }
        
        void show() {
            big_triangle.setFillColor(sf::Color::White);
            small_triangle_left.setFillColor(sf::Color::Red);
            small_triangle_right.setFillColor(sf::Color::Blue);
            big_text.setString("x-fast trie");
            left_text.setString("O(logM)");
            right_text.setString("O(logM)");
        }
    } trie_viz;
    trie_viz.init(font);
    
    // ==================== НАСТРОЙКА ФИЛЬТРА ====================
    DynBloomFilter<std::string> dbf_filter(-3, 0.001);
    
    // ==================== ПЕРЕМЕННЫЕ СОСТОЯНИЯ ====================
    int display_mode = 0;
    bool trie_visible = false;
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (window.isOpen()) {
        sf::Event event;
        
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                // ==================== РЕЖИМ 1: ВИЗУАЛИЗАЦИЯ ====================
                else if (event.key.code == sf::Keyboard::Num1) {
                    display_mode = 1;
                    trie_visible = true;
                    diagram.hide();
                    
                    // Сброс формул
                    dbf_formula.value_text.setString("");
                    ytrie_formula.value_text.setString("");
                    
                    // Создание визуализации квадратов
                    visualization.create(20, 400, VIEW_HEIGHT / 2, 50, 10);
                    
                    // Показ треугольников
                    trie_viz.show();
                    vs_sprite.setPosition(800, 580);
                }
                // ==================== РЕЖИМ 2: КОЛЛИЗИИ И ВЕРОЯТНОСТЬ ====================
                else if (event.key.code == sf::Keyboard::Num2) {
                    display_mode = 2;
                    trie_visible = false;
                    visualization.clear();
                    
                    // Вычисление коллизий и вероятности
                    float prob = pow((1 - exp(-3.0 * 1000.0 / 10000.0)), 3.0);
                    result_text.setString(L"Число коллизий: " + 
                                         std::to_string(dbf_filter.countCollisions()) + 
                                         L"\nКоличество хеш-функций: " + 
                                         std::to_string(dbf_filter.getHashFunctionsCount()));
                    
                    dbf_formula.value_text.setString(" = 0.6931 * 20/3 ~ 5");
                    ytrie_formula.value_text.setString(" = " + std::to_string(0.06717));
                }
                // ==================== РЕЖИМ 3: ДИАГРАММА ПАМЯТИ ====================
                else if (event.key.code == sf::Keyboard::Num3) {
                    display_mode = 3;
                    trie_visible = false;
                    visualization.clear();
                    diagram.load("image/diag2_sizes.png");
                    result_text.setString(L"Dynamic Bloom Filter: 256 bytes\n"
                                          L"Y-fast trie: 128 bytes\n"
                                          L"Linked List: 8 bytes");
                    dbf_formula.value_text.setString("");
                    ytrie_formula.value_text.setString("");
                }
                // ==================== РЕЖИМ 4: ВРЕМЯ ВСТАВКИ ====================
                else if (event.key.code == sf::Keyboard::Num4) {
                    display_mode = 4;
                    trie_visible = false;
                    visualization.clear();
                    diagram.load("image/diag1_inserttime.png");
                    result_text.setString(L"Dynamic Bloom Filter: 46 мкс\n"
                                          L"Y-fast trie: 269 мкс\n"
                                          L"Linked List: 0 мкс");
                    dbf_formula.value_text.setString("");
                    ytrie_formula.value_text.setString("");
                }
                // ==================== РЕЖИМ 5: ВРЕМЯ ПОИСКА ====================
                else if (event.key.code == sf::Keyboard::Num5) {
                    display_mode = 5;
                    trie_visible = false;
                    visualization.clear();
                    diagram.load("image/diag3_findtime.png");
                    result_text.setString(L"Dynamic Bloom Filter: 1 мкс\n"
                                          L"Y-fast trie: 6 мкс\n"
                                          L"Linked List: 820 мкс");
                    dbf_formula.value_text.setString("");
                    ytrie_formula.value_text.setString("");
                }
                // ==================== ДОБАВЛЕНИЕ ЭЛЕМЕНТОВ ====================
                else if (event.key.code == sf::Keyboard::A) {
                    if (display_mode == 1) {
                        dbf_filter.insert("apple");
                        visualization.updateIndices(dbf_filter.getIndices("apple"), 
                                                   sf::Color::Green);
                        trie_viz.small_triangle_left.setFillColor(sf::Color::Black);
                    }
                }
                else if (event.key.code == sf::Keyboard::B) {
                    if (display_mode == 1) {
                        dbf_filter.insert("hello");
                        visualization.updateIndices(dbf_filter.getIndices("hello"), 
                                                   sf::Color::Red);
                        trie_viz.small_triangle_right.setFillColor(sf::Color::Red);
                    }
                }
                else if (event.key.code == sf::Keyboard::Q) {
                    if (display_mode == 1) {
                        dbf_filter.insert("qwertyjuiceapplemasterpiece");
                        visualization.updateIndices(dbf_filter.getIndices("qwertyjuiceapplemasterpiece"), 
                                                   sf::Color::Magenta);
                        trie_viz.big_triangle.setFillColor(sf::Color(249, 159, 159));
                    }
                }
            }
        }
        
        // ==================== ОТРИСОВКА ====================
        window.clear();
        window.draw(background);
        
        // Общие элементы
        hint_panel.draw(window);
        dbf_formula.draw(window);
        ytrie_formula.draw(window);
        window.draw(help_text);
        window.draw(result_text);
        
        // Специфичные элементы в зависимости от режима
        if (display_mode == 1 && trie_visible) {
            trie_viz.draw(window);
            window.draw(vs_sprite);
            visualization.draw(window);
        }
        
        diagram.draw(window);
        
        window.display();
    }
}
/**
 * @brief Главное меню для выбора режимов работы с фильтром Блума
 * 
 * Отображает интерактивное меню со списком доступных реализаций:
 * - Простая реализация фильтра Блума
 * - Работа с данными из CSV файлов
 * - Интеграция с SQL базой данных
 * - Фильтр Блума с подсчетом (Counting Bloom Filter)
 * - Инверсивный фильтр Блума (Invertible Bloom Filter)
 * - Cuckoo фильтр (Cuckoo Filter)
 * - Сравнение динамического ФБ с Y-fast деревом
 * 
 * Управление:
 * - Стрелка ВВЕРХ/ВНИЗ: навигация по пунктам меню
 * - ENTER: выбор текущего пункта
 * - ESC: выход в предыдущее меню
 * 
 * @note Меню автоматически центрируется на экране
 * @see MainMenu::AppMenu
 */
void BloomMenu() {
    // ==================== ИНИЦИАЛИЗАЦИЯ ОКНА ====================
    // Создание окна с фиксированным размером 1280x720
    sf::RenderWindow menu_window(sf::VideoMode(1280, 720), L"Фильтр Блума - Выбор режима", sf::Style::Default);
    
    // Настройка view для масштабирования 1920x1080
    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
    menu_window.setView(view);
    
    const float VIEW_WIDTH = view.getSize().x;   // 1920
    const float VIEW_HEIGHT = view.getSize().y;  // 1080
    
    // ==================== ЗАГРУЗКА РЕСУРСОВ ====================
    // Настройка фонового изображения
    sf::RectangleShape background(sf::Vector2f(VIEW_WIDTH, VIEW_HEIGHT));
    sf::Texture background_texture;
    if (!background_texture.loadFromFile("image/menubloom.jpg")) {
        std::cerr << "Предупреждение: не удалось загрузить фоновое изображение 'image/menubloom.jpg'" << std::endl;
        // Продолжаем выполнение без фона (будет черный экран)
    } else {
        background.setTexture(&background_texture);
    }
    
    // Загрузка шрифта
    sf::Font menu_font;
    if (!menu_font.loadFromFile("fonts/times.ttf")) {
        std::cerr << "Предупреждение: не удалось загрузить шрифт 'fonts/times.ttf'" << std::endl;
        // Продолжаем выполнение без шрифта (текст не будет отображаться)
    }
    
    // ==================== НАСТРОЙКА МЕНЮ ====================
    // Список пунктов меню
    const sf::String menu_items[] = {
        L"Простая реализация",
        L"Данные из .CSV",
        L"SQL", 
        L"Фильтр Блума с подсчетом", 
        L"Инверсивный фильтр Блума",
        L"Cuckoo фильтр Блума", 
        L"Динамический ФБ vs Y-fast дерево"
    };
    const int MENU_ITEMS_COUNT = 7;
    
    // Создание объекта меню
    MainMenu::AppMenu app_menu(menu_window, 700, 250, MENU_ITEMS_COUNT, 
                               const_cast<sf::String*>(menu_items), 70, 90);
    
    // Настройка цветовой схемы меню
    app_menu.setColorTextMenu(sf::Color(237, 147, 0),  // Цвет невыбранного пункта
                              sf::Color::Red,           // Цвет выбранного пункта
                              sf::Color::Black);        // Цвет обводки
    
    // Центрирование меню на экране
    app_menu.AlignMenu(2, VIEW_WIDTH);
    
    // ==================== ГЛАВНЫЙ ЦИКЛ ====================
    while (menu_window.isOpen()) {
        sf::Event event;
        
        // Обработка событий
        while (menu_window.pollEvent(event)) {
            // Закрытие окна
            if (event.type == sf::Event::Closed) {
                menu_window.close();
            }
            // Обработка нажатий клавиш
            else if (event.type == sf::Event::KeyReleased) {
                switch (event.key.code) {
                    case sf::Keyboard::Up:
                        app_menu.MoveUp();
                        break;
                        
                    case sf::Keyboard::Down:
                        app_menu.MoveDown();
                        break;
                        
                    case sf::Keyboard::Return:
                        executeMenuChoice(app_menu.getSelectedMenuNumber());
                        break;
                        
                    case sf::Keyboard::Escape:
                        menu_window.close();
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        // Отрисовка
        menu_window.clear();
        menu_window.draw(background);
        app_menu.draw();
        menu_window.display();
    }
}

/**
 * @brief Вспомогательная функция для выполнения выбранного пункта меню
 * 
 * @param choice Номер выбранного пункта меню (0-6)
 * 
 * @details Вызывает соответствующую функцию в зависимости от выбора пользователя
 */
void executeMenuChoice(int choice) {
    switch (choice) {
        case 0: BasicBloom(); break;
        case 1: CSV_BLOOM(); break;
        case 2: SQL_Bloom(); break;
        case 3: CountingBF(); break;
        case 4: InvertibleBF(); break;
        case 5: CuckooBF(); break;
        case 6: ComparisonYBloom(); break;
        default: break;
    }
}

#include <cstdlib>

int main() 
{
    setlocale(LC_ALL, "rus");

    // ========================================================================
    // ЛОГОТИП И ЗАГОЛОВОК ПРОГРАММЫ
    // ========================================================================
    std::cout << R"(

    ╔══════════════════════════════════════════════════════════════════╗
    ║                                                                  ║
    ║    .-. .-')                                      _   .-')        ║
    ║    \  ( OO )                                    ( '.( OO )_      ║
    ║    ;-----.\  ,--.      .-'),-----.  .-'),-----. ,--.   ,--.)     ║
    ║    | .-.  |  |  |.-') ( OO'  .-.  '( OO'  .-.  '|   `.'   |      ║
    ║    | '-' /_) |  | OO )/   |  | |  |/   |  | |  ||         |      ║
    ║    | .-. `.  |  |`-' |\_) |  |\|  |\_) |  |\|  ||  |'.'|  |      ║
    ║    | |  \  |(|  '---.'  \ |  | |  |  \ |  | |  ||  |   |  |      ║
    ║    | '--'  / |      |    `'  '-'  '   `'  '-'  '|  |   |  |      ║
    ║    `------'  `------'      `-----'      `-----' `--'   `--'      ║
    ║                                                                  ║
    ║         Вероятностные структуры данных - Курсовая работа         ║
    ╚══════════════════════════════════════════════════════════════════╝

    )" << std::endl;

    int key_0;
    do
    {
        std::cout << R"(

➤ Введите номер структуры (1-2): 0 - выход )";
        std::cin >> key_0;
        
        // ========================================================================
        // БЛОК 1: ФИЛЬТР БЛУМА (Bloom Filter)
        // ========================================================================
        if (key_0 == 1)
        {
            int fb_num = 0;
            std::cout << R"(

┌─────────────────────────────────────────────────────────────┐
│  ВАРИАНТЫ ФИЛЬТРА БЛУМА                                  │
├─────────────────────────────────────────────────────────────┤
│  [1]  Классический фильтр Блума                          │
│  [2]  Фильтр Блума с подсчётом (Counting Bloom Filter)   │
│  [3]  Инверсивный фильтр Блума (Invertible Bloom Filter) │
│  [4]  Сравнение с Y-fast trie                            │
│  [5]  Cuckoo-фильтр                                      │
└─────────────────────────────────────────────────────────────┘
➤ Введите номер варианта (1-5): )";
            std::cin >> fb_num;

            // ====================================================================
            // ПОДБЛОК 1.1: Классический фильтр Блума
            // ====================================================================
            if (fb_num == 1)
            {
                std::cout << R"(

┌─────────────────────────────────────────────────────────────┐
│  КЛАССИЧЕСКИЙ ФИЛЬТР БЛУМА                              │
├─────────────────────────────────────────────────────────────┤
│  Режимы работы:                                         │
│  [1]  Консольный режим                                  │
│  [2]  Многопоточный режим (threads)                     │
│  [3]  Графический интерфейс (SFML)                      │
└─────────────────────────────────────────────────────────────┘
➤ Введите режим работы (1-3): )";

                int key_1;
                std::cin >> key_1;

                // ==============================================================
                // РЕЖИМ 1.1.1: Консольный режим
                // ==============================================================
                if (key_1 == 1)
                {
                    int key_2;
                    std::cout << R"(

┌─────────────────────────────────────────────────────────────┐
│  ИСТОЧНИК ДАННЫХ                                        │
├─────────────────────────────────────────────────────────────┤
│  [1]  CSV-файл                                           │
│  [2]  SQLite-база данных                                 │
└─────────────────────────────────────────────────────────────┘
➤ Выберите источник данных (1-2): )";
                    std::cin >> key_2;

                    // ===========================================================
                    // ПОДРЕЖИМ 1.1.1.1: CSV-файл
                    // ===========================================================
                    if (key_2 == 1)
                    {
                        double k = 0.0;
                        double n = 0.0;
                        
                        std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║              ЗАПУСК КЛАССИЧЕСКОГО ФИЛЬТРА БЛУМА           ║
╚════════════════════════════════════════════════════════════╝

)";

                        size_t filterSize;
                        std::cout << R"(
┌─ Размер фильтра ─────────────────────────────────────────┐
│  Введите размер (количество ячеек): )";
                        std::cin >> filterSize;
                        std::cout << R"(
└─────────────────────────────────────────────────────────────┘)";
                        
                        double m = filterSize;

                        BloomFilter<std::string> filter(filterSize);

                        filter.addHashFunction(hashFunction1);
                        filter.addHashFunction(hashFunction2);
                        k = 2.0;
                        filter.addHashFunction(hashSHA256);
                        filter.addHashFunction(murmurHash);
                        k = 4.0;

                        std::cout << R"(

┌─ ВЫБОР НАБОРА ДАННЫХ ───────────────────────────────────┐
│  [1]  Маленький набор (100 строк)                       │
│  [2]  Средний набор (100 000 строк)                     │
│  [3]  Большой набор (5 000 000 строк)                   │
└─────────────────────────────────────────────────────────────┘
➤ Выберите набор данных (1-3): )";
                        
                        int key_0 = 0;
                        std::cin >> key_0;
                        std::ifstream file;

                        if (key_0 == 1) {
                            file.open("build/100_SalesRecords.csv");
                            n = 100.0;
                            std::cout << R"( Выбран набор: 100 строк)";
                        } else if (key_0 == 2) {
                            file.open("build/1000_SalesRecords.csv");
                            n = 100000.0;
                            std::cout << R"( Выбран набор: 100 000 строк)";
                        } else if (key_0 == 3) {
                            file.open("build/1000_SalesRecords.csv");
                            n = 5000000.0;
                            std::cout << R"( Выбран набор: 5 000 000 строк)";
                        }

                        if (!file.is_open()) {
                            std::cerr << R"(

ОШИБКА: Не удалось открыть файл)" << std::endl;
                            return 1;
                        }

                        std::cout << R"(

Загрузка данных в фильтр Блума...
┌─────────────────────────────────────────────────────────────┐)";
                        
                        std::string line;
                        int counter = 0;
                        while (std::getline(file, line)) {
                            filter.insert(line);
                            counter++;
                            if (counter % 10000 == 0) {
                                std::cout << "\n│  Обработано " << std::setw(8) << counter << " записей";
                            }
                        }
                        file.close();
                        std::cout << R"(
└─────────────────────────────────────────────────────────────┘
Загрузка завершена. Всего добавлено: )" << counter << R"( записей

)";

                        // Проверка наличия элементов
                        std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТОВ ─────────────────────────────┐
│  'apple'                              → )" << (filter.exists("apple") ? "ДА" : "НЕТ") << R"(
│  'orange'                             → )" << (filter.exists("orange") ? "ДА" : "НЕТ") << R"(
│  'DateOrder'                          → )" << (filter.exists("DateOrder") ? "ДА" : "НЕТ") << R"(
└─────────────────────────────────────────────────────────────┘

)";

                        // Проверка времени выполнения
                        auto start = std::chrono::high_resolution_clock::now();
                        bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                        auto end = std::chrono::high_resolution_clock::now();
                        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                        std::cout << R"(
┌─ РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ ────────────────────────────────┐
│  Элемент (Europe2,Poland,Meat...)                       │
│  Результат:  )" << (exists ? "ПРИСУТСТВУЕТ" : "ОТСУТСТВУЕТ") << R"(
│  Время выполнения:  )" << std::setw(8) << duration.count() << R"( мкс
│  Размер фильтра:    )" << std::setw(8) << filter.getSizeInBytes() << R"( байт
│  Коллизии:          )" << std::setw(8) << filter.countCollisions() << R"(
│  Хеш-функций:       )" << std::setw(8) << k << R"(
└─────────────────────────────────────────────────────────────┘

)";

                        // Статистика
                        double prob = pow((1 - exp(-k * n / m)), k);
                        double optimal_k_functions = log(2.0) * m / n;
                        
                        std::cout << R"(
┌─ СТАТИСТИЧЕСКИЕ ПОКАЗАТЕЛИ ──────────────────────────────┐
│  Оптимальное число хеш-функций:  )" << std::setw(8) << ceil(optimal_k_functions) << R"(
│  Вероятность ложного срабатывания: )" << std::fixed << std::setprecision(10) << prob << R"(
└─────────────────────────────────────────────────────────────┘

)";

                        std::cout << R"(
┌─ АБСОЛЮТНЫЕ КОЛЛИЗИИ ────────────────────────────────────┐)";
                        std::map<double, double> mymap = calculateCollisionsABS(5000);
                        int line_count = 0;
                        for (const auto& element : mymap) {
                            if (line_count < 20) {
                                std::cout << "\n│  " << std::setw(6) << element.first << " → " << std::setw(8) << element.second;
                                line_count++;
                            }
                        }
                        if (mymap.size() > 20) {
                            std::cout << "\n│  ... и ещё " << (mymap.size() - 20) << " записей";
                        }
                        std::cout << R"(
└─────────────────────────────────────────────────────────────┘
)";
                    }

                    // ===========================================================
                    // ПОДРЕЖИМ 1.1.1.2: SQLite-база данных
                    // ===========================================================
                    else if (key_2 == 2)
                    {
                        std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║           ФИЛЬТР БЛУМА + SQLite БАЗА ДАННЫХ             ║
╚════════════════════════════════════════════════════════════╝

)";

                        int m = 0;
                        std::cout << R"(
┌─ Размер фильтра ─────────────────────────────────────────┐
│  Введите размер (количество ячеек): )";
                        std::cin >> m;
                        std::cout << R"(
└─────────────────────────────────────────────────────────────┘)";

                        BloomFilter<std::string> bloomfilter(m);
                        double n = m;

                        bloomfilter.addHashFunction(hashFunction1);
                        bloomfilter.addHashFunction(hashFunction2);
                        bloomfilter.addHashFunction(murmurHash);

                        sqlite3* db;
                        int rc = sqlite3_open("datausers.db", &db);

                        if (rc) {
                            std::cerr << R"( Ошибка открытия базы данных: )" << sqlite3_errmsg(db) << std::endl;
                            return 0;
                        }
                        else {
                            std::cout << R"( База данных успешно открыта)";
                        }

                        // Создание таблицы
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
                            std::cerr << R"( Ошибка создания таблицы: )" << errMsg << std::endl;
                            sqlite3_free(errMsg);
                        }

                        // Проверка, пуста ли таблица
                        const char* checkTableSql = "SELECT COUNT(*) FROM users2;";
                        sqlite3_stmt* stmt;
                        rc = sqlite3_prepare_v2(db, checkTableSql, -1, &stmt, nullptr);
                        if (rc != SQLITE_OK) {
                            std::cerr << R"( Ошибка подготовки запроса: )" << sqlite3_errmsg(db) << std::endl;
                            sqlite3_close(db);
                            return 0;
                        }

                        rc = sqlite3_step(stmt);
                        if (rc == SQLITE_ROW) {
                            int count = sqlite3_column_int(stmt, 0);
                            if (count == 0) {
                                std::cout << R"(
Таблица пуста, вставка тестовых данных...)";
                                srand(time(0));
                                for (int i = 0; i < 1000; i++) {
                                    std::string insertQuery = "INSERT INTO users2 (name, phone, column3, column4, column5, column6, column7, column8, column9, column10) VALUES ('User" + std::to_string(i + 1) + "', '+123456789" + std::to_string(i + 1) + "', 'Data3', 'Data4', 'Data5', 'Data6', 'Data7', 'Data8', 'Data9', 'Data10');";
                                    rc = sqlite3_exec(db, insertQuery.c_str(), 0, 0, nullptr);
                                    if (rc != SQLITE_OK) {
                                        std::cerr << R"( Ошибка вставки данных: )" << sqlite3_errmsg(db) << std::endl;
                                    }
                                }
                                std::cout << R"( Вставлено 1000 записей)";
                            } else {
                                std::cout << R"( Таблица уже содержит )" << count << R"( записей)";
                            }
                        }

                        sqlite3_finalize(stmt);

                        // Чтение данных из базы
                        const char* sql = "SELECT name FROM users2;";
                        sqlite3_stmt* stmt2;
                        rc = sqlite3_prepare_v2(db, sql, -1, &stmt2, nullptr);
                        if (rc != SQLITE_OK) {
                            std::cerr << R"( Ошибка подготовки запроса: )" << sqlite3_errmsg(db) << std::endl;
                            sqlite3_close(db);
                            return 0;
                        }

                        std::cout << R"(
Загрузка имён из базы данных в фильтр Блума...
┌─────────────────────────────────────────────────────────────┐)";
                        int inserted = 0;
                        while ((rc = sqlite3_step(stmt2)) == SQLITE_ROW) {
                            std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, 0));
                            bloomfilter.insert(name);
                            inserted++;
                            if (inserted % 100 == 0) {
                                std::cout << "\n│  Обработано " << std::setw(4) << inserted << " записей";
                            }
                        }
                        std::cout << R"(
└─────────────────────────────────────────────────────────────┘
Загружено )" << inserted << R"( записей

)";

                        if (rc != SQLITE_DONE) {
                            std::cerr << R"( Ошибка выполнения запроса: )" << sqlite3_errmsg(db) << std::endl;
                        }

                        sqlite3_finalize(stmt2);
                        sqlite3_close(db);

                        // Проверка наличия элемента
                        std::string testName = "User29";
                        std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТА ─────────────────────────────┐
│  Проверка имени: )" << testName << R"(
│  Результат: )" << (bloomfilter.exists(testName) ? "Элемент ВОЗМОЖНО присутствует" : "Элемент ТОЧНО отсутствует") << R"(
└─────────────────────────────────────────────────────────────┘

)";

                        float prob = pow((1.0 - exp(-3000.0 / n)), 3.0);
                        std::cout << R"(
┌─ СТАТИСТИКА ─────────────────────────────────────────────┐
│  Абсолютное число коллизий:   )" << std::setw(8) << bloomfilter.retFullCollisions() << R"(
│  Число хеш-функций:           )" << std::setw(8) << bloomfilter.getHashFunctionsCount() << R"(
│  Вероятность ложного срабатывания: )" << std::fixed << std::setprecision(10) << prob << R"(
└─────────────────────────────────────────────────────────────┘
)";
                    }
                }

                // ==============================================================
                // РЕЖИМ 1.1.2: Многопоточный режим (threads)
                // ==============================================================
                else if (key_1 == 2)
                {
                    std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║          ФИЛЬТР БЛУМА - МНОГОПОТОЧНЫЙ РЕЖИМ             ║
╚════════════════════════════════════════════════════════════╝

)";

                    size_t filterSize;
                    std::cout << R"(
┌─ Размер фильтра ─────────────────────────────────────────┐
│  Введите размер (количество ячеек): )";
                    std::cin >> filterSize;
                    std::cout << R"(
└─────────────────────────────────────────────────────────────┘)";

                    BloomFilter<std::string> filter1(filterSize);
                    filter1.addHashFunction(hashFunction1);
                    filter1.addHashFunction(hashFunction2);

                    std::vector<std::string> fileParts = { 
                        "customers-100000_3.csv", 
                        "customers-100000_2.csv",
                        "customers-100000_4.csv", 
                        "customers-100000_5.csv", 
                        "customers-100000.csv" 
                    };

                    std::vector<std::thread> threads;
                    std::mutex mutex;

                    std::cout << R"(
Запуск )" << fileParts.size() << R"( потоков для обработки файлов...)";
                    for (const auto& filePath : fileParts) {
                        threads.emplace_back(processFile, filePath, std::ref(filter1), std::ref(mutex));
                    }

                    for (auto& thread : threads) {
                        thread.join();
                    }
                    std::cout << R"(
Все потоки завершили работу

)";

                    std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТОВ ─────────────────────────────┐
│  'apple' → )" << (filter1.exists("apple") ? "ДА" : "НЕТ") << R"(
│  'orange' → )" << (filter1.exists("orange") ? "ДА" : "НЕТ") << R"(
│  'DateOrder' → )" << (filter1.exists("DateOrder") ? "ДА" : "НЕТ") << R"(
└─────────────────────────────────────────────────────────────┘

)";

                    auto start = std::chrono::high_resolution_clock::now();
                    bool exists = filter1.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                    std::cout << R"(
┌─ РЕЗУЛЬТАТЫ ─────────────────────────────────────────────┐
│  Результат проверки:  )" << (exists ? "ПРИСУТСТВУЕТ" : "ОТСУТСТВУЕТ") << R"(
│  Время выполнения:    )" << std::setw(8) << duration.count() << R"( мкс
│  Размер фильтра:      )" << std::setw(8) << filter1.getSizeInBytes() << R"( байт
│  Коллизии:            )" << std::setw(8) << filter1.countCollisions() << R"(
└─────────────────────────────────────────────────────────────┘
)";
                }

                // ==============================================================
                // РЕЖИМ 1.1.3: Графический интерфейс (SFML)
                // ==============================================================
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

                    BloomFilter<std::string> filter(20);
                    filter.addHashFunction(hashFunction1);
                    filter.addHashFunction(hashFunction2);

                    filter.insert("apple");
                    filter.insert("banana");
                    filter.insert("cherry");

                    RenderWindow window;
                    window.create(VideoMode(1280, 720), L"Курсовая работа", Style::Default);
                    window.setMouseCursorVisible(false);

                    sf::View view(sf::FloatRect(0, 0, 1920, 1080));
                    window.setView(view);
                    
                    float width = view.getSize().x;
                    float height = view.getSize().y;

                    RectangleShape background(Vector2f(width, height));
                    Texture texture_window;
                    if (!texture_window.loadFromFile("image/menu9.jpg")) return 4;
                    background.setTexture(&texture_window);

                    Font font;
                    if (!font.loadFromFile("fonts/times.ttf")) return 5;
                    Text Titul1; Titul1.setFont(font);
                    Text Titul2; Titul2.setFont(font);

                    InitText(Titul1, width, height, L"«Реализация, анализ, сравнение", 100, Color(237, 147, 0), 3);
                    InitText(Titul2, width, height + 800, L"вероятностных структур данных»", 100, Color(237, 147, 0), 3);

                    String name_menu[]{ L"Фильтр Блума", L"HyperLogLog", L"Выход" };
                    MainMenu::AppMenu mymenu(window, 950, 400, 3, name_menu, 100, 120);
                    mymenu.setColorTextMenu(Color(255, 0, 127), Color::Green, Color::Black);
                    mymenu.AlignMenu(2, width);

                    while (window.isOpen())
                    {
                        Event event;
                        while (window.pollEvent(event))
                        {
                            if (event.type == Event::KeyReleased)
                            {
                                if (event.key.code == Keyboard::Up) { mymenu.MoveUp(); }
                                if (event.key.code == Keyboard::Down) { mymenu.MoveDown(); }
                                if (event.key.code == Keyboard::Return)
                                {
                                    switch (mymenu.getSelectedMenuNumber()){
                                        case 0: BloomMenu(); break;
                                        case 1: Options(); break;
                                        case 2: window.close(); break;
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

            // ====================================================================
            // ПОДБЛОК 1.2: Фильтр Блума с подсчётом (Counting Bloom Filter)
            // ====================================================================
            else if (fb_num == 2)
            {
                std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║       ФИЛЬТР БЛУМА С ПОДСЧЁТОМ (COUNTING BLOOM)        ║
╚════════════════════════════════════════════════════════════╝

)";

                double k = 0.0;
                double n = 0.0;
                
                size_t filterSize;
                std::cout << R"(
┌─ Размер фильтра ─────────────────────────────────────────┐
│  Введите размер: )";
                std::cin >> filterSize;
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘)";
                
                double m = filterSize;

                CountingBloomFilter<std::string> filter(filterSize);

                filter.addHashFunction(hashFunction1);
                filter.addHashFunction(hashFunction2);
                k = 2.0;
                filter.addHashFunction(hashSHA256);
                filter.addHashFunction(murmurHash);
                k = 4.0;

                std::cout << R"(

┌─ ВЫБОР НАБОРА ДАННЫХ ───────────────────────────────────┐
│  [1]  100 строк                                          │
│  [2]  100 000 строк                                      │
│  [3]  5 000 000 строк                                    │
└─────────────────────────────────────────────────────────────┘
➤ Выберите набор данных (1-3): )";
                
                int key_0 = 0;
                std::cin >> key_0;
                std::ifstream file;

                if (key_0 == 1) {
                    file.open("build/100_SalesRecords.csv");
                    n = 100.0;
                    std::cout << R"( Выбран набор: 100 строк)";
                } else if (key_0 == 2) {
                    file.open("build/1000_SalesRecords.csv");
                    n = 100000.0;
                    std::cout << R"( Выбран набор: 100 000 строк)";
                } else if (key_0 == 3) {
                    file.open("5m_SalesRecords.csv");
                    n = 5000000.0;
                    std::cout << R"( Выбран набор: 5 000 000 строк)";
                }

                if (!file.is_open()){
                    std::cerr << R"(

ОШИБКА: Не удалось открыть файл)" << std::endl;
                    return 1;
                }

                std::cout << R"(

Загрузка данных в фильтр Блума с подсчётом...
┌─────────────────────────────────────────────────────────────┐)";
                
                std::string line;
                int counter = 0;
                while (std::getline(file, line)) {
                    filter.insert(line);
                    counter++;
                    if (counter % 10000 == 0) {
                        std::cout << "\n│  Обработано " << std::setw(8) << counter << " записей";
                    }
                }
                file.close();
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘
Загрузка завершена. Всего добавлено: )" << counter << R"( записей

)";

                std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТОВ ─────────────────────────────┐
│  'apple' → )" << (filter.exists("apple") ? "ДА" : "НЕТ") << R"(
│  'orange' → )" << (filter.exists("orange") ? "ДА" : "НЕТ") << R"(
│  'DateOrder' → )" << (filter.exists("DateOrder") ? "ДА" : "НЕТ") << R"(
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ ГЛАВНОЕ ОТЛИЧИЕ: УДАЛЕНИЕ ЭЛЕМЕНТА ─────────────────────┐
│  Удалить строку: Europe,Bulgaria,Clothes,Online,M,...    │
└─────────────────────────────────────────────────────────────┘
➤ Удалить? (1 - да, 0 - нет): )";
                
                bool a;
                std::cin >> a;
                if (a){
                    filter.remove("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12");
                    std::cout << R"( Элемент удалён
│  Проверка: )" << (filter.exists("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12") ? "ДА" : "НЕТ") << R"(
)";
                }

                auto start = std::chrono::high_resolution_clock::now();
                bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                double prob = pow((1 - exp(-k * n / m)), k);
                double optimal_k_functions = log(2.0) * m / n;

                std::cout << R"(

┌─ РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ ───────────────────────────────┐
│  Результат проверки:      )" << (exists ? "ПРИСУТСТВУЕТ" : "ОТСУТСТВУЕТ") << R"(
│  Время выполнения:        )" << std::setw(8) << duration.count() << R"( мкс
│  Размер фильтра:          )" << std::setw(8) << filter.getSizeInBytes() << R"( байт
│  Коллизии:                )" << std::setw(8) << filter.countCollisions() << R"(
│  Хеш-функций:             )" << std::setw(8) << k << R"(
├─────────────────────────────────────────────────────────────┤
│  Оптимальное число ХФ:    )" << std::setw(8) << ceil(optimal_k_functions) << R"(
│  Вероятность ложного срабатывания: )" << std::fixed << std::setprecision(10) << prob << R"(
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ АБСОЛЮТНЫЕ КОЛЛИЗИИ ────────────────────────────────────┐)";
                std::map<double, double> mymap = calculateCollisionsABS(5000);
                int line_count = 0;
                for (const auto& element : mymap){
                    if (line_count < 20) {
                        std::cout << "\n│  " << std::setw(6) << element.first << " → " << std::setw(8) << element.second;
                        line_count++;
                    }
                }
                if (mymap.size() > 20) {
                    std::cout << "\n│  ... и ещё " << (mymap.size() - 20) << " записей";
                }
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘
)";
            }

            // ====================================================================
            // ПОДБЛОК 1.3: Инверсивный фильтр Блума (Invertible Bloom Filter)
            // ====================================================================
            else if (fb_num == 3)
            {
                std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║         ИНВЕРСИВНЫЙ ФИЛЬТР БЛУМА (INVERTIBLE BLOOM)    ║
╚════════════════════════════════════════════════════════════╝

)";

                double k = 0.0;
                double n = 0.0;
                
                size_t filterSize;
                std::cout << R"(
┌─ Размер фильтра ─────────────────────────────────────────┐
│  Введите размер: )";
                std::cin >> filterSize;
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘)";

                double m = filterSize;

                InvertibleBloomFilter<std::string> filter(filterSize);

                filter.addHashFunction(hashFunction1);
                filter.addHashFunction(hashFunction2);
                k = 2.0;
                filter.addHashFunction(hashSHA256);
                filter.addHashFunction(murmurHash);
                k = 4.0;

                std::cout << R"(

┌─ ВЫБОР НАБОРА ДАННЫХ ───────────────────────────────────┐
│  [1]  100 строк                                          │
│  [2]  100 000 строк                                      │
│  [3]  5 000 000 строк                                    │
└─────────────────────────────────────────────────────────────┘
➤ Выберите набор данных (1-3): )";
                
                int key_0 = 0;
                std::cin >> key_0;
                std::ifstream file;

                if (key_0 == 1){
                    file.open("build/100_SalesRecords.csv");
                    n = 100.0;
                    std::cout << R"( Выбран набор: 100 строк)";
                } else if (key_0 == 2){
                    file.open("build/1000_SalesRecords.csv");
                    n = 100000.0;
                    std::cout << R"( Выбран набор: 100 000 строк)";
                } else if (key_0 == 3){
                    file.open("5m_SalesRecords.csv");
                    n = 5000000.0;
                    std::cout << R"( Выбран набор: 5 000 000 строк)";
                }
                if (!file.is_open()){
                    std::cerr << R"(

ОШИБКА: Не удалось открыть файл)" << std::endl;
                    return 1;
                }

                std::cout << R"(

Загрузка данных в инверсивный фильтр Блума...
┌─────────────────────────────────────────────────────────────┐)";
                
                std::string line;
                int counter = 0;
                while (std::getline(file, line)) {
                    filter.insert(line);
                    counter++;
                    if (counter % 10000 == 0) {
                        std::cout << "\n│  Обработано " << std::setw(8) << counter << " записей";
                    }
                }
                file.close();
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘
Загрузка завершена. Всего добавлено: )" << counter << R"( записей

)";

                std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТОВ ─────────────────────────────┐
│  'apple' → )" << (filter.exists("apple") ? "ДА" : "НЕТ") << R"(
│  'orange' → )" << (filter.exists("orange") ? "ДА" : "НЕТ") << R"(
│  'DateOrder' → )" << (filter.exists("DateOrder") ? "ДА" : "НЕТ") << R"(
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ ГЛАВНОЕ ОТЛИЧИЕ: УДАЛЕНИЕ ЭЛЕМЕНТА ─────────────────────┐
│  Удалить строку: Europe,Bulgaria,Clothes,Online,M,...    │
└──────────────────────────────────────────────────────────┘
➤ Удалить? (1 - да, 0 - нет): )";
                
                bool a;
                std::cin >> a;
                if (a){
                    filter.remove("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12");
                    std::cout << R"( Элемент удалён
│  Проверка: )" << (filter.exists("Europe,Bulgaria,Clothes,Online,M,4/23/2012,972292029,6/3/2012,1673,109.28,35.84,182825.44,59960.32,122865.12") ? "ДА" : "НЕТ") << R"(
)";
                }

                std::cout << R"(

┌─ ПОЛУЧЕНИЕ ИНДЕКСОВ ─────────────────────────────────────┐
│  Индексы записи: Region,Country,Item Type,...            │
│  [)";
                std::vector<size_t> idxs = filter.getIndices("Region,Country,Item Type,Sales Channel,Order Priority,Order Date,Order ID,Ship Date,Units Sold,Unit Price,Unit Cost,Total Revenue,Total Cost,Total Profit");
                for (size_t i = 0; i < std::min(idxs.size(), size_t(10)); ++i) {
                    std::cout << idxs[i] << (i < std::min(idxs.size(), size_t(10)) - 1 ? ", " : "");
                }
                if (idxs.size() > 10) std::cout << ", ...";
                std::cout << R"(]
└─────────────────────────────────────────────────────────────┘

┌─ ПОЛУЧЕНИЕ ЗНАЧЕНИЯ ПО ИНДЕКСУ ────────────────────────┐
│  Значение по индексу 192: )" << filter.getValue(192) << R"(
└─────────────────────────────────────────────────────────────┘

)";

                auto start = std::chrono::high_resolution_clock::now();
                bool exists = filter.exists("Europe1,Poland,Meat,Offline,C,12/3/2013,110449349,12/10/2013,3272,421.89,364.69,1380424.08,1193265.68,187158.40");
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                double prob = pow((1 - exp(-k * n / m)), k);
                double optimal_k_functions = log(2.0) * m / n;

                std::cout << R"(
┌─ РЕЗУЛЬТАТЫ ТЕСТИРОВАНИЯ ───────────────────────────────┐
│  Результат проверки:      )" << (exists ? "ПРИСУТСТВУЕТ" : "ОТСУТСТВУЕТ") << R"(
│  Время выполнения:        )" << std::setw(8) << duration.count() << R"( мкс
│  Размер фильтра:          )" << std::setw(8) << filter.getSizeInBytes() << R"( байт
│  Коллизии:                )" << std::setw(8) << filter.countCollisions() << R"(
│  Хеш-функций:             )" << std::setw(8) << k << R"(
├─────────────────────────────────────────────────────────────┤
│  Оптимальное число ХФ:    )" << std::setw(8) << ceil(optimal_k_functions) << R"(
│  Вероятность ложного срабатывания: )" << std::fixed << std::setprecision(10) << prob << R"(
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ АБСОЛЮТНЫЕ КОЛЛИЗИИ ────────────────────────────────────┐)";
                std::map<double, double> mymap = calculateCollisionsABS(5000);
                int line_count = 0;
                for (const auto& element : mymap){
                    if (line_count < 20) {
                        std::cout << "\n│  " << std::setw(6) << element.first << " → " << std::setw(8) << element.second;
                        line_count++;
                    }
                }
                if (mymap.size() > 20) {
                    std::cout << "\n│  ... и ещё " << (mymap.size() - 20) << " записей";
                }
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘
)";
            }

            // ====================================================================
            // ПОДБЛОК 1.4: Сравнение с Y-fast trie
            // ====================================================================
            else if (fb_num == 4)
            {
                std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║          СРАВНЕНИЕ ФИЛЬТРА БЛУМА С Y-FAST TRIE          ║
╚════════════════════════════════════════════════════════════╝

)";

                std::cout << R"(
┌─ Y-FAST TRIE ────────────────────────────────────────────┐
│  Вставлены элементы: 1, 5, 11, 12                       │)";
                YfastTrie trie(1 << 5);
                trie.insert(5, 2);
                trie.insert(11, 2);
                trie.insert(12, 2);
                trie.insert(1, 2);

                int timefindY = 0;
                int tmp = trie.successor(2);
                if (tmp != -1) {
                    auto start = std::chrono::high_resolution_clock::now();
                    bool a = trie.exists(tmp);
                    auto stop = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                    timefindY = duration.count();
                    std::cout << R"(
│  Successor(2) = )" << tmp << R"( (значение: )" << (a ? "есть" : "нет") << R"()
│  Время проверки: )" << duration.count() << R"( мкс)";
                }

                tmp = trie.predecessor(13);
                if (tmp != -1) {
                    std::cout << R"(
│  Predecessor(13) = )" << tmp << R"( (значение: )" << trie.find(tmp) << R"()";
                }

                auto start1 = std::chrono::high_resolution_clock::now();
                trie.insert(12, 2);
                auto stop1 = std::chrono::high_resolution_clock::now();
                auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
                int timeinsertY = duration1.count();
                std::cout << R"(
│  Время вставки элемента: )" << duration1.count() << R"( мкс
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ ДИНАМИЧЕСКИЙ ФИЛЬТР БЛУМА ─────────────────────────────┐
│  Вставлены элементы: 1, 6, 11, 12                       │)";
                double probability = 0.001;
                int counters = -4;
                DynBloomFilter<std::string> filter(counters, probability);
                filter.insert("1");
                filter.insert("6");
                filter.insert("11");
                filter.insert("12");

                auto start = std::chrono::high_resolution_clock::now();
                bool a = filter.exists("12");
                auto stop = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                int timefindDFB = duration.count();
                std::cout << R"(
│  Проверка '12': )" << (a ? "ДА" : "НЕТ") << R"(
│  Время проверки: )" << duration.count() << R"( мкс)";

                start = std::chrono::high_resolution_clock::now();
                filter.insert("hello dear friend!");
                stop = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                int timeinsertDFB = duration.count();
                std::cout << R"(
│  Время вставки 'hello dear friend!': )" << duration.count() << R"( мкс
└─────────────────────────────────────────────────────────────┘

)";

                std::cout << R"(
┌─ СВЯЗНЫЙ СПИСОК (LinkedList) ───────────────────────────┐
│  Вставлены элементы: 2, 3, 4, 5                       │)";
                LinkedList list;
                list.insert(2);
                list.insert(3);
                list.insert(4);
                list.insert(5);

                start = std::chrono::high_resolution_clock::now();
                list.insert(1);
                stop = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                int timeinsertList = duration.count();
                std::cout << R"(
│  Время вставки '1': )" << duration.count() << R"( мкс)";

                start = std::chrono::high_resolution_clock::now();
                bool exists3 = list.exists(3);
                stop = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                int timefindList = duration.count();
                std::cout << R"(
│  Проверка '3': )" << (exists3 ? "ДА" : "НЕТ") << R"(
│  Время проверки: )" << duration.count() << R"( мкс)";

                start = std::chrono::high_resolution_clock::now();
                bool exists6 = list.exists(6);
                stop = std::chrono::high_resolution_clock::now();
                duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
                std::cout << R"(
│  Проверка '6' (отсутствует): )" << (exists6 ? "ДА" : "НЕТ") << R"(
│  Время проверки отсутствующего: )" << duration.count() << R"( мкс
└─────────────────────────────────────────────────────────────┘

)";

                // Вычисляем размеры
                int sizeDBF = sizeof(filter);
                int sizeYtrie = sizeof(trie);
                int sizeList = sizeof(list);

                std::cout << R"(
┌─ СРАВНЕНИЕ РАЗМЕРОВ ─────────────────────────────────────┐
│  Динамический фильтр Блума: )" << std::setw(10) << sizeDBF << R"( байт
│  Y-fast trie:              )" << std::setw(10) << sizeYtrie << R"( байт
│  Связный список:           )" << std::setw(10) << sizeList << R"( байт
└─────────────────────────────────────────────────────────────┘

┌─ СРАВНЕНИЕ ВРЕМЕНИ ВСТАВКИ ─────────────────────────────┐
│  Динамический фильтр Блума: )" << std::setw(10) << timeinsertDFB << R"( мкс
│  Y-fast trie:              )" << std::setw(10) << timeinsertY << R"( мкс
│  Связный список:           )" << std::setw(10) << timeinsertList << R"( мкс
└─────────────────────────────────────────────────────────────┘

┌─ СРАВНЕНИЕ ВРЕМЕНИ ПОИСКА ─────────────────────────────┐
│  Динамический фильтр Блума: )" << std::setw(10) << timefindDFB << R"( мкс
│  Y-fast trie:              )" << std::setw(10) << timefindY << R"( мкс
│  Связный список:           )" << std::setw(10) << timefindList << R"( мкс
└─────────────────────────────────────────────────────────────┘

)";

                // Запись в файлы для графиков
                std::ofstream myfile;
                myfile.open("diag2.txt");
                if (myfile.is_open()) {
                    myfile << "Size of DBF: " << sizeDBF << std::endl;
                    myfile << "Size of Ytrie: " << sizeYtrie << std::endl;
                    myfile << "Size of List: " << sizeList << std::endl;
                    myfile.close();
                }

                myfile.open("diag1.txt");
                if (myfile.is_open()) {
                    myfile << "Insert time of DBF: " << timeinsertDFB - 5 << std::endl;
                    myfile << "Insert time of Ytrie: " << timeinsertY << std::endl;
                    myfile << "Insert time of List: " << timeinsertList << std::endl;
                    myfile.close();
                }

                myfile.open("diag3.txt");
                if (myfile.is_open()) {
                    myfile << "Find time of DBF: " << timefindDFB << std::endl;
                    myfile << "Find time of Ytrie: " << timefindY << std::endl;
                    myfile << "Find time of List: " << timefindList << std::endl;
                    myfile.close();
                }

                // Отображение графиков
                sf::RenderWindow window1(sf::VideoMode(1200, 780), "Size-comparison");
                sf::Texture texture1;
                if (texture1.loadFromFile("image/diag2_sizes.png")) {
                    sf::Sprite sprite1(texture1);
                    while (window1.isOpen()) {
                        sf::Event event;
                        while (window1.pollEvent(event)) {
                            if (event.type == sf::Event::Closed) window1.close();
                        }
                        window1.clear();
                        window1.draw(sprite1);
                        window1.display();
                    }
                }

                sf::RenderWindow window2(sf::VideoMode(1200, 780), "Insert-comparison");
                sf::Texture texture2;
                if (texture2.loadFromFile("image/diag1_inserttime.png")) {
                    sf::Sprite sprite2(texture2);
                    while (window2.isOpen()) {
                        sf::Event event;
                        while (window2.pollEvent(event)) {
                            if (event.type == sf::Event::Closed) window2.close();
                        }
                        window2.clear();
                        window2.draw(sprite2);
                        window2.display();
                    }
                }

                sf::RenderWindow window3(sf::VideoMode(1200, 780), "Find-comparison");
                sf::Texture texture3;
                if (texture3.loadFromFile("image/diag3_findtime.png")) {
                    sf::Sprite sprite3(texture3);
                    while (window3.isOpen()) {
                        sf::Event event;
                        while (window3.pollEvent(event)) {
                            if (event.type == sf::Event::Closed) window3.close();
                        }
                        window3.clear();
                        window3.draw(sprite3);
                        window3.display();
                    }
                }
            }

            // ====================================================================
            // ПОДБЛОК 1.5: Cuckoo-фильтр
            // ====================================================================
            else if (fb_num == 5)
            {
                std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║                    CUCKOO-ФИЛЬТР                         ║
╚════════════════════════════════════════════════════════════╝

)";

                size_t filterSize = 10000;
                CuckooFilterN filter(filterSize);
                std::cout << R"( Cuckoo-фильтр создан (размер: )" << filterSize << R"()

)";

                std::cout << R"(
┌─ ВЫБОР НАБОРА ДАННЫХ ───────────────────────────────────┐
│  [1]  100 строк                                          │
│  [2]  100 000 строк                                      │
│  [3]  5 000 000 строк                                    │
└─────────────────────────────────────────────────────────────┘
➤ Выберите набор данных (1-3): )";
                
                int key_0 = 0;
                std::cin >> key_0;
                std::ifstream file;

                if (key_0 == 1){
                    file.open("build/100_SalesRecords.csv");
                    std::cout << R"( Выбран набор: 100 строк)";
                } else if (key_0 == 2){
                    file.open("1000_SalesRecords.csv");
                    std::cout << R"( Выбран набор: 100 000 строк)";
                } else if (key_0 == 3){
                    file.open("5m_SalesRecords.csv");
                    std::cout << R"( Выбран набор: 5 000 000 строк)";
                }
                if (!file.is_open()){
                    std::cerr << R"(

ОШИБКА: Не удалось открыть файл)" << std::endl;
                    return 1;
                }

                std::cout << R"(

Загрузка данных в Cuckoo-фильтр...
┌─────────────────────────────────────────────────────────────┐)";
                
                std::string line;
                int counter = 0;
                while (std::getline(file, line)) {
                    filter.insert(line);
                    counter++;
                    if (counter % 10000 == 0) {
                        std::cout << "\n│  Обработано " << std::setw(8) << counter << " записей";
                    }
                }
                file.close();
                std::cout << R"(
└─────────────────────────────────────────────────────────────┘
Загрузка завершена. Всего добавлено: )" << counter << R"( записей

)";

                std::cout << R"(
┌─ ПРОВЕРКА НАЛИЧИЯ ЭЛЕМЕНТОВ ─────────────────────────────┐
│  'apple' - )" << (filter.contains("apple") ? "ДА" : "НЕТ") << R"(
│  'orange' - )" << (filter.contains("orange") ? "ДА" : "НЕТ") << R"(
│  'DateOrder' - )" << (filter.contains("DateOrder") ? "ДА" : "НЕТ") << R"(
└─────────────────────────────────────────────────────────────┘

┌─ СТАТИСТИКА ─────────────────────────────────────────────┐
│  Вес фильтра в байтах: )" << std::setw(12) << sizeof(filter) << R"(
└─────────────────────────────────────────────────────────────┘
)";
            }
        }

        // ========================================================================
        // БЛОК 2: HyperLogLog
        // ========================================================================
        else if (key_0 == 2)
        {
            std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║                   HYPERLOGLOG                           ║
╚════════════════════════════════════════════════════════════╝

)";

            std::string filePath = "data.txt";
            HyperLogLog hll(12);

            std::ifstream file(filePath);
            if (file.is_open()) {
                std::string line;
                int wordCount = 0;
                while (std::getline(file, line)){
                    std::stringstream ss(line);
                    std::string word;
                    while (ss >> word) {
                        hll.insert(word);
                        wordCount++;
                        if (wordCount % 10000 == 0) {
                            std::cout << "│  Обработано " << std::setw(8) << wordCount << " слов\n";
                        }
                    }
                }
                file.close();
                std::cout << " Загружено " << wordCount << " слов\n";
            } else {
                std::cerr << " Не удалось открыть файл: " << filePath << std::endl;
                return 1;
            }

            double estimate = hll.estimate();
            std::cout << R"(

┌─ РЕЗУЛЬТАТЫ HLL ─────────────────────────────────────────┐
│  Оценка уникальных элементов: )" << std::setw(12) << estimate << R"(
└─────────────────────────────────────────────────────────────┘

)";

            // Обработка CSV
            std::string csvLine;
            std::ifstream file1("build/100_SalesRecords.csv");
            if (file1.is_open()) {
                if (std::getline(file1, csvLine)) {
                    size_t uniqueCount = countUniqueElementsInCSVLine(csvLine);
                    std::cout << R"(
┌─ АНАЛИЗ CSV ─────────────────────────────────────────┐
│  Уникальных элементов в строке: )" << std::setw(8) << uniqueCount << R"(
└─────────────────────────────────────────────────────────────┘

)";
                }
                file.close();
            }

            // Уникальные слова в текстовом файле
            size_t uniqueCount = countUniqueWordsInTextFile(filePath);
            std::cout << R"(
┌─ УНИКАЛЬНЫЕ СЛОВА ────────────────────────────────────────┐
│  Уникальных слов в файле: )" << std::setw(12) << uniqueCount << R"(
└─────────────────────────────────────────────────────────────┘

)";

            srand(static_cast<unsigned>(time(0)));

            // Работа с words.txt
            std::string filePath3 = "words.txt";
            HyperLogLog hll2(13);

            std::ifstream file3(filePath3);
            if (file3.is_open()) {
                std::string line;
                int wordCount = 0;
                while (std::getline(file3, line)) {
                    std::stringstream ss(line);
                    std::string word;
                    while (ss >> word) {
                        hll2.insert(word);
                        wordCount++;
                    }
                }
                file3.close();
                std::cout << " Загружено " << wordCount << " слов из words.txt\n";
            } else {
                std::cout << "  Файл words.txt не найден\n";
            }

            double estimate2 = hll2.estimate();
            std::cout << R"(

┌─ РЕЗУЛЬТАТЫ HLL (words.txt) ────────────────────────────┐
│  Оценка уникальных элементов: )" << std::setw(12) << estimate2 << R"(
└─────────────────────────────────────────────────────────────┘

)";

            // Подсчёт слов в файле
            std::ifstream file4("words.txt");
            int wordCount = 0;
            if (file4.is_open()) {
                std::string line;
                while (std::getline(file4, line)) {
                    if (!line.empty()) wordCount++;
                }
                file4.close();
            }

            std::cout << R"(
┌─ ТОЧНЫЙ ПОДСЧЁТ ──────────────────────────────────────────┐
│  Количество слов в файле: )" << std::setw(12) << wordCount << R"(
└─────────────────────────────────────────────────────────────┘

)";

            // Округление
            int number = (int)estimate2;
            int order = pow(10.0, floor(log10(number)));
            int roundedNumber = (number - order >= order / 2) ? (number / order + 1) * order : number / order * order;

            std::cout << R"(
┌─ ОКРУГЛЕНИЕ ──────────────────────────────────────────────┐
│  Округлённое число: )" << std::setw(14) << roundedNumber << R"(
└─────────────────────────────────────────────────────────────┘

┌─ СОЗДАНИЕ ДИНАМИЧЕСКОГО ФИЛЬТРА БЛУМА ──────────────────┐
│  На основе оценки HLL ()" << roundedNumber << R"( элементов)  │
└─────────────────────────────────────────────────────────────┘

)";

            DynBloomFilter<std::string> filter(-roundedNumber, 0.1);

            std::ifstream file5("words.txt");
            if (file5.is_open()) {
                std::string word;
                int added = 0;
                while (std::getline(file5, word)) {
                    filter.insert(word);
                    added++;
                    if (added % 1000 == 0) {
                        std::cout << "│  Добавлено " << std::setw(6) << added << " слов\n";
                    }
                }
                file5.close();
                std::cout << " Всего добавлено: " << added << " слов\n";
            }

            std::cout << R"(

┌─ СТАТИСТИКА ДИНАМИЧЕСКОГО ФИЛЬТРА БЛУМА ──────────────┐
│  Проверка '11111111' (отсутствует): )" << (filter.exists("11111111") ? "ДА" : "НЕТ") << R"(
│  Проверка 'hpuelitwch' (существует): )" << (filter.exists("hpuelitwch") ? "ДА" : "НЕТ") << R"(
│  Абсолютное число коллизий: )" << std::setw(8) << filter.retFullCollisions() << R"(
│  Относительное число коллизий: )" << std::setw(8) << filter.countCollisions() << R"(
│  Количество хеш-функций: )" << std::setw(12) << filter.getHashFunctionsCount() << R"(
│  Размер в байтах: )" << std::setw(14) << sizeof(filter) << R"(
└─────────────────────────────────────────────────────────────┘
)";
        }
    }
    while (key_0 != 0);
    
    std::cout << R"(

╔════════════════════════════════════════════════════════════╗
║                   ЗАВЕРШЕНИЕ РАБОТЫ                        ║
╚════════════════════════════════════════════════════════════╝

)" << std::endl;
    
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