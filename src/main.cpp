// 线程池项目-最终版.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <functional>
#include <thread>
#include <future>
#include <chrono>
using namespace std;

#include "threadpool.hpp"


/*
如何能让线程池提交任务更加方便
1. pool.submitTask(sum1, 10, 20);
   pool.submitTask(sum2, 1 ,2, 3);
   submitTask:可变参模板编程

2. 我们自己造了一个Result以及相关的类型，代码挺多
    C++11 线程库   thread   packaged_task(function函数对象)  async 
   使用future来代替Result节省线程池代码
*/

int sum1(int a, int b)
{
    this_thread::sleep_for(chrono::seconds(3));
    // 比较耗时
    return a + b;
}
int sum2(int a, int b, int c)
{
    this_thread::sleep_for(chrono::seconds(2));
    return a + b + c;
}
// io线程 
void io_thread(int listenfd)
{

}
// worker线程
void worker_thread(int clientfd)
{

}

// Simple function that adds multiplies two numbers and returns the result
int multiply1(const int a, const int b)
{
    const int res = a * b;
    return res;
}

void  multiply2(int& out_res, const int a, const int b) {
	out_res = a * b;
}


int main()
{
    //创建线程池
    ThreadPool pool;

    //设置线程池模式
    pool.setMode(PoolMode::MODE_CACHED);

    //开启线程池，指定其数量
    pool.start(2);

    future<int> r1 = pool.submitTask(sum1, 1, 2);
    future<int> r2 = pool.submitTask(sum2, 1, 2, 3);
    future<int> r3 = pool.submitTask([](int b, int e)->int {
        int sum = 0;
        for (int i = b; i <= e; i++)
            sum += i;
        return sum;
        }, 1, 100);
    future<int> r4 = pool.submitTask([](int b, int e)->int {
        int sum = 0;
        for (int i = b; i <= e; i++)
            sum += i;
        return sum;
        }, 1, 100);
    future<int> r5 = pool.submitTask([](int b, int e)->int {
        int sum = 0;
        for (int i = b; i <= e; i++)
            sum += i;
        return sum;
        }, 1, 100);
    //future<int> r4 = pool.submitTask(sum1, 1, 2);
    future<int> r6 = pool.submitTask(multiply1,2,5);
    int res = 0;
    auto r7 = pool.submitTask(multiply2,ref(res),1,2);


    cout << r1.get() << endl;
    cout << r2.get() << endl;
    cout << r3.get() << endl;
    cout << r4.get() << endl;
    cout << r5.get() << endl;
    cout << r6.get() << endl; 
    r7.get(); 
    cout << "result " << res <<endl;

    pool.shutdown();

    getchar();

    //packaged_task<int(int, int)> task(sum1);
    //// future <=> Result
    //future<int> res = task.get_future();
    //// task(10, 20);
    //thread t(std::move(task), 10, 20);
    //t.detach();

    //cout << res.get() << endl;

    /*thread t1(sum1, 10, 20);
    thread t2(sum2, 1, 2, 3);

    t1.join();
    t2.join();*/
}