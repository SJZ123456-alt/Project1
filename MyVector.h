#pragma once

template <typename T>
class MyVector {
private:
    T* data;
    int capacity;
    int length;

    void expand() {
        capacity *= 2;
        T* newData = new T[capacity];
        for (int i = 0; i < length; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
    }

public:
    MyVector() : capacity(10), length(0) {
        data = new T[capacity];
    }

    // 【核心修复】深拷贝构造函数
    MyVector(const MyVector& other) {
        capacity = other.capacity;
        length = other.length;
        data = new T[capacity];
        for (int i = 0; i < length; i++) {
            data[i] = other.data[i];
        }
    }

    // 【核心修复】赋值运算符重载
    MyVector& operator=(const MyVector& other) {
        if (this != &other) {
            delete[] data; // 先释放旧内存
            capacity = other.capacity;
            length = other.length;
            data = new T[capacity];
            for (int i = 0; i < length; i++) {
                data[i] = other.data[i];
            }
        }
        return *this;
    }

    ~MyVector() {
        delete[] data;
    }

    void push_back(T val) {
        if (length == capacity) expand();
        data[length++] = val;
    }

    int size() const { return length; }

    T& operator[](int i) { return data[i]; }
    const T& operator[](int i) const { return data[i]; }
};