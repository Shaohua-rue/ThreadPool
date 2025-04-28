/**
 * @brief 线程池实现
 * 
 * 生产者消费者模型
 * 
*/
#pragma once

#include <mutex>
#include <atomic>
#include <memory>
#include <future>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <condition_variable>


#include "noncopyable.hpp"
#include "safequeue.hpp"
#include "thread.hpp"

int Thread::generateId_ = 0;
const int MAX_TASK_SIZE = 1024;     //最大任务数量
const int MAX_THREAD_SIZE = 1024;   //最大线程数量
const int MAX_THREAD_IDLE_TIME = 10;    //线程最大空闲时间(超时cached模式下会清除多余线程)

enum class PoolMode
{
    MODE_CACHED,    //动态增加模式
    MODE_FIXED      //固定线程数量
};
class ThreadPool : public noncopyable
{
public:
    ThreadPool():
    initThreadSize_(0),
    idleThreadSize_(0),
    taskQueMaxThreshHole_(MAX_TASK_SIZE),
    maxThreadSize_(MAX_THREAD_SIZE),
    poolMode_(PoolMode::MODE_FIXED),
    isPoolRunning_(false){}

    /**
     * @brief 线程池析构函数
    */
    ~ThreadPool(){
        //等待任务队列为空时，才能正常释放线程
        isPoolRunning_ = false;

        std::unique_lock<std::mutex> lock(taskMutex_); //获取任务队列锁
        notEmptyCond_.notify_all();  
        exitCond_.wait(lock,[&](){return threads_.empty();});
    }

    /**
     * @brief 设置线程模式
     * @param poolMode 线程模式
    */
    void setMode(PoolMode poolMode){poolMode_ = poolMode;}

    /**
     * @brief 返回线程池状态
    */
    bool checkPoolRunningState()const{return isPoolRunning_;}


    /**
     * @brief 开启线程池
     * @param initThreadSize 初始化线程数量
    */
    void start(int initThreadSize = std::thread::hardware_concurrency())
    {
        isPoolRunning_ = true;
        initThreadSize_ = initThreadSize;
        idleThreadSize_ = initThreadSize;

        for(int id = 0; id < initThreadSize_; id++)
        {
            std::unique_ptr<Thread> ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this,std::placeholders::_1));
            threads_.emplace(id,std::move(ptr));
            threads_[id]->start();
        }
    }


    /**
     * @brief 任务提交
     */ 
    template<typename Func,typename... Args>
    auto submitTask(Func&& func, Args&&... args) ->std::future<decltype(func(args...))>
    {   //使用完美转发支持任意函数与参数，通过std::packaged_task包装任务，std::future用于异步返回值
        auto task = std::make_shared<std::packaged_task<decltype(func(args...))()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

        std::unique_lock<std::mutex> lock(taskMutex_);

        //----等待安全任务队列非满--------
        if(!notFullCond_.wait_for(lock,std::chrono::seconds(1),[&]()->bool{return safeTaskQue_.size() < (size_t) taskQueMaxThreshHole_;}))
        {
            std::cout <<"task queue is full, submit task failed." <<std::endl;
            auto task = std::make_shared<std::packaged_task<decltype(func(args...))()>>([]()->decltype(func(args...)) {return decltype(func(args...))();});
            (*task)();
            return task->get_future();
        }
        //Task fun =std::bind([task](){(*task)();});
        Task fun = [task](){(*task)();};     //将std::function<void()>作为任务类型，通过lambda表达式封装具体任务，实现类型擦除
        safeTaskQue_.push(std::move(fun));    //将任务移动到安全任务队列中
        
        
        notEmptyCond_.notify_all(); //此时任务队列非空，条件变量通知处理

        //---------------CACHED模式动态增加线程-------------------
        if(poolMode_ == PoolMode::MODE_CACHED && safeTaskQue_.size() > idleThreadSize_ && threads_.size() < MAX_THREAD_SIZE)
        {
            std::cout<< "create new thread..."<<std::endl;

            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int id = ptr->getId();
            threads_.emplace(id,std::move(ptr));
            threads_[id]->start();
            idleThreadSize_++;
        }
        return task->get_future();
    }

    /**
     * @brief 线程池关闭
    */ 
    void shutdown()
    {
        isPoolRunning_ = false;

        std::unique_lock<std::mutex> lock(taskMutex_); //获取任务队列锁
        notEmptyCond_.notify_all();  
        exitCond_.wait(lock,[&](){return threads_.empty();});
        std::cout << "thread pool quit" << std::endl;
    }
    

private:
    void threadFunc(int threadId)
    {
        auto lastTime = std::chrono::high_resolution_clock::now();
        Task task;
        for(;;)
        {
            {
                std::unique_lock<std::mutex> lock(taskMutex_);//获取任务队列锁

                //---------------------等待线程池任务队列非空-------------------
                while(safeTaskQue_.empty())//当任务队列为空时
                {
                    if(!isPoolRunning_) //线程池停止
                    {
                        threads_.erase(threadId);
                        std::cout << "threadId:" << threadId << " exit" << std::endl;
                        exitCond_.notify_all();
                        return;
                    }

                    if(poolMode_ == PoolMode::MODE_CACHED)//MODE_CACHED模式
                    {
                        if(std::cv_status::timeout == notEmptyCond_.wait_for(lock,std::chrono::seconds(1)))
                        {
                            auto curTime = std::chrono::high_resolution_clock::now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(curTime - lastTime);
                            //当线程空闲超过THREAD_MAX_IDLE_TIME且线程数大于初始线程数时，移除该线程
                            if(dur.count() >=  MAX_THREAD_IDLE_TIME && threads_.size() > initThreadSize_) 
                            {
                                threads_.erase(threadId);    //线程池中移除该线程
                                idleThreadSize_--;
                                std::cout << "threadId:" << threadId << " delete" << std::endl;
                                return ;
                            }
                        }
                    }
                    else    //FIXED模式
                    {
                        notEmptyCond_.wait(lock);    //等待队列不为空(线程休眠)
                    }
                }
                //---------------------执行线程任务-------------------
                idleThreadSize_--;   //线程池中空闲线程数减少
                safeTaskQue_.pop(task);
                
                if(!safeTaskQue_.empty()){
                    notEmptyCond_.notify_all();
                }
                notFullCond_.notify_all();
            }//锁的作用域结束，释放锁

            if(task != nullptr)
            {
                task(); //执行任务
            }
            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock::now();
        }
    }

private:
    //----------线程与线程池相关--------------------------------------------------------------
    PoolMode poolMode_;     //线程池模式
    int initThreadSize_;    //初始化线程数量
    int maxThreadSize_;     //最大线程数量
    std::atomic_int idleThreadSize_;    //空闲线程数量
    std::atomic_bool isPoolRunning_;    //线程池是否运行
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;  //线程容器


    //----------任务队列相关-------------------------------------------------------------------
    int taskQueMaxThreshHole_;  //最大任务队列数量
    using Task = std::function<void()>; //声明任务函数
    SafeQueue<Task> safeTaskQue_;   //定义安全任务队列

    std::mutex taskMutex_;  //任务队列互斥锁
    std::condition_variable notFullCond_;   //任务队列非满
    std::condition_variable notEmptyCond_;  //任务队列非空
    std::condition_variable exitCond_;  //线程池退出
};
