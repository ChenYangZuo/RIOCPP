#include <iostream>
#include <memory>

class MyClass {
public:
    MyClass() = default;

    ~MyClass() = default;
private:
    int a;
};

int main() {
    std::cout << "Hello, World!" << std::endl;

    MyClass* obj = new MyClass();
    std::shared_ptr<MyClass> ptr1(obj);
    std::shared_ptr<MyClass> ptr2(ptr1);

    std::cout << ptr1.use_count() << std::endl;
    std::cout << ptr2.use_count() << std::endl;

    return 0;
}
