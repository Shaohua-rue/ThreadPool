#pragma once
#include <thread>
#include <functional>
class Thread
{
public:
    //声明线程函数
    using ThreadFunc = std::function<void(int)>;

    //线程构造与析构
    Thread(ThreadFunc func):func_(func),threadId_(generateId_++){}
    ~Thread(){}
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    //线程启动
    void start()
    {
        std::thread t(func_,threadId_);
        t.detach();
    }

    //获取线程ID
    int getId()const{return threadId_;}

private:
    ThreadFunc func_;   //线程函数
    int threadId_;      //线程id
    static int generateId_; //线程id生成器
};