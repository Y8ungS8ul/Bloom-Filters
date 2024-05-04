#include <iostream>
#include <unordered_map>
#include <vector>
#include <map>
#include <iterator>


//Структура узла дерева
struct Node 
{
    int level;

    int key;

    //указатели на двух потомков двоичного дерева
    Node* left, * right;

    Node() 
    {
        level = -1;
        left = nullptr;
        right = nullptr;
    }
};

//Верхушка - X-быстрое дерево

class XfastTrie 
{
    //количество бит в ключах, которые хранятся в дереве  - определяет масштаб и размер дерева
    int w;
    std::vector<std::unordered_map<int, Node*>> hash_table;

    //вектор хеш-таблиц для каждого уровня дерева. Каждая ХТ содержит указатели на узлы соответствующего уровня для быстрого нахождения узлов по ключам

    //Функция для вычисления количества бит в числе X
    int getDigitCount(int x) 
    {
        int count = 0;
        for (; x > 0; x >>= 1, ++count);
        return count;
    }

    //методы для навигации по дереву - вычисление ключей левого и правого потомка
    int leftChild(int x) 
    {
        return (x << 1);
    }

    int rightChild(int x) {
        return ((x << 1) | 1);
    }


    //
    Node* getLeftmostLeaf(Node* parent) {
        while (parent->level != w) {
            if (parent->left != nullptr)
                parent = parent->left;
            else
                parent = parent->right;
        }
        return parent;
    }

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

    //конструктор: число 
    XfastTrie(int U) 
    {
        //получаем количество бит в числе U
        w = getDigitCount(U);

        //инициализация вектора, где каждый элмент - хт 
        hash_table.assign(w + 1, std::unordered_map<int, Node*>());

        //Создание корневого узла
        Node* root = new Node();
        root->level = 0; // 0-уровень соответствует корню
        hash_table[0][0] = root; //добавление узла в ХТ

    }

    Node* find(int k) 
    {
        if (hash_table[w].find(k) == hash_table[w].end())
            return nullptr;
        return hash_table[w][k];
    }

    Node* successor(int k) {
        int low = 0,
            high = w + 1,
            mid, prefix;

        Node* tmp = nullptr;
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
            // occours on first insertion  
            return nullptr;

        if (tmp->level == w)
            return tmp;

        // use descendant node
        if ((k >> (w - tmp->level - 1)) & 1)
            tmp = tmp->right;
        else
            tmp = tmp->left;

        if (tmp->key < k) {
            return tmp->right;
        }
        return tmp;
    }

    Node* predecessor(int k) {
        // completely same as successor except lst section
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
            // occours on first insertion
            return nullptr;

        if (tmp->level == w)
            return tmp;

        // use descendant node
        if ((k >> (w - tmp->level - 1)) & 1)
            tmp = tmp->right;
        else
            tmp = tmp->left;

        if (tmp->key > k) {
            return tmp->left;
        }
        return tmp;
    }

    void insert(int k) {
        Node* node = new Node();
        node->key = k;
        node->level = w;

        // update linked list
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

        // update descendant pointers
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

class BinarySearchTree {
public:
    std::map<int, int> tree;

    void insert(int k, int val) {
        tree[k] = val;
    }

    int successor(int k) {
        std::map<int, int> ::iterator tmp = tree.lower_bound(k);
        if (tmp == tree.end())
            return -1;
        else
            return tmp->first;
    }

    int predecessor(int k) {
        std::map<int, int> ::iterator tmp = tree.upper_bound(k);
        if (tmp == tree.begin())
            return -1;
        tmp = std::prev(tmp);
        return tmp->first;
    }
};

class YfastTrie {
    std::unordered_map<int, BinarySearchTree> bst;
    XfastTrie xtrie;
    int w;

    int getDigitCount(int x) {
        int count = 0;
        for (; x > 0; x >>= 1, ++count);
        return count;
    }

public:
    YfastTrie(int u) {
        w = getDigitCount(u);
        xtrie = XfastTrie(u);
    }

    int find(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);

        if (bst[suc->key].tree.find(k) != bst[suc->key].tree.end())
            return bst[suc->key].tree[k];
        if (bst[pre->key].tree.find(k) != bst[pre->key].tree.end())
            return bst[pre->key].tree[k];

        return -1;
    }

    int successor(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);
        // used as infinite here
        int x = 2 << 2, y = 2 << w;

        if (suc != nullptr)
            x = bst[suc->key].successor(k);
        if (pre != nullptr)
            y = bst[pre->key].successor(k);

        return (x < y) ? x : y;
    }

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

    void insert(int k, int val) 
    {
        Node* suc = xtrie.successor(k);
        if (suc == nullptr) {
            xtrie.insert(k);
            // representative can be anything. using first element as
            // representative requires length of xfast trie to be u.
            bst[k] = BinarySearchTree();
            bst[k].tree[k] = val;
        }
        else {
            int succ = suc->key;
            std::cout << succ << '\n';
            bst[succ].tree[k] = val;
        }
    }

    bool exists(int k) {
        Node* suc = xtrie.successor(k);
        Node* pre = xtrie.predecessor(k);

        // Проверяем наличие ключа в BST, связанном с преемником
        if (suc != nullptr && bst[suc->key].tree.find(k) != bst[suc->key].tree.end()) {
            return true;
        }

        // Проверяем наличие ключа в BST, связанном с предшественником
        if (pre != nullptr && bst[pre->key].tree.find(k) != bst[pre->key].tree.end()) {
            return true;
        }

        return false;
    }

};