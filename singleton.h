#ifndef _SINGLE_TO_H_
#define _SINGLE_TO_H_
#include <memory>
template<typename T>
class SingleTon {
public:
    template<typename... ARGS>
    static std::shared_ptr<T> getInstance(ARGS... args) {
        if(instance_ == nullptr) {
            instance_ = std::shared_ptr<T>(new T(std::forward<ARGS>(args)...));
        }
        return instance_;
    }
    virtual ~SingleTon();
private:
    SingleTon();
    static std::shared_ptr<T> instance_;
};

template<typename T>
SingleTon<T>::~SingleTon()
{

}

template<typename T>
std::shared_ptr<T> SingleTon<T>::instance_ = nullptr;

#endif