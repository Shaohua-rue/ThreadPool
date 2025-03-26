#pragma once
#include <memory>

//Any 类型：可以接收任意数据类型
class Any 
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

    //这个构造函数，可以接收任意数据类型
    template<typename T>
    Any(T data):base_(std::make_unique<Derive<T>>(data)){}

    //将Any对象里面存储的data数据提取出来
    template<typename T>
    T  cast_()
    {
        //从基类base_找到它所指向的Derive<T>对象，从它里面取出data成员变量
        Derive<T>* p = dynamic_cast<Derive<T>*>(base_.get());
        if (p == nullptr)
        {
            throw std::logic_error("type is unmatch");
            
        }
        return p->data_;
    }
private:
    //基类类型
    class Base
    {
        public:
            virtual ~Base(){}
    };
    //派生类类型
    template<typename T>
    class Derive:public Base
    {
        public:
           Derive(T data):data_(data){}
           T data_;
    };
private:
    //定义一个基类的指针
    std::unique_ptr<Base> base_;
};