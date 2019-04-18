#ifndef _SINGLE_TO_H_
#define _SINGLE_TO_H_

template<typename T>
class SingleTon {
public:
    static T *getInstance();
    virtual ~SingleTon();
private:
    SingleTon();
    static T *instance_;
};

template<typename T>
T *SingleTon<T>::getInstance()
{
    if(instance_ == nullptr) {
        instance_ = new T;
    }
    return instance_;
}

template<typename T>
SingleTon<T>::~SingleTon()
{

}

template<typename T>
T* SingleTon<T>::instance_ = nullptr;

#endif