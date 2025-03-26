#pragma once
#include <vector>
#include <queue>
#include <atomic>

#include <functional>
#include <iostream>
#include <unordered_map>
#include "any.h"
#include "semaphore.h"
#include "thread.h"
#include "task.h"

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

//线程池支持模式
enum  class PoolMode
{
    MODE_FIXED, //固定大小
    MODE_CACHED //动态增长
};


class ThreadPool
{
public:
    //构造函数
    ThreadPool();
    //析构函数
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    //开启线程池
    void start(int initThreadSize = 4);

    //设置线程池模式
    void setMode(PoolMode mode = PoolMode::MODE_FIXED){
        if(checkPoolStatus())
            return;
        poolMode_ = mode; }


    //设置task任务队列上限阈值
    void setTaskQueMaxThreshold(size_t threshhold){ taskQueMaxThreshold_ = threshhold; }

    //给线程池提交任务
    Result submitTask(std::shared_ptr<Task> sp);

private:
    void threadFunc(int threadId);

    //检查pool工作状态
    bool checkPoolStatus()const{return isPoolRunning_;}
private:
    //std::vector<std::unique_ptr<Thread>> threads_; //线程列表
    std::unordered_map<int,std::unique_ptr<Thread>> threads_;
    int initThreadSize_; //初始化线程数量
    std::atomic_int curThreadSize_; //当前线程数量

    std::queue<std::shared_ptr<Task>> taskQue_; //任务队列
    std::atomic_int taskSize_; //任务数量
    size_t taskQueMaxThreshold_; //任务队列阈值 

    std::mutex  taskQueMtx_;    //任务队列互斥锁
    std::condition_variable notFull_;   //任务队列非满条件变量
    std::condition_variable notEmpty_;  //任务队列非空条件变量
    std::condition_variable exitCond_;  //等待线程资源全部回收

    PoolMode poolMode_; //线程池模式

    
    std::atomic_bool isPoolRunning_;//表示线程池工作状态

    std::atomic_int idleThreadSize_;    //空闲线程的数量

};