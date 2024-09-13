#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <functional> // 需要包含这个头文件来使用 std::ref
#include <queue>
#include <condition_variable>
#include <atomic>

//写个线程池
//TODO List: 1.用atomic改造sharedptr，对比加锁与原子变量的性能差异 2.如何实现uniqueptr 3.实现带优先级的线程池

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



// 共享指针 计数器类
class Lock{
    public:

    Lock(int i = 0){
        std::lock_guard<std::mutex> lock(mtx);
        //TODO
        ref_count = i;
    }

    ~Lock(){}

    // 获取底层指针
    size_t get() const { return ref_count; }

    int increase(){
        std::lock_guard<std::mutex> lock(mtx);
        return ++ref_count;
    }
    int decrease(){
        std::lock_guard<std::mutex> lock(mtx);
        return --ref_count;
    }

    bool decrease_if(){
        std::lock_guard<std::mutex> lock(mtx);
        return (--ref_count) == 0;
    }


private:

    std::mutex mtx;
    size_t ref_count;    // 引用计数器

};

// 自定义的简化版 SharedPointer 类
template<typename T>
class SharedPointer {
private:
    T* ptr;               // 指向管理对象的指针
    Lock *m_lock;

public:


    // 默认构造函数
    SharedPointer() : ptr(nullptr), m_lock(new Lock()) {
        std::cout << "SharedPointer default created, ref_count = " << m_lock->get() << std::endl;
    }

    // 带指针的构造函数
    SharedPointer(T* p) : ptr(p), m_lock(new Lock(1)) {
        std::cout << "SharedPointer created, ref_count = " << m_lock->get() << std::endl;
    }

    // 拷贝构造函数：增加引用计数
    SharedPointer(const SharedPointer& sp){
        ptr = sp.get();
        m_lock = sp.get_lock();
        std::this_thread::sleep_for(std::chrono::milliseconds (500));
        m_lock->increase();

        std::cout << "copy constructor, reference count = " << m_lock->get() << std::endl;
    }


    // 拷贝赋值操作符
    SharedPointer<T>& operator=(const SharedPointer<T>& sp) {
        if (this == &sp) return *this; // 防止自赋值

//        std::lock_guard<std::mutex> lock(mtx);
        // 先减少当前对象的引用计数
        if (m_lock->decrease_if()) {
            delete ptr;
            delete m_lock;
            std::cout << "SharedPointer deleted old object, ref_count = 0" << std::endl;
        }

        ptr = sp.get();
        m_lock = sp.get_lock();
        m_lock->increase();
        std::cout << "copy assignment, ref_count = " << m_lock->get() << std::endl;

        return *this;
    }


    // 析构函数：减少引用计数，若为 0 则释放资源
    ~SharedPointer() {
//        std::lock_guard<std::mutex> lock(mtx);
        if (m_lock->decrease_if()) {
            delete ptr;
            delete m_lock;
            std::cout << "ref_count = 0, Object destroyed" << std::endl;
        } else {
            std::cout << "Reference count decreased to " << m_lock->get() << std::endl;
        }
    }

    // 获取底层指针
    T* get() const { return ptr; }

    Lock* get_lock() const { return m_lock; }

    // 重载箭头运算符，便于访问对象成员
    T* operator->() const { return ptr; }

    // 重载解引用运算符
    T& operator*() const { return *ptr; }
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
void threadFunction3(SharedPointer<MyObject>& sp) {
    SharedPointer<MyObject> sp3;
    sp3 = sp;
    std::cout << "Thread3: After sp3 assignment, ref_count = " << sp.get_lock()->get() << std::endl;
    sp3->display();
}

// 线程4的函数, 调用拷贝赋值操作符
void threadFunction4(SharedPointer<MyObject>& sp, int taskid) {
    SharedPointer<MyObject> sp4 = sp;
    std::cout << "Thread4: After sp4 copy, ref_count = " << sp.get_lock()->get() << std::endl;
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
