#pragma once
#include <memory>
#include <atomic>
#include <iostream>
#include "any.h"
#include "semaphore.h"

// Task的前置声明
class Task;

// ʵ定义返回值类
class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// 线程设置返回值
	void setVal(Any any);

	// 客户获得线程返回值
	Any get();
private:
	Any any_; // Any类接收
	Semaphore sem_; // 信号量接受
	std::shared_ptr<Task> task_; //ָ线程任务绑定 
	std::atomic_bool isValid_; // 返回值是否有效
};

class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	//纯虚函数，子类继承必须实现run()函数
	virtual Any run() = 0;

private:
	Result* result_; //任务返回值
};