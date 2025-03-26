#include <iostream>
#include "threadPool.h"

#include <thread>
#include <chrono>

using uLong = unsigned long long;
class MyTask :public Task {
public:
    MyTask(int begin,int end):begin_(begin),end_(end)
    {
        //std::cout << "MyTask " << std::this_thread::get_id() << " is created" << std::endl;
    }
    Any run()
    {
        uLong sum = 0;
        for(uLong i = begin_;i <= end_;++i)
        {
            sum += i;
        }
        return sum;
    }
private:
    int begin_;
    int end_;
};
int main()
{
    ThreadPool pool;
    pool.setMode();
    pool.start(4);

    Result res1 = pool.submitTask(std::make_shared<MyTask>(1,10000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(10001,20000)); 
    Result res3 = pool.submitTask(std::make_shared<MyTask>(20001,30000));
    pool.submitTask(std::make_shared<MyTask>(20001,30000));
    pool.submitTask(std::make_shared<MyTask>(20001,30000));
    pool.submitTask(std::make_shared<MyTask>(20001,30000));

    uLong sum1 = res1.get().cast_<uLong>();
    uLong sum2 = res2.get().cast_<uLong>();
    uLong sum3 = res3.get().cast_<uLong>();

    std::cout << sum1 + sum2 + sum3 << std::endl;
    uLong sum = 0;
    for(int i = 1;i < 30001;++i)
    {
        sum += i;
    }
    std::cout << sum << std::endl;
    getchar();
    
}