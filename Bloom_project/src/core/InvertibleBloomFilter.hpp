#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

/**
 * Класс InvertibleBloomFilter - Инверсивный фильтр Блума
 * 
 * Реализация вероятностной структуры данных "Инверсивный фильтр Блума"
 * (Invertible Bloom Filter). Отличается от классического фильтра Блума
 * тем, что позволяет не только проверять наличие элемента и удалять его,
 * но и ВОССТАНАВЛИВАТЬ (инвертировать) удалённые элементы.
 * 
 * @tparam T - тип хранимых элементов (обычно std::string)
 * 
 * Основные возможности:
 * - Вставка элемента (insert)
 * - Проверка наличия (exists) - возможны ложноположительные срабатывания
 * - Удаление элемента (remove)
 * - Восстановление элемента по индексу (getValue)
 * - Получение всех хранящихся элементов (getAllValues)
 * 
 * Название "инверсивный" означает, что из фильтра можно восстановить
 * исходные элементы, даже после их удаления (при определённых условиях)
 */
template<typename T>
class InvertibleBloomFilter {
private:
    // Счётчики для каждой ячейки фильтра (сколько элементов указывают на эту ячейку)
    std::vector<int> counters;
    
    // Вектор хеш-функций, используемых для распределения элементов
    std::vector<std::function<size_t(const T&)>> hashFunctions;
    
    // Хранилище значений (индекс ячейки -> элемент)
    // Используется для восстановления элементов по индексу
    std::unordered_map<size_t, T> valueMap;
    
    // Для отслеживания индексов элементов (элемент -> вектор индексов)
    // Используется для расчёта абсолютных коллизий
    std::unordered_map<T, std::vector<size_t>> elementIndices;
    
    // Карта коллизий (индекс -> сколько раз был установлен)
    // Для подсчёта относительных коллизий
    std::unordered_map<size_t, size_t> collisionMap;

public:
    /**
     * Конструктор инверсивного фильтра Блума
     * 
     * @param size - размер фильтра (количество ячеек-счётчиков)
     * 
     * Примечание: В отличие от классического фильтра Блума, здесь используется
     * массив целочисленных счётчиков вместо битового массива. Это позволяет
     * отслеживать, сколько элементов указывают на каждую ячейку.
     */
    InvertibleBloomFilter(size_t size) : counters(size, 0) {}

    /**
     * Добавление хеш-функции в фильтр
     * 
     * @param hashFunction - хеш-функция, принимающая элемент T и возвращающая size_t
     * 
     * Примечание: Для хорошего распределения рекомендуется использовать
     * независимые хеш-функции (кольцевые, MurmurHash, SHA256, MD5)
     */
    void addHashFunction(const std::function<size_t(const T&)>& hashFunction) {
        hashFunctions.push_back(hashFunction);
    }

