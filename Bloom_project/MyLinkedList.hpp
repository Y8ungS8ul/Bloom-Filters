#include <iostream>
#include <stdexcept>

// ����������� ��������� ����
struct Nodell 
{
    int data;
    Nodell* next;

    Nodell(int data) : data(data), next(nullptr) {}
};

// ����������� ������ LinkedList
class LinkedList 
{
private:
    Nodell* head;

public:
    LinkedList() : head(nullptr) {}

    // ���������� �������� � ������
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

    // �������� ������� �������� � ������
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

    // �������� ���� ��������� ������
    ~LinkedList() {
        while (head != nullptr) {
            Nodell* nextNode = head->next;
            delete head;
            head = nextNode;
        }
    }
};