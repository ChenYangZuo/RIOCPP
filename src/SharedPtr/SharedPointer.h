//
// Created by zuo-c on 2024/9/12.
// changed by wjw on  2024/9/13.
//

#ifndef PROJECTS_SHAREDPOINTER_H
#define PROJECTS_SHAREDPOINTER_H

#include <cstddef>
#include <mutex>
#include <thread>

struct ControlBlock {
public:

    ControlBlock(int i=0){
        std::lock_guard<std::mutex> lock(mtx);
        use_count = i;
    }

    ~ControlBlock() = default;

    // 获取底层指针
    size_t get() const { return use_count; }

    int increase(){
        std::lock_guard<std::mutex> lock(mtx);
        return ++use_count;
    }
    int decrease(){
        std::lock_guard<std::mutex> lock(mtx);
        return --use_count;
    }

    bool decrease_if(){
        std::lock_guard<std::mutex> lock(mtx);
        return (--use_count) == 0;
    }

private:
    std::mutex mtx;
    size_t use_count = 0;
};

template<typename T>
class SharedPointer {
public:

    //默认构造函数
    SharedPointer(){
        m_ptr = nullptr;
        m_cb = new ControlBlock();
        std::cout << "SharedPointer default created, use_count = " << m_cb->get() << std::endl;
    }

    // 带指针的构造函数
    SharedPointer(T* p){
        m_ptr = p;
        m_cb = new ControlBlock(1);
        std::cout << "SharedPointer created, use_count = " << m_cb->get() << std::endl;
    }

    //拷贝构造函数
    SharedPointer(const SharedPointer &other){
        m_ptr = other.get();
        m_cb = other.get_lock();
        std::this_thread::sleep_for(std::chrono::milliseconds (500));
        m_cb->increase();
        std::cout << "copy constructor, reference count = " << m_cb->get() << std::endl;
    }

    SharedPointer(SharedPointer &&other) = delete;

    ~SharedPointer(){
        if (m_cb->decrease_if()) {
            delete m_ptr;
            delete m_cb;
            std::cout << "use_count = 0, Object destroyed" << std::endl;
        } else {
            std::cout << "Reference count decreased to " << m_cb->get() << std::endl;
        }
    }

    SharedPointer &operator=(const SharedPointer &other){
        if (this == &other) return *this; // 防止自赋值

        // 先减少当前对象的引用计数
        if (m_cb->decrease_if()) {
            delete m_ptr;
            delete m_cb;
            std::cout << "SharedPointer deleted old object, use_count = 0" << std::endl;
        }

        m_ptr = other.get();
        m_cb = other.get_lock();
        m_cb->increase();
        std::cout << "copy assignment, use_count = " << m_cb->get() << std::endl;

        return *this;
    }

    SharedPointer &operator=(SharedPointer &&other) noexcept = delete;

    /***********************************************
     * 功能：获取当前管理的裸指针
     * 输入：NULL
     * 返回：裸指针
     ***********************************************/
    T* get() const { return m_ptr; }

    /***********************************************
     * 功能：释放当前管理的裸指针，恢复初始状态
     * 输入：NULL
     * 返回：NULL
     ***********************************************/
    void reset();

    /***********************************************
     * 功能：返回当前智能指针的计数值
     * 输入：NULL
     * 返回：计数值
     ***********************************************/
    ControlBlock* get_lock() const { return m_cb; }

    /***********************************************
     * 功能：重载箭头运算符，便于访问对象成员
     * 输入：NULL
     * 返回：裸指针
     ***********************************************/
    T* operator->() const { return m_ptr; }

    /***********************************************
     * 功能：重载解引用运算符
     * 输入：NULL
     * 返回：裸指针
     ***********************************************/
    T& operator*() const { return *m_ptr; }


private:
    T *m_ptr;
    ControlBlock *m_cb;
};


#endif //PROJECTS_SHAREDPOINTER_H
