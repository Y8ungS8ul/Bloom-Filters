#include "SkipList.hpp"
#include <cstring>

/**
 * @brief Генерация случайного уровня
 * 
 * @details Вероятность:
 *          - Уровень 0: 1 - P
 *          - Уровень 1: P * (1 - P)
 *          - Уровень 2: P^2 * (1 - P)
 *          - и т.д.
 */
int SkipList::randomLevel() {
    float r = static_cast<float>(rand()) / RAND_MAX;
    int lvl = 0;
    while (r < P && lvl < MAXLVL) {
        lvl++;
        r = static_cast<float>(rand()) / RAND_MAX;
    }
    return lvl;
}

/**
 * @brief Создание нового узла
 */
SkipNode* SkipList::createNode(int& value, int level) {
    return new SkipNode(level, value);
}

/**
 * @brief Конструктор списка
 */
SkipList::SkipList(int max_level, float p) {
    MAXLVL = max_level;
    P = p;
    level = 0;
    int dummy_value = -1;
    header = new SkipNode(MAXLVL, dummy_value);
    
    // Инициализация генератора случайных чисел
    srand(static_cast<unsigned>(time(nullptr)));
}

/**
 * @brief Деструктор списка
 */
SkipList::~SkipList() {
    SkipNode* current = header;
    while (current != nullptr) {
        SkipNode* next = current->forward[0];
        delete current;
        current = next;
    }
}

/**
 * @brief Вставка элемента в список
 */
void SkipList::insertElement(int& value) {
    SkipNode* current = header;
    
    // Массив для хранения узлов, которые нужно обновить на каждом уровне
    SkipNode** update = new SkipNode*[MAXLVL + 1];
    memset(update, 0, sizeof(SkipNode*) * (MAXLVL + 1));
    
    // Поиск позиции для вставки на каждом уровне
    for (int i = level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    // Если элемент не существует, вставляем его
    if (current == nullptr || current->value != value) {
        int rlevel = randomLevel();
        
        // Если новый уровень выше текущего максимума, обновляем
        if (rlevel > level) {
            for (int i = level + 1; i <= rlevel; i++) {
                update[i] = header;
            }
            level = rlevel;
        }
        
        // Создание и вставка нового узла
        SkipNode* newNode = createNode(value, rlevel);
        for (int i = 0; i <= rlevel; i++) {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
    }
    
    delete[] update;
}

/**
 * @brief Вывод списка на экран
 */
void SkipList::displayList() {
    std::cout << "\n======= Список с пропусками (Skip List) =======\n";
    std::cout << "Максимальный уровень: " << MAXLVL << "\n";
    std::cout << "Текущий уровень: " << level << "\n";
    std::cout << "Вероятность P: " << P << "\n\n";
    
    for (int i = 0; i <= level; i++) {
        SkipNode* node = header->forward[i];
        std::cout << "Уровень " << i << ": ";
        while (node != nullptr) {
            std::cout << node->value << " ";
            node = node->forward[i];
        }
        std::cout << "\n";
    }
    std::cout << "===============================================\n";
}

/**
 * @brief Поиск элемента в списке
 */
bool SkipList::search(int value) {
    SkipNode* current = header;
    
    // Проход сверху вниз
    for (int i = level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
    }
    
    current = current->forward[0];
    
    // Проверка, найден ли элемент
    if (current != nullptr && current->value == value) {
        return true;
    }
    return false;
}

/**
 * @brief Удаление элемента из списка
 */
bool SkipList::remove(int value) {
    SkipNode* current = header;
    SkipNode** update = new SkipNode*[MAXLVL + 1];
    memset(update, 0, sizeof(SkipNode*) * (MAXLVL + 1));
    
    // Поиск удаляемого элемента
    for (int i = level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->value < value) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    current = current->forward[0];
    
    // Если элемент найден, удаляем его
    if (current != nullptr && current->value == value) {
        for (int i = 0; i <= level; i++) {
            if (update[i]->forward[i] != current) {
                break;
            }
            update[i]->forward[i] = current->forward[i];
        }
        
        // Обновляем текущий максимальный уровень
        while (level > 0 && header->forward[level] == nullptr) {
            level--;
        }
        
        delete current;
        delete[] update;
        return true;
    }
    
    delete[] update;
    return false;
}