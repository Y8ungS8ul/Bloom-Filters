#ifndef SKIP_NODE_HPP
#define SKIP_NODE_HPP

#include <cstring>

/**
 * @brief Узел списка с пропусками (Skip List Node)
 * 
 * Представляет элемент списка с пропусками, содержащий значение
 * и массив указателей на следующие узлы на разных уровнях.
 * 
 * @details SkipNode хранит:
 *          - Целочисленное значение
 *          - Массив указателей forward для доступа к узлам на разных уровнях
 * 
 * @see SkipList
 */
class SkipNode 
{
public:
    int value;              ///< Значение, хранящееся в узле
    SkipNode** forward;     ///< Массив указателей на следующие узлы (по уровням)

    /**
     * @brief Конструктор узла списка с пропусками
     * @param level Уровень узла (определяет размер массива forward)
     * @param value Значение для хранения в узле
     * 
     * @details Выделяет память под массив forward размером (level + 1)
     *          и инициализирует все указатели нулями.
     */
    SkipNode(int level, int& value) {
        forward = new SkipNode*[level + 1];
        memset(forward, 0, sizeof(SkipNode*) * (level + 1));
        this->value = value;
    }
    
    /**
     * @brief Деструктор узла
     * @details Освобождает память, выделенную под массив forward
     */
    ~SkipNode() {
        delete[] forward;
    }
};

#endif // SKIP_NODE_HPP