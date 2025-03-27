#pragma once
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <future>
#include <queue>
#include <memory>
#include <atomic>
#include <chrono>

#include "thread.h"
#include "safeQueue.h"


//定义线程池模式
enum class PoolMode{
    MODE_CACHED,
    MODE_FIXED
};

class ThreadPool
{
public:
    //线程池构造与析构相关
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator = (const ThreadPool&) = delete;

    //线程池启动函数
    void start(int initThreadSize = std::thread::hardware_concurrency());

    void setMode(PoolMode mode){ poolMode_ = mode;}
    template<typename Func, typename... Args>
	auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;


    bool cheakPoolRunningState()const{return isPoolRunning_;}
private:
    //线程函数
    void threadFunc(int threadId);
private:
    int initThreadSize_; //初始化线程数量
    int threadSizeThreshHold_;  //线程数量上限
    std::atomic_int idleThreadSize_;    //空闲线程数量
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;    //线程容器

    using Task = std::function<void()>;
    SafeQueue<Task> taskQue_;  //任务队列
    int taskQueMaxThreshHode_;  //任务队列上限

    std::mutex taskQueMtx_; //任务队列互斥锁
    std::condition_variable notFull_;   //任务队列非空
    std::condition_variable notEmpty_;  //任务队列非满
    std::condition_variable exitCond_;  //线程池退出


    PoolMode poolMode_; //线程池模式
    std::atomic_bool isPoolRunning_;    //线程池是否运行

};

//模版函数的声明与实现必须都在头文件中
template<typename Func, typename... Args>
auto ThreadPool::submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
{
    using RType = decltype(func(args...));
    auto task = std::make_shared<std::packaged_task<RType()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
	std::future<RType> result = task->get_future();

    std::unique_lock<std::mutex> lock(taskQueMtx_);

    // ---------------------等待线程池任务队列非满-------------------
   if (!notFull_.wait_for(lock, std::chrono::seconds(1),[&]()->bool { return taskQue_.size() < (size_t)taskQueMaxThreshHode_; }))
    {
        std::cout << "task queue is full, submit task failed."<<std::endl;
        auto task = std::make_shared<std::packaged_task<RType()>>([]()->RType {return RType();});
        (*task)();
        return task->get_future();
    }

    taskQue_.enQueue([task](){(*task)();});
    
    notEmpty_.notify_all();

    //---------------CACHED模式动态增加线程-------------------
    if(poolMode_ == PoolMode::MODE_CACHED && taskQue_.size() > idleThreadSize_ && threads_.size() < threadSizeThreshHold_)
    {
        std::cout<< "create new thread..."<<std::endl;

        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        threads_.emplace(threadId,std::move(ptr));
        threads_[threadId]->start();
        idleThreadSize_++;
    }
    return result;
}
