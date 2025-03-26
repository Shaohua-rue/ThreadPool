#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <iostream>

const size_t TASK_MAX_THRESHOLD = 1024;
//Any 类型：可以接收任意数据类型
class Any 
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

    //这个构造函数，可以接收任意数据类型
    template<typename T>
    Any(T data):base_(std::make_unique<Derive<T>>(data)){}

    //将Any对象里面存储的data数据提取出来
    template<typename T>
    T  cast_()
    {
        //从基类base_找到它所指向的Derive<T>对象，从它里面取出data成员变量
        Derive<T>* p = dynamic_cast<Derive<T>*>(base_.get());
        if (p == nullptr)
        {
            throw std::logic_error("type is unmatch");
            
        }
        return p->data_;
    }
private:
    //基类类型
    class Base
    {
        public:
            virtual ~Base(){}
    };
    //派生类类型
    template<typename T>
    class Derive:public Base
    {
        public:
           Derive(T data):data_(data){}
           T data_;
    };
private:
    //定义一个基类的指针
    std::unique_ptr<Base> base_;
};

//实现一个信号量
class Semaphore
{
public:
    Semaphore(int limit = 0):resLimit_(limit){}
    ~Semaphore(){}

    //获取一个信号量资源
    void wait()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        //等待信号量有资源，没有资源的话，会阻塞这个资源
        cond_.wait(lock,[&](){
            return resLimit_ > 0;
        });

        resLimit_--;
    }

    //增加一个信号量资源
    void post()
    {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        cond_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

// Task的前置声明
class Task;

// ʵ定义返回值类
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// 线程设置返回值
	void setVal(Any any);

	// 客户获得线程返回值
	Any get();
private:
	Any any_; // Any类接收
	Semaphore sem_; // 信号量接受
	std::shared_ptr<Task> task_; //ָ线程任务绑定 
	std::atomic_bool isValid_; // 返回值是否有效
};

class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	//纯虚函数，子类继承必须实现run()函数
	virtual Any run() = 0;

private:
	Result* result_; //任务返回值
};
//线程池支持模式
enum  class PoolMode
{
    MODE_FIXED, //固定大小
    MODE_CACHED //动态增长
};
class Thread
{
public:
    using ThreadFunc = std::function<void()>;

    //线程构造
    Thread(ThreadFunc func):func_(func){}
    //线程析构
    ~Thread(){}

    //启动线程
    void start(){
        //创建一个线程来执行一个线程函数
        std::thread t(func_);   
        t.detach(); //设置分离线程
    }

private:
    ThreadFunc func_;
   
};
/*
example:
TreadPool pool;
pool.start(4);
class MyTask:public Task
{
public:
    void run()
    {
    //线程代码
    }
};
pool.submitTask(std::make_shared<MyTask>());
*/
class ThreadPool
{
public:
    //构造函数
    ThreadPool():taskSize_(0),taskQueMaxThreshold_(TASK_MAX_THRESHOLD),poolMode_(PoolMode::MODE_FIXED){}

    //析构函数
    ~ThreadPool(){}

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //开启线程池
    void start(int initThreadSize = 4);

    //设置线程池模式
    void setMode(PoolMode mode){poolMode_ = mode; }


    //设置task任务队列上限阈值
    void setTaskQueMaxThreshold(size_t threshhold){ taskQueMaxThreshold_ = threshhold; }

    //给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);

private:
    void threadFunc();
private:
    std::vector<std::unique_ptr<Thread>> threads_; //线程列表
    int initThreadSize_; //初始化线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; //任务队列
    std::atomic_int taskSize_; //任务数量
    size_t taskQueMaxThreshold_; //任务队列阈值 

    std::mutex  taskQueMtx_;    //任务队列互斥锁
    std::condition_variable notFull_;   //任务队列非满条件变量
    std::condition_variable notEmpty_;  //任务队列非空条件变量

    PoolMode poolMode_; //线程池模式

};