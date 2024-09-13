//
// Created by zuo-c on 2024/9/12.
//

#ifndef PROJECTS_SHAREDPOINTER_H
#define PROJECTS_SHAREDPOINTER_H

#include <cstddef>

struct ControlBlock {
    size_t use_count = 0;

    ControlBlock() = default;

    ~ControlBlock() = default;
};

template<typename T>
class SharedPointer {
public:
    SharedPointer();

    SharedPointer(const SharedPointer &other);

    SharedPointer(SharedPointer &&other);

    ~SharedPointer();

    SharedPointer &operator=(const SharedPointer &other);

    SharedPointer &operator=(SharedPointer &&other) noexcept;

    /***********************************************
     * 功能：获取当前管理的裸指针
     * 输入：NULL
     * 返回：裸指针
     ***********************************************/
    T *get();

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
    size_t use_count();

private:
    T *m_ptr;
    ControlBlock *m_cb;
};


#endif //PROJECTS_SHAREDPOINTER_H
