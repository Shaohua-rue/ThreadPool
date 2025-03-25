#include"threadPool.h"




//开启线程池
void ThreadPool::start(int initThreadSize)
{
    initThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i = 0; i < initThreadSize; ++i)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this));
        threads_.emplace_back(std::move(ptr));
    }

    //启动线程
    for(int i = 0; i < initThreadSize; ++i)
    {
        threads_[i]->start();
    }
}
//线程函数
void ThreadPool::threadFunc()
{
    for(;;)
    {
        std::shared_ptr<Task> task;
        {
            //先获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            //等待notEmpty条件变量，如果有任务，就执行任务，没有任务就阻塞等待
            std::cout<<"tid:"<<std::this_thread::get_id()<<"尝试获取任务..."<<std::endl;
            notEmpty_.wait(lock,[&](){
                return !taskQue_.empty();
            });
            //从任务队列中取出一个任务，执行任务
            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;
         std::cout<<"tid:"<<std::this_thread::get_id()<<"获取到任务..."<<std::endl;
            //如果依然有剩余任务，继续通知其他线程执行任务
            if(!taskQue_.empty())
            {
                notEmpty_.notify_all();
            }

            //通知任务队列，任务队列不为空了，可以继续添加任务了
            notFull_.notify_all();
        }   // 释放锁

        //当前线程负责执行这个函数
        if(task != nullptr)
        {
            task->run();  
        }
          
    }
}
//给线程池提交任务 用户调用改接口，传入任务对象，生产任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
    //获取锁
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    //线程的通信，等待任务队列不为满
    //用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败
    // while(taskQue_.size() == taskQueMaxThreshold_)
    // {
    //     notFull_.wait(lock);
    // }
    //wait wait_for wait_until
    if(!notFull_.wait_for(lock,std::chrono::seconds(1),[&](){
        return taskQue_.size() < taskQueMaxThreshold_;
    }))
    {
        //表示notFull_超时了,条件依然没有满足，返回false
        std::cerr<<"task queue is full, submit task fail."<<std::endl;
        return;
    }


    //如果有空余，把任务放到任务队列中
    taskQue_.emplace(sp);
    taskSize_++; 
    //因为新放置任务，任务队列不空，notEmpty条件变量通知线程池中的线程，有任务了  
    notEmpty_.notify_all();   


}
