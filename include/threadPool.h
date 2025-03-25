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

//任务抽象基类
class Task
{
public:
    //用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务处理
    virtual void run() = 0;
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
    void submitTask(std::shared_ptr<Task> sp);

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