    /**
     * Вставка элемента в инверсивный фильтр Блума
     * 
     * Алгоритм:
     * 1. Для каждой хеш-функции вычисляется индекс ячейки
     * 2. Счётчик по этому индексу увеличивается на 1
     * 3. Если счётчик становится равным 1, в valueMap сохраняется элемент
     * 4. Сохраняются индексы для последующего анализа коллизий
     * 
     * @param item - элемент для вставки
     * 
     * Примечание: В отличие от классического фильтра Блума, здесь не теряется
     * информация о количестве элементов, указывающих на каждую ячейку.
     * Это позволяет впоследствии удалять элементы.
     */
    void insert(const T& item) {
        std::vector<size_t> indices;
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            counters[index]++;                    // Увеличиваем счётчик для данной ячейки
            indices.push_back(index);              // Сохраняем индекс для последующего анализа
            collisionMap[index]++;                 // Увеличиваем счётчик коллизий
            
            // Если счётчик стал равен 1, значит, это первый элемент, указывающий на ячейку
            // Сохраняем его в valueMap для возможности восстановления
            if (counters[index] == 1) {
                valueMap[index] = item;
            }
        }
        elementIndices[item] = indices;            // Сохраняем индексы для текущего элемента
    }

    /**
     * Проверка наличия элемента в фильтре
     * 
     * Алгоритм:
     * 1. Для каждой хеш-функции вычисляется индекс ячейки
     * 2. Если хотя бы один счётчик равен 0 → элемент точно отсутствует
     * 3. Если все счётчики > 0 → элемент возможно присутствует
     * 
     * @param item - проверяемый элемент
     * @return true - элемент возможно присутствует (может быть ложноположительное срабатывание)
     * @return false - элемент точно отсутствует
     */
    bool exists(const T& item) {
        for (const auto& hashFunction : hashFunctions) {
            size_t index = hashFunction(item) % counters.size();
            if (counters[index] == 0) {
                return false; // Если хоть один счётчик равен 0, элемента точно нет
            }
        }
        return true; // Все счётчики больше 0, элемент возможно присутствует
    }

    /**
     * Удаление элемента из фильтра
     * 
     * Алгоритм:
     * 1. Получить индексы ячеек для данного элемента
     * 2. Для каждого индекса уменьшить счётчик на 1
     * 3. Если счётчик стал равен 0, удалить элемент из valueMap
     * 
     * @param item - элемент для удаления
     * 
     * Примечание: В классическом фильтре Блума удаление невозможно.
     * Инверсивный фильтр позволяет удалять элементы благодаря использованию
     * счётчиков вместо битов.
     */
    void remove(const T& item) {
        std::vector<size_t> indices = getIndices(item);
        for (const auto& index : indices) {
            if (counters[index] > 0) {
                counters[index]--; // Уменьшаем счётчик
                // Если счётчик стал равен 0, удаляем элемент из valueMap
                if (counters[index] == 0) {
                    valueMap.erase(index);
                }
            }
        }
    }

    /**
     * Получение индексов ячеек, связанных с элементом
     * 
     * @param item - элемент
     * @return std::vector<size_t> - вектор индексов, где счётчики не равны 0
     */
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

    /**
     * Восстановление элемента по индексу ячейки
     * 
     * @param index - индекс ячейки
     * @return T - элемент, хранящийся в этой ячейке (или пустой элемент, если ячейка пуста)
     * 
     * Примечание: Это ключевая особенность инверсивного фильтра Блума.
     * Зная индекс, можно восстановить исходный элемент, который на него указывает.
     * Это позволяет "инвертировать" (восстанавливать) удалённые элементы.
     */
    T getValue(size_t index) const {
        auto it = valueMap.find(index);
        if (it != valueMap.end()) {
            return it->second;
        }
        return T(); // Возвращаем пустой элемент, если индекс не найден
    }

    /**
     * Вычисление размера фильтра в байтах
     * 
     * @return size_t - размер в байтах
     */
    size_t getSizeInBytes() const {
        size_t filterSize = sizeof(InvertibleBloomFilter);           // Размер объекта фильтра
        size_t counterArraySize = counters.size() / 8;               // Размер массива счётчиков в байтах
        return filterSize + counterArraySize;                        // Общий размер
    }

    /**
     * Получение всех элементов, хранящихся в фильтре
     * 
     * @return std::vector<T> - вектор всех уникальных элементов
     * 
     * Примечание: В классическом фильтре Блума это невозможно,
     * так как информация об элементах теряется. Инверсивный фильтр
     * сохраняет элементы в valueMap, что позволяет их восстановить.
     */
    std::vector<T> getAllValues() const {
        std::vector<T> values;
        for (const auto& entry : valueMap) {
            values.push_back(entry.second);
        }
        return values;
    }

    /**
     * Подсчёт относительных коллизий
     * Коллизия - ситуация, когда несколько элементов устанавливают один и тот же индекс
     * 
     * @return size_t - общее количество коллизий
     */
    size_t countCollisions() const {
        size_t totalCollisions = 0;
        for (const auto& entry : collisionMap) {
            if (entry.second > 1) {
                totalCollisions += entry.second - 1; // Подсчёт коллизий для каждого индекса
            }
        }
        return totalCollisions;
    }

    /**
     * Подсчёт абсолютных коллизий (полных совпадений)
     * Абсолютная коллизия - когда два элемента имеют полностью совпадающие наборы индексов
     * Это более редкое и серьёзное явление
     * 
     * @return size_t - количество абсолютных коллизий
     */
    size_t retFullCollisions() const
    {
        size_t totalCollisions = 0;
        // Дополнительная логика для подсчёта коллизий, когда все индексы совпадают
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
                        totalCollisions++; // Увеличить счётчик коллизий, если все индексы совпадают
                    }
                }
            }
        }
        return totalCollisions;
    }

    /**
     * Получение количества хеш-функций
     * 
     * @return size_t - количество зарегистрированных хеш-функций
     */
    size_t getHashFunctionsCount() const 
    {
        return hashFunctions.size();
    }

    /**
     * Очистка фильтра Блума
     * Сбрасывает все счётчики в 0 и очищает все хранилища
     */
    void clear() {
        std::fill(counters.begin(), counters.end(), 0); // Очистка счётчиков
        valueMap.clear();                                // Очистка хранилища значений
        elementIndices.clear();                          // Очистка информации об индексах элементов
    }
};