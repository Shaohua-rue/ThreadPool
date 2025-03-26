#pragma once
#include <functional>
#include <thread>
class Thread
{
public:
    using ThreadFunc = std::function<void(int)>;

    //线程构造
    Thread(ThreadFunc func):
    func_(func),
    threadId_(generateId_++)
    {}
    //线程析构
    ~Thread(){}

    //启动线程
    void start(){
        //创建一个线程来执行一个线程函数
        std::thread t(func_,threadId_);   
        t.detach(); //设置分离线程
    }

    //获取线程id
    int getId(){
        return threadId_;
    }

private:
    ThreadFunc func_;
    static int generateId_;
    int threadId_;  //线程id
   
};
