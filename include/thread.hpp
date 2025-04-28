#pragma once
#include <thread>
#include <functional>

#include "noncopyable.hpp"
/**
 * @brief 封装线程实现
*/
class Thread : public noncopyable
{
public:
    using ThreadFunc = std::function<void(int)>;

    Thread(ThreadFunc func) : func_(func),threadId_(generateId_++){}
    ~Thread(){};

    void start(){
        std::thread t(func_, threadId_);
        t.detach();
    }

    int getId()const {return threadId_;}
private:
    ThreadFunc func_;   //线程函数
    int threadId_;  //线程id
    static int generateId_; //线程id生成
};