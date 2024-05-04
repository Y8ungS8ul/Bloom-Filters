#include <iostream>
#include <stdexcept>

// Определение структуры узла
struct Nodell 
{
    int data;
    Nodell* next;

    Nodell(int data) : data(data), next(nullptr) {}
};

// Определение класса LinkedList
class LinkedList 
{
private:
    Nodell* head;

public:
    LinkedList() : head(nullptr) {}

    // Добавление элемента в список
    void insert(int data) {
        Nodell* newNode = new Nodell(data);
        if (head == nullptr) 
        {
            head = newNode;
        }
        else 
        {
            Nodell* current = head;
            while (current->next != nullptr) 
            {
                current = current->next;
            }
            current->next = newNode;
        }
    }

    // Проверка наличия элемента в списке
    bool exists(int data) {
        Nodell* current = head;
        while (current != nullptr) {
            if (current->data == data) {
                return true;
            }
            current = current->next;
        }
        return false;
    }

    // Удаление всех элементов списка
    ~LinkedList() {
        while (head != nullptr) {
            Nodell* nextNode = head->next;
            delete head;
            head = nextNode;
        }
    }
};