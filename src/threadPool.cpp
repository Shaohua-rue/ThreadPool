#include "threadPool.h"

int Thread::generateId_ = 0;

const int TASK_MAX_THRESHHOLD = 1024;   //线程池最大任务数
const int THREAD_MAX_THRESHHOLD = 1024; //线程池最大线程数
const int THREAD_MAX_IDLE_TIME = 10;    //线程最大空闲时间 (cached模式清理多余线程)

/************************************
 * @brief 线程池构造函数
 * **********************************/
ThreadPool::ThreadPool():
	initThreadSize_(0),
    idleThreadSize_(0),
    taskQueMaxThreshHode_(TASK_MAX_THRESHHOLD),
    threadSizeThreshHold_(THREAD_MAX_THRESHHOLD),
    poolMode_(PoolMode::MODE_FIXED),
    isPoolRunning_(false)
{}

/************************************
 * @brief 线程池析构函数
 * **********************************/
ThreadPool::~ThreadPool()
{
    isPoolRunning_ = false;

    std::unique_lock<std::mutex> lock(taskQueMtx_); //获取任务队列锁
    notEmpty_.notify_all();  
    exitCond_.wait(lock,[&](){return threads_.empty();});
    std::cout << "thread pool exit" << std::endl;
}
/************************************
 * @brief 启动线程池
 * **********************************/
void ThreadPool::start(int initThreadSize)
{
    isPoolRunning_ = true;

    initThreadSize = initThreadSize;
    
    for(int i = 0; i < initThreadSize; i++)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        int threadId = ptr->getId();
        threads_.emplace(threadId,std::move(ptr)); //线程池增加线程
        threads_[threadId]->start();   //启动线程
        idleThreadSize_++;      //线程池中空闲线程数增加
    }   
}



/************************************
 * @brief 线程函数 
 * **********************************/

 void ThreadPool::threadFunc(int threadId)
 {
    auto lastTime = std::chrono::high_resolution_clock::now();
    Task task;
    for(;;)
    {
        {
            std::unique_lock<std::mutex> lock(taskQueMtx_);//获取任务队列锁

            //---------------------等待线程池任务队列非空-------------------
            while(taskQue_.empty())//当任务队列为空时
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
                    if(std::cv_status::timeout == notEmpty_.wait_for(lock,std::chrono::seconds(1)))
                    {
                        auto curTime = std::chrono::high_resolution_clock::now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(curTime - lastTime);
                        //当线程空闲超过THREAD_MAX_IDLE_TIME且线程数大于初始线程数时，移除该线程
                        if(dur.count() >= THREAD_MAX_IDLE_TIME && threads_.size() > initThreadSize_) 
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
                    notEmpty_.wait(lock);    //等待队列不为空
                }
            }
            //---------------------执行线程任务-------------------
            idleThreadSize_--;   //线程池中空闲线程数减少
            task = taskQue_.front();
            taskQue_.pop();
            
            if(!taskQue_.empty()){
                notEmpty_.notify_all();
            }
            notFull_.notify_all();
        }//锁的作用域结束，释放锁

        if(task != nullptr)
        {
            task(); //执行任务
        }
        idleThreadSize_++;
        lastTime = std::chrono::high_resolution_clock::now();
    }
 }