#ifndef MYSQL_CONN_POOL_H_
#define MYSQL_CONN_POOL_H_
#include <memory>
#include <mutex>
#include <utility>
#include <chrono>
#include <type_traits>
#include <condition_variable>
#include "mysql/mysql_driver.h"
#include "mysql/cppconn/prepared_statement.h"
template<typename DB>
class MySqlConnPool {
public:
    MySqlConnPool();
    ~MySqlConnPool();
    int init(size_t init_count, size_t max_count);
    void uninit();
    /*
    * @fun:从连接池获取一个连接
    * @return nullptr：获取不到，可能是超时；非nullptr：正确；
    */
    std::shared_ptr<DB> getConnDB();
    /*
    * @fun:回收一个连接
    * @param[in] db 
    */
    void recycleConnDB(std::shared_ptr<DB> db);

    /*
    * @fun:连接断开回调
    * @parm:回调的连接对象
    */
    void onConnDisconnect(std::shared_ptr<DB> db);
private:
    std::shared_ptr<std::thread> recycle_thread_;
    std::recursive_mutex db_list_mutex_;
    std::list<std::shared_ptr<DB>> db_list_;
    size_t init_count_;
    size_t curr_count_;
    size_t max_count_;

    std::mutex exit_mutex_;
    std::condition_variable exit_cv_;
    bool initialized_ = false;
private:
    void recycleThread();
};

template<typename DB>
MySqlConnPool<DB>::MySqlConnPool()
{

}

template<typename DB>
MySqlConnPool<DB>::~MySqlConnPool()
{
    uninit();
}


template<typename DB>
int MySqlConnPool<DB>::init(size_t init_count, size_t max_count)
{
    assert(max_count > init_count);
    init_count_ = init_count;
    max_count_ = max_count_;
    
    {
        std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
        for(size_t i = 0; i < init_count_; i++) {
            std::shared_ptr<DB> db = std::make_shared<DB>();
            db->onDisconnect(std::bind(&MySqlConnPool::onConnDisconnect, this, std::placeholders::_1));
            if(0 == db->connect()) {
                db_list_.emplace_back(std::move(db));
            } else {
                db_list_.clear();
                return -1;
            }
        }
        curr_count_ = init_count_;
    }
    //启动定时回收线程
    recycle_thread_ = std::make_shared<std::thread>(std::bind(&MySqlConnPool::recycleThread, this));

    initialized_ = true;
    return 0;
}

template<typename DB>
void MySqlConnPool<DB>::onConnDisconnect(std::shared_ptr<DB> db)//对于那种外部完了归还的情况，这里处理
{
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    curr_count_--;
    if(curr_count_ < init_count_) {//比初始值小，忘记归还
        std::shared_ptr<DB> db = std::make_shared<DB>();
        db->onDisconnect(std::bind(&MySqlConnPool::onConnDisconnect, this, std::placeholders::_1));
        if(0 == db->connect()) {
            db_list_.emplace_back(std::move(db));
        }
    }
}

template<typename DB>
void MySqlConnPool<DB>::uninit()
{
    if(!initialized_) {
        return;
    }

    {
        std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
        db_list_.clear();
        curr_count_ = 0;
    }

     if(recycle_thread_) {
        exit_cv_.notify_one();
        recycle_thread_->join();
    }
}

template<typename DB>
std::shared_ptr<DB> MySqlConnPool<DB>::getConnDB()
{
    bool need_add = false;
    {
        std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
        if(db_list_.size() <= 0) {
            if(curr_count_ >= max_count_) {
                return nullptr;
            }
            need_add = true;
        } else {
            std::shared_ptr<DB> db = db_list_.front();
            db_list_.pop_front();
            return db;
        }
    }
    
    if(need_add) {
        std::shared_ptr<DB> db = std::make_shared<DB>();
        db->onDisconnect(std::bind(&MySqlConnPool::onConnDisconnect, this, std::placeholders::_1));
        if(0 == db->connect()) {
            std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
            curr_count_++;
            return db;
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

template<typename DB>
void MySqlConnPool<DB>::recycleConnDB(std::shared_ptr<DB> db)//如果忘了归还，外面析构掉？
{
    if(!db) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    db_list_.emplace_back(std::move(db));
}

template<typename DB>
void MySqlConnPool<DB>::recycleThread()
{
    while(1) {
        std::unique_lock<std::mutex> lck(exit_mutex_);
        if(exit_cv_.wait_for(lck, std::chrono::seconds(10)) == std::cv_status::timeout) {//10秒钟回收
            std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
            if(curr_count_ > init_count_ && db_list_.size() > 0) {//回收超过的
                int recy_count = curr_count_ - init_count_;
                for(int i = 0; i < recy_count; i++) {
                    db_list_.pop_front();
                }
            }
        } else {
            break;
        }
    }
}


#endif