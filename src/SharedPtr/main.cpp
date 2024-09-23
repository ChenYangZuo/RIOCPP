#include <iostream>
#include <memory>
#include <functional>
#include <queue>
#include <atomic>
#include <condition_variable>

#include "SharedPointer.h"


// 任务类，优先级属性
class Task{
public:
    int id;
    int priority;
    std::function<void()> func;

    Task(int i, int p, std::function<void()> f) {
        id = i;
        priority = p;
        func = std::move(f);
    }

    ~Task() = default;  // 使用编译器提供的默认析构函数

    bool operator < (const Task& other) const{
        return priority < other.priority;
    }
};

// 线程池类，多个线程, 带优先级
class PriorityThreadPool{
private:
    std::vector<std::thread> workers;   //工作线程
    std::priority_queue<Task> tasks;    //优先队列存储任务
    std::mutex mtx;                     //互斥锁保护任务队列
    std::condition_variable cv;         //条件变量控制任务获取
    std::atomic<bool> stop;             //控制线程池是否停止

    void worker() {
        while (true) {
            Task task(0, 0, [] {});
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return stop || !tasks.empty(); });

                if (stop && tasks.empty())
                    return;

                task = tasks.top();
                tasks.pop();
            }
            task.func();
        }
    }

public:
    PriorityThreadPool(size_t threadCount) : stop(false) {
        for (size_t i = 0; i < threadCount; ++i)
            workers.emplace_back([this] { worker(); });
    }

    void enqueue(int id, int priority, std::function<void()> func) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.emplace(id, priority, std::move(func));
        }
        cv.notify_one();
    }

    ~PriorityThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();

        for (std::thread &worker : workers)
            if (worker.joinable())
                worker.join();
    }
};

// 示例类
class MyObject {
public:
    MyObject() {
        std::cout << "MyObject created" << std::endl;
    }
    ~MyObject() {
        std::cout << "MyObject destroyed" << std::endl;
    }
    void display() {
        std::cout << "Displaying MyObject" << std::endl;
    }
};




// 线程3的函数，调用拷贝构造函数
void threadFunction3(SharedPointer<MyObject>& other) {
    SharedPointer<MyObject> sp3;
    sp3 = other;
    std::cout << "Thread3: After sp3 assignment, ref_count = " << other.get_lock()->get() << std::endl;
    sp3->display();
}

// 线程4的函数, 调用拷贝赋值操作符
void threadFunction4(SharedPointer<MyObject>& other, int taskid) {
    SharedPointer<MyObject> sp4 = other;
    std::cout << "Thread4: After sp4 copy, ref_count = " << other.get_lock()->get() << std::endl;
    sp4->display();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Thread " << std::this_thread::get_id() << ": Task " << taskid << " completed.\n";
}




int main() {
/*
 * 线程池，支持任务优先级
 */
    MyObject* aaa = new MyObject();
    SharedPointer<MyObject> sp1(aaa);
    std::cout << "Main: After sp1 first creation, ref_count = " << sp1.get_lock()->get() << std::endl;
    sp1->display();

    PriorityThreadPool pool(8);
    for(int i=0; i< 640; i++){
        pool.enqueue(i, i%5, [&, i] { threadFunction4(sp1, i);});     // 设置优先级，优先级循环变化
    }

    std::cout << "Main: After 640 creation, ref_count = " << sp1.get_lock()->get() << std::endl;
    //TODO 条件变量，等待所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(2));


/*
 * 简单多线程
 */
//    // 创建一个 MyObject 对象的共享指针
//    SharedPointer<MyObject> sp1(new MyObject());
//    std::cout << "Main: After sp1 creation, ref_count = " << sp1.get_lock()->get() << std::endl;
//    sp1->display();
//
//    // 启动线程
//    std::vector<std::thread> threads;
//    threads.reserve(640);
//    for(int i=0; i<640; i++){
//        threads.emplace_back(threadFunction4, std::ref(sp1));
//    }
//
//    std::cout << "Main: After 640 creation, ref_count = " << sp1.get_lock()->get() << std::endl;
//    // 等待所有线程完成
//    for(auto& th : threads){
//        th.join();
//    }


/*
 分别调用 带指针的构造函数， 拷贝构造函数， 拷贝赋值操作符
 */
//    MyObject* aaa = new MyObject();
//
//    SharedPointer<MyObject> ssp1(aaa);
//    std::cout << "Main: After ssp1 creation, ref_count = " << ssp1.get_lock()->get() << std::endl;
//
//    SharedPointer<MyObject> ssp2(aaa);      //触发带指针的构造函数, 不能这么用，STL库的共享指针也不支持
//    std::cout << "Main: After ssp2 creation, ref_count = " << ssp2.get_lock()->get() << std::endl;
//
//    SharedPointer<MyObject> ssp3(ssp1);      //触发拷贝构造函数
//    std::cout << "Main: After ssp3 creation, ref_count = " << ssp3.get_lock()->get() << std::endl;
//    SharedPointer<MyObject> ssp4 = ssp1;     //触发拷贝构造函数
//    std::cout << "Main: After ssp4 creation, ref_count = " << ssp4.get_lock()->get() << std::endl;
//
//    SharedPointer<MyObject> ssp5;
//    ssp5 = ssp1;    //触发拷贝赋值操作符
//    std::cout << "Main: After ssp5 creation, ref_count = " << ssp5.get_lock()->get() << std::endl;
//
//    std::this_thread::sleep_for(std::chrono::milliseconds (50));


/*
 * 初步调用自定义共享指针
 */
//    // 创建一个 MyObject 对象的共享指针
//    SharedPointer<MyObject> sp1(new MyObject());
//    std::cout << "After sp1 creation, ref_count = " << sp1.getRefCount() << std::endl;
//
//    // 使用共享指针访问对象
//    sp1->display();
//
//    {
//        // 创建 sp1 的副本，引用计数增加
//        SharedPointer<MyObject> sp2 = sp1;
//        std::cout << "After sp2 copy, ref_count = " << sp1.getRefCount() << std::endl;
//        sp2->display(); // 调用 display 方法
//    } // sp2 出作用域，引用计数减少
//
//    std::cout << "After sp2 delete, ref_count = " << sp1.getRefCount() << std::endl;
//
//    SharedPointer<MyObject> sp3;
//    sp3 = sp1;
//    std::cout << "After sp3 assignment, ref_count = " << sp1.getRefCount() << std::endl;
//    sp3->display(); // 调用 display 方法

    return 0; // sp1 离开作用域，引用计数为 0，销毁对象
}
