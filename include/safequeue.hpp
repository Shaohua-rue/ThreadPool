#pragma once
#include <queue>
#include <mutex>
#include "noncopyable.hpp"

/**
 * @brief 封装线程安全队列
*/

template<typename T>
class SafeQueue : public noncopyable
{
public:
    //默认构造与析构即可

    bool empty()const{
        std::unique_lock<std::mutex>(mutex_);
        return queue_.empty();
    }
    int size()const{
        std::unique_lock<std::mutex>(mutex_);
        return queue_.size();
    }
    void push(T&& t){
        std::unique_lock<std::mutex>(mutex_);
        queue_.emplace(std::move(t));
    }
    bool pop(T& t){
        std::unique_lock<std::mutex>(mutex_);
        if(queue_.empty()) 
            return false;
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }
private:
    std::queue<T> queue_;
    std::mutex mutex_;
};