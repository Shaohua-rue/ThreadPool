#pragma once
/** 
* @brief noncopyable被继承后，派生类对象可以正常构造与析构，但不能拷贝构造与赋值构造
*/
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;  //禁止拷贝构造
    noncopyable &operator=(const noncopyable &) = delete;   //禁止赋值构造
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};