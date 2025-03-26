#pragma once
#include <mutex>
#include <condition_variable>

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