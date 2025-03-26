#include "task.h"
/////////////////  Task相关成员函数
Task::Task()
	: result_(nullptr)
{}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run()); // 执行run
	}
}

void Task::setResult(Result* res)
{
	result_ = res;
}

/////////////////   Result构造函数
Result::Result(std::shared_ptr<Task> task, bool isValid)
	: isValid_(isValid)
	, task_(task)
{
	task_->setResult(this);
}

Any Result::get() // 用户获得任务结果，返回any类型
{
	if (!isValid_)
	{
		return "";
	}
	sem_.wait(); //等待线程task执行完毕，获得任务结果
	return std::move(any_); 
}

void Result::setVal(Any any)  // 设置任务结果
{
	this->any_ = std::move(any);
	sem_.post(); // 通知线程task执行完毕，可以获得任务结果
}