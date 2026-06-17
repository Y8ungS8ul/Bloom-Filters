#include <iostream>
#include <unordered_map>
#include <vector>
#include <map>
#include <iterator>

/**
 * @brief Структура узла X-Fast Trie
 * 
 * Представляет узел в X-Fast Trie дереве. Узлы могут быть двух типов:
 * - Внутренние узлы (level < w) - содержат только указатели на потомков
 * - Листовые узлы (level == w) - содержат ключ и связи для двусвязного списка
 */
struct Node 
{
    int level;      ///< Уровень узла в дереве (0 - корень, w - лист)
    int key;        ///< Ключ (значим только для листовых узлов)
    
    Node* left;     ///< Указатель на левого потомка или предыдущий элемент в списке
    Node* right;    ///< Указатель на правого потомка или следующий элемент в списке

    Node() 
    {
        level = -1;
        left = nullptr;
        right = nullptr;
    }
};

/**
 * @brief X-Fast Trie (X-быстрое дерево)
 * 
 * Реализация вероятностной структуры данных для целочисленных ключей.
 * 
 * @details Особенности:
 * - Поддерживает операции вставки, поиска, поиска следующего/предыдущего элемента
 * - Использует хеш-таблицы для быстрого доступа к узлам на каждом уровне
 * - Время работы: O(log w) для основных операций, где w - битовая глубина
 * 
 * @note Требует знания максимального значения ключа U для инициализации
 * @warning Ключи должны быть неотрицательными целыми числами
 */
class XfastTrie 
{
    /**
     * @brief Битовая глубина дерева
     * 
     * Определяет максимальное количество уровней в дереве.
     * Вычисляется как количество бит, необходимых для представления числа U
     */
    int w;
    
    /**
     * @brief Хеш-таблицы для каждого уровня дерева
     * 
     * hash_table[level] - отображение префикса на узел на данном уровне
     * Размер вектора: w+1 (уровни от 0 до w)
     */
    std::vector<std::unordered_map<int, Node*>> hash_table;

    /**
     * @brief Вычисляет количество бит, необходимых для представления числа x
     * 
     * @param x Целое число
     * @return Количество бит (минимальная степень двойки, достаточная для x)
     * 
     * @example getDigitCount(5) вернет 3, так как 5 = 101b (3 бита)
     */
    int getDigitCount(int x) 
    {
        int count = 0;
        for (; x > 0; x >>= 1, ++count);
        return count;
    }

    /**
     * @brief Вычисляет ключ левого потомка
     * @param x Ключ родительского узла
     * @return Ключ левого потомка (добавляет 0 в конец бинарного представления)
     */
    int leftChild(int x) 
    {
        return (x << 1);
    }

    /**
     * @brief Вычисляет ключ правого потомка
     * @param x Ключ родительского узла
     * @return Ключ правого потомка (добавляет 1 в конец бинарного представления)
     */
    int rightChild(int x) {
        return ((x << 1) | 1);
    }

    /**
     * @brief Находит самый левый лист в поддереве
     * 
     * Используется для обновления указателей на потомков при вставке
     * 
     * @param parent Узел, с которого начинается поиск
     * @return Указатель на самый левый листовой узел
     */
    Node* getLeftmostLeaf(Node* parent) {
        while (parent->level != w) {
            if (parent->left != nullptr)
                parent = parent->left;
            else
                parent = parent->right;
        }
        return parent;
    }

    /**
     * @brief Находит самый правый лист в поддереве
     * 
     * @param parent Узел, с которого начинается поиск
     * @return Указатель на самый правый листовой узел
     */
    Node* getRightmostLeaf(Node* parent) {
        while (parent->level != w) {
            if (parent->right != nullptr)
                parent = parent->right;
            else
                parent = parent->left;
        }
        return parent;
    }

public:
    XfastTrie() {}

    /**
     * @brief Конструктор X-Fast Trie
     * 
     * @param U Максимальное значение ключа (верхняя граница универсума)
     * 
     * @details Инициализирует:
     * - Вычисляет битовую глубину w = ceil(log2(U))
     * - Создает хеш-таблицы для каждого уровня
     * - Создает корневой узел на уровне 0
     */
    XfastTrie(int U) 
    {
        w = getDigitCount(U);
        hash_table.assign(w + 1, std::unordered_map<int, Node*>());
        
        Node* root = new Node();
        root->level = 0;
        hash_table[0][0] = root;
    }

    /**
     * @brief Поиск узла по ключу
     * 
     * @param k Искомый ключ
     * @return Указатель на узел, если найден, иначе nullptr
     * 
     * @note Сложность O(1) - прямой доступ к хеш-таблице уровня w
     */
    Node* find(int k) 
    {
        if (hash_table[w].find(k) == hash_table[w].end())
            return nullptr;
        return hash_table[w][k];
    }

    /**
     * @brief Поиск следующего (наименьшего) элемента, большего или равного k
     * 
     * @param k Ключ, для которого ищется следующий элемент
     * @return Указатель на узел с ключом >= k, или nullptr если такого нет
     * 
     * @details Использует бинарный поиск по уровням, чтобы найти наибольший общий префикс
     */
    Node* successor(int k) {
        int low = 0,
            high = w + 1,
            mid, prefix;

        Node* tmp = nullptr;
        
        // Бинарный поиск по уровням
        while (high - low > 1) 
        {
            mid = (low + high) >> 1;
            prefix = k >> (w - mid);
            
            if (hash_table[mid].find(prefix) == hash_table[mid].end())
                high = mid;
            else 
            {
                low = mid;
                tmp = hash_table[mid][prefix];
            }
        }

        if (tmp == nullptr || tmp->level == 0)
            return nullptr;

        if (tmp->level == w)
            return tmp;

        // Спуск к листу
        if ((k >> (w - tmp->level - 1)) & 1)
            tmp = tmp->right;
        else
            tmp = tmp->left;

        if (tmp->key < k) {
            return tmp->right;
        }
        return tmp;
    }

    /**
     * @brief Поиск предыдущего (наибольшего) элемента, меньшего или равного k
     * 
     * @param k Ключ, для которого ищется предыдущий элемент
     * @return Указатель на узел с ключом <= k, или nullptr если такого нет
     * 
     * @note Аналогичен successor, но с обратной логикой на последнем шаге
     */
    Node* predecessor(int k) {
        int low = 0,
            high = w + 1,
            mid, prefix;

        Node* tmp = nullptr;
        while (high - low > 1) {
            mid = (low + high) >> 1;
            prefix = k >> (w - mid);
            if (hash_table[mid].find(prefix) == hash_table[mid].end())
                high = mid;
            else {
                low = mid;
                tmp = hash_table[mid][prefix];
            }
        }

        if (tmp == nullptr || tmp->level == 0)
            return nullptr;

        if (tmp->level == w)
            return tmp;

        if ((k >> (w - tmp->level - 1)) & 1)
            tmp = tmp->right;
        else
            tmp = tmp->left;

        if (tmp->key > k) {
            return tmp->left;
        }
        return tmp;
    }

    /**
     * @brief Вставка нового ключа в дерево
     * 
     * @param k Вставляемый ключ
     * 
     * @details Алгоритм:
     * 1. Создает новый листовой узел
     * 2. Обновляет двусвязный список листьев
     * 3. Создает недостающие внутренние узлы
     * 4. Обновляет указатели на потомков
     */
    void insert(int k) {
        Node* node = new Node();
        node->key = k;
        node->level = w;

        // Обновление двусвязного списка листьев
        Node* pre = predecessor(k);
        Node* suc = successor(k);
        
        if (pre != nullptr) {
            if (pre->level != w) {
                std::cout << "Wierd level " << pre->level << '\n';
            }
            node->right = pre->right;
            pre->right = node;
            node->left = pre;
        }
        
        if (suc != nullptr) {
            if (suc->level != w) {
                std::cout << "Wierd level " << suc->level << '\n';
            }
            node->left = suc->left;
            suc->left = node;
            node->right = suc;
        }

        // Создание внутренних узлов
        int lvl = 1, prefix;
        while (lvl != w) {
            prefix = k >> (w - lvl);
            if (hash_table[lvl].find(prefix) == hash_table[lvl].end()) {
                Node* inter = new Node();
                inter->level = lvl;
                hash_table[lvl][prefix] = inter;
                
                if (prefix & 1)
                    hash_table[lvl - 1][prefix >> 1]->right = inter;
                else
                    hash_table[lvl - 1][prefix >> 1]->left = inter;
            }
            ++lvl;
        }
        
        hash_table[w][k] = node;
        if (k & 1)
            hash_table[w - 1][k >> 1]->right = node;
        else
            hash_table[w - 1][k >> 1]->left = node;

        // Обновление указателей на потомков
        prefix = k;
        lvl = w - 1;
        while (lvl != 0) {
            prefix = prefix >> 1;
            if (hash_table[lvl][prefix]->left == nullptr)
                hash_table[lvl][prefix]->left = getLeftmostLeaf(hash_table[lvl][prefix]->right);
            else if (hash_table[lvl][prefix]->right == nullptr)
                hash_table[lvl][prefix]->right = getRightmostLeaf(hash_table[lvl][prefix]->left);
            --lvl;
        }
        
        if (hash_table[0][0]->left == nullptr) {
            hash_table[0][0]->left = getLeftmostLeaf(hash_table[0][0]->right);
        }
        if (hash_table[0][0]->right == nullptr) {
            hash_table[0][0]->right = getRightmostLeaf(hash_table[0][0]->left);
        }
    }
};

/**
 * @brief Двоичное дерево поиска на основе std::map
 * 
 * Вспомогательная структура для Y-Fast Trie.
 * Хранит ключи в отсортированном порядке для быстрого поиска соседей.
 */
class BinarySearchTree {
public:
    std::map<int, int> tree;  ///< Красно-черное дерево STL

    /**
     * @brief Вставка пары ключ-значение
     * @param k Ключ
     * @param val Значение
     */
    void insert(int k, int val) {
        tree[k] = val;
    }

    /**
     * @brief Поиск следующего элемента (>= k)
     * @param k Ключ
     * @return Следующий ключ или -1, если не найден
     */
    int successor(int k) {
        std::map<int, int> ::iterator tmp = tree.lower_bound(k);
        if (tmp == tree.end())
            return -1;
        else
            return tmp->first;
    }

    /**
     * @brief Поиск предыдущего элемента (<= k)
     * @param k Ключ
     * @return Предыдущий ключ или -1, если не найден
     */
    int predecessor(int k) {
        std::map<int, int> ::iterator tmp = tree.upper_bound(k);
        if (tmp == tree.begin())
            return -1;
        tmp = std::prev(tmp);
        return tmp->first;
    }
};

/**
 * @brief Y-Fast Trie (Y-быстрое дерево)
 * 
 * Реализация вероятностной структуры данных, сочетающей X-Fast Trie
 * и несколько BST для эффективного хранения ключей.
 * 
 * @details Особенности:
 * - Объединяет X-Fast Trie для навигации и BST для хранения данных
 * - Каждый узел X-Fast Trie содержит BST с элементами в своем диапазоне
 * - Ожидаемое время операций: O(log log U)
 * 
 * @note Более эффективен чем X-Fast Trie для больших объемов данных
 * @see XfastTrie, BinarySearchTree
 */
class YfastTrie {
    /**
     * @brief Хеш-таблица BST для каждого узла X-Fast Trie
     * 
     * Ключ - ключ узла X-Fast Trie, значение - BST с элементами в этом диапазоне
     */
    std::unordered_map<int, BinarySearchTree> bst;
    
    XfastTrie xtrie;  ///< X-Fast Trie для навигации
    int w;            ///< Битовая глубина (унаследована от X-Fast Trie)

    /**
     * @brief Вычисление количества бит для числа x
     * @param x Число
     * @return Количество бит
     */
    int getDigitCount(int x) {
        int count = 0;
        for (; x > 0; x >>= 1, ++count);
        return count;
    }

public:
    /**
     * @brief Конструктор Y-Fast Trie
     * @param u Максимальное значение ключа (верхняя граница)
     */
    YfastTrie(int u) {
        w = getDigitCount(u);
        xtrie = XfastTrie(u);
    }

    /**
     * @brief Поиск значения по ключу
     * @param k Ключ
     * @return Значение или -1, если ключ не найден
     * 
     * @details Ищет в BST как следующего, так и предыдущего представителя
     */
    int find(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);

        if (bst[suc->key].tree.find(k) != bst[suc->key].tree.end())
            return bst[suc->key].tree[k];
        if (bst[pre->key].tree.find(k) != bst[pre->key].tree.end())
            return bst[pre->key].tree[k];

        return -1;
    }

    /**
     * @brief Поиск следующего ключа (>= k)
     * @param k Ключ
     * @return Следующий ключ или максимальное значение, если не найден
     */
    int successor(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);
        
        int x = 2 << 2, y = 2 << w;  // Инициализация большими значениями

        if (suc != nullptr)
            x = bst[suc->key].successor(k);
        if (pre != nullptr)
            y = bst[pre->key].successor(k);

        return (x < y) ? x : y;
    }

    /**
     * @brief Поиск предыдущего ключа (<= k)
     * @param k Ключ
     * @return Предыдущий ключ или -1, если не найден
     */
    int predecessor(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);
        int x = -1, y = -1;
        
        if (suc != nullptr)
            x = bst[suc->key].predecessor(k);
        if (pre != nullptr)
            y = bst[pre->key].predecessor(k);

        return (x > y) ? x : y;
    }

    /**
     * @brief Вставка ключа и значения
     * @param k Ключ
     * @param val Значение
     * 
     * @details Если ключ-представитель не существует, создает новый узел в X-Fast Trie
     */
    void insert(int k, int val) 
    {
        Node* suc = xtrie.successor(k);
        if (suc == nullptr) {
            xtrie.insert(k);
            bst[k] = BinarySearchTree();
            bst[k].tree[k] = val;
        }
        else {
            int succ = suc->key;
            std::cout << succ << '\n';
            bst[succ].tree[k] = val;
        }
    }

    /**
     * @brief Проверка существования ключа в дереве
     * @param k Ключ
     * @return true если ключ существует, false в противном случае
     * 
     * @details Проверяет наличие ключа в BST как следующего, так и предыдущего представителя
     */
    bool exists(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);

        // Поиск в BST следующего представителя
        if (suc != nullptr && bst[suc->key].tree.find(k) != bst[suc->key].tree.end()) {
            return true;
        }

        // Поиск в BST предыдущего представителя
        if (pre != nullptr && bst[pre->key].tree.find(k) != bst[pre->key].tree.end()) {
            return true;
        }

        return false;
    }
};