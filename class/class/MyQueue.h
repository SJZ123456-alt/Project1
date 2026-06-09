#pragma once

template <typename T>
struct QueueNode {
    T data;
    QueueNode* next;
    QueueNode(T d) : data(d), next(nullptr) {}
};

template <typename T>
class MyQueue {
private:
    QueueNode<T>* head;
    QueueNode<T>* tail;
    int count; 

public:
    MyQueue() : head(nullptr), tail(nullptr), count(0) {}
    ~MyQueue() 
    {
        while (!empty()) pop();
    }
    void push(T val) 
    {
        QueueNode<T>* newNode = new QueueNode<T>(val);
        if (!tail) 
        {
            head = tail = newNode;
        }
        else 
        {
            tail->next = newNode;
            tail = newNode;
        }
        count++;
    }
    void pop() 
    {
        if (empty()) return;
        QueueNode<T>* temp = head;
        head = head->next;
        if (!head) tail = nullptr;
        delete temp;
        count--;
    }
    T front() 
    { 
        return head->data; 
    }
    bool empty() 
    { 
        return head == nullptr; 
    }
    int size() 
    { 
        return count; 
    }
};