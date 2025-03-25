#include <iostream>
#include "threadPool.h"

#include <thread>
#include <chrono>

class MyTask :public Task {
public:
    void run()
    {
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "task " << std::this_thread::get_id() << " is running" << std::endl;
    }
};
int main()
{
    ThreadPool pool;
    pool.start(4);
    for(int i = 0; i < 5; ++i)
    {
        std::shared_ptr<Task> sp(new MyTask);
        pool.submitTask(sp);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
}