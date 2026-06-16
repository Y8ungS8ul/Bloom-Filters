# Bloom-Filters - Вероятностные структуры данных

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![SFML](https://img.shields.io/badge/SFML-2.6-orange.svg)
![CMake](https://img.shields.io/badge/CMake-3.14+-green.svg)
![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux%20%7C%20Windows-lightgrey.svg)

## О проекте

Данный проект представляет собой курсовую работу по дисциплине 
"Структуры и алгоритмы обработки данных". В рамках работы были реализованы 
и исследованы различные вероятностные структуры данных, их производительность 
и области применения.

### Цель работы

Изучение и сравнительный анализ вероятностных структур данных, их эффективности
при обработке больших объемов данных, а также визуализация принципов их работы.

## Реализованные структуры данных

| Структура | Описание | Сложность | Особенности |
|-----------|----------|-----------|-------------|
| **Классический фильтр Блума** | Базовый вероятностный фильтр | O(k) | Экономия памяти, возможны ложноположительные срабатывания |
| **Фильтр Блума с подсчетом** | Counting Bloom Filter | O(k) | Поддержка удаления элементов |
| **Инверсивный фильтр Блума** | Invertible Bloom Filter | O(k) | Возможность восстановления элементов |
| **Cuckoo-фильтр** | Cuckoo Filter | O(1) | Высокая производительность, поддержка удаления |
| **Динамический фильтр Блума** | Dynamic Bloom Filter | O(k) | Автоматический подбор параметров |
| **HyperLogLog** | Cardinality estimation | O(n) | Оценка количества уникальных элементов |
| **Y-fast trie** | Целочисленное дерево | O(log log U) | Быстрый поиск целочисленных ключей |

## Интерфейс и визуализация

- **Графический интерфейс** - реализован с использованием библиотеки **SFML 2.6**
- **Интерактивное меню** - навигация с клавиатуры (стрелки, Enter, ESC)

### Управление в программе

| Клавиша | Действие |
|---------|----------|
| `↑` / `↓` | Навигация по меню |
| `Enter` | Выбор пункта |
| `ESC` | Выход / возврат назад |
| `1-9` | Выбор режима в подменю |
| `A`, `B`, `Q` | Добавление тестовых элементов |

## Установка и сборка

### Требования

- Компилятор с поддержкой C++17
- CMake 3.14
- SFML 2.6
- SQLite3

### macOS

```bash
# Установка зависимостей
brew install cmake sfml@2 sqlite3

# Клонирование репозитория
git clone https://github.com/Y8ungS8ul/Bloom-Filters.git
cd Bloom-Filters/Bloom_project

# Сборка проекта
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Запуск
./build/Bloom_project

# Установка зависимостей
sudo apt update
sudo apt install cmake libsfml-dev libsqlite3-dev

# Сборка проекта
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# Запуск
./Bloom_project

mkdir build && cd build
cmake .. -G "Visual Studio 16 2019"
# Открыть сгенерированный Bloom_project.sln


![картинка](https://i.imgur.com/tObEz5f.png)