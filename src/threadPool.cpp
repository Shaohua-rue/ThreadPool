#include"threadPool.h"
const size_t TASK_MAX_THRESHOLD = 1024; //任务队列最大阈值
const int MAX_THREAD_SIZE = 100;        //线程池最大线程数量
const int THREAD_MAX_IDIE_TIME = 10;    //空闲线程最大空闲时间，单位s
int Thread::generateId_ = 0;
ThreadPool::ThreadPool():taskSize_(0),
taskQueMaxThreshold_(1024),
idleThreadSize_(0),
curThreadSize_(0),
poolMode_(PoolMode::MODE_FIXED),
isPoolRunning_(false){}


ThreadPool::~ThreadPool(){
    isPoolRunning_ = false;
    

    //等待线程池里面所有线程返回，有两种状态：阻塞 & 正在执行任务中
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();

    exitCond_.wait(lock,[&](){return taskQue_.empty() && threads_.empty();});

}


//开启线程池
void ThreadPool::start(int initThreadSize)
{
    // 设置线程池状态为运行状态
    isPoolRunning_ = true;

    initThreadSize_ = initThreadSize;
    curThreadSize_ = initThreadSize;

    //创建线程对象
    for(int i = 0; i < initThreadSize; ++i)
    {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
        int id = ptr->getId();
        threads_.emplace(id,std::move(ptr));
        //threads_.emplace_back(std::move(ptr));
    }

    //启动线程
    for(int i = 0; i < initThreadSize; ++i)
    {
        threads_[i]->start();   //需要去执行一个线程函数
        idleThreadSize_++;      //记录初始空闲线程的数量
    }
}
//线程函数
void ThreadPool::threadFunc(int threadId)
{
    auto lastTime = std::chrono::high_resolution_clock::now();

    //所有的任务必须全部执行完成，线程池才可以回收所有资源
    for(;;)
    {
        std::shared_ptr<Task> task;
        {
            //先获取锁
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            //等待notEmpty条件变量，如果有任务，就执行任务，没有任务就阻塞等待
            std::cout<<"tid:"<<std::this_thread::get_id()<<"尝试获取任务..."<<std::endl;

            //cashed模式下，可能创建了很多线程，但是空闲时间超过60s，应该把多余的线程结束回收掉(超过initThreadSize_数量的线程要回收)
            //当前时间 - 上一次线程执行的时间 > 60s
            while (taskQue_.size() == 0)
			{
				// 线程池要结束，回收线程资源
				if (!isPoolRunning_)
				{
					threads_.erase(threadId); // std::this_thread::getid()
					std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
						<< std::endl;
					exitCond_.notify_all();
					return; // 线程函数结束，线程结束
				}

				if (poolMode_ == PoolMode::MODE_CACHED)
				{
					// 条件变量，超时返回了
					if (std::cv_status::timeout ==
						notEmpty_.wait_for(lock, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDIE_TIME
							&& curThreadSize_ > initThreadSize_)
						{
							// 开始回收当前线程
							// 记录线程数量的相关变量的值修改
							// 把线程对象从线程列表容器中删除   没有办法 threadFunc《=》thread对象
							// threadid => thread对象 => 删除
							threads_.erase(threadId); // std::this_thread::getid()
							curThreadSize_--;
							idleThreadSize_--;

							std::cout << "threadid:" << std::this_thread::get_id() << " delete!"
								<< std::endl;
							return;
						}
					}
				}
				else
				{
					// 等待notEmpty条件
					notEmpty_.wait(lock);
				}
                if (!isPoolRunning_)
				{
					threads_.erase(threadId); // std::this_thread::getid()
					std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
						<< std::endl;
					exitCond_.notify_all();
					return; // 结束线程函数，就是结束当前线程了!
				}

            }

            
            idleThreadSize_--;
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
            task->exec();  
        }
        idleThreadSize_++;
        auto curTime = std::chrono::high_resolution_clock::now();  //更新线程调度执行完的时间
       
          
    }
}
//给线程池提交任务 用户调用改接口，传入任务对象，生产任务
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
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
        return Result(sp,false);
    }


    //如果有空余，把任务放到任务队列中
    taskQue_.emplace(sp);
    taskSize_++; 
    //因为新放置任务，任务队列不空，notEmpty条件变量通知线程池中的线程，有任务了  
    notEmpty_.notify_all();   

    //cached模式 任务处理比较紧急 场景：小而快的任务 需要根据任务数量和空闲线程的数量判断是否增加线程
    if(poolMode_ == PoolMode::MODE_CACHED)
    {
        //如果当前任务数量大于空闲线程的数量，则创建新线程
        if(taskSize_ > idleThreadSize_ && curThreadSize_ < MAX_THREAD_SIZE )
        {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
            int id = ptr->getId();
            threads_.emplace(id,std::move(ptr));

            //启动线程
            threads_[id]->start();
            
            //修改线程个数相关的变量
            curThreadSize_++;
            idleThreadSize_++;
        }
    }

    //返回任务的Result对象
    return Result(sp);
}
