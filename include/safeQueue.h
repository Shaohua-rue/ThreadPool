#pragma once
#include <mutex>
#include <queue>

template<typename T>
class SafeQueue
{
public:
    SafeQueue() = default;
    ~SafeQueue() = default;
    SafeQueue(const SafeQueue&) = delete;
    SafeQueue& operator=(const SafeQueue&) = delete;
    bool empty()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    int size()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
    void enQueue(T&& t)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(t));
    }
    //支持右值引用
    bool deQueue(T& t)
    {
        std::unique_lock<std::mutex> lock(mutex_); 
        if(queue_.empty()){
            return false;
        }
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }
private:
    std::queue<T> queue_;
    std::mutex mutex_;
};