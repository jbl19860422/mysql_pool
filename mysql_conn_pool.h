#ifndef MYSQL_CONN_POOL_H_
#define MYSQL_CONN_POOL_H_
#include <memory>
#include <mutex>
#include <utility>
#include <atomic>
#include <chrono>
#include <type_traits>
#include <condition_variable>
#include "db_table.h"
#include "mysql/mysql_driver.h"
#include "mysql/cppconn/prepared_statement.h"
template<typename DB>
class MySqlConnPool;

template<typename DB>
class MySqlConn {
public:
    MySqlConn(std::shared_ptr<DB> db, std::weak_ptr<MySqlConnPool<DB>> pool) {
        db_ = db;
        weak_pool_ = pool;
    }
    
    virtual ~MySqlConn() {
        std::shared_ptr<MySqlConnPool<DB>> shr_pool_ = weak_pool_.lock();
        if(shr_pool_) {
            shr_pool_->recycleConnDB(db_);
        }
    }

    std::shared_ptr<Table> getTable(const std::string &table_name) {
        return db_->getTable(table_name);
    }
private:
    std::shared_ptr<DB> db_;
    std::weak_ptr<MySqlConnPool<DB>> weak_pool_;
};

template<typename DB>
class MySqlConnPool : public std::enable_shared_from_this<MySqlConnPool<DB>> {
public:
    using DB_PTR = std::shared_ptr<MySqlConn<DB>>;
    MySqlConnPool();
    ~MySqlConnPool();
    int init(size_t init_count, size_t max_count);
    void uninit();

    MySqlConnPool& operator=(const MySqlConnPool&) = delete;
    MySqlConnPool& operator=(const MySqlConnPool&) volatile = delete;
    /*
    * @fun:从连接池获取一个连接
    * @return nullptr：获取不到，可能是超时；非nullptr：正确；
    */
    DB_PTR getConnDB();
    /*
    * @fun:回收一个连接
    * @param[in] db 
    */
    void recycleConnDB(std::shared_ptr<DB> db);

    /*
    * @fun:连接断开回调
    * @parm:回调的连接对象
    */
    void onConnDisconnect(DB *db);
private:
    std::shared_ptr<std::thread> recycle_thread_;
    std::recursive_mutex db_list_mutex_;
    std::list<std::shared_ptr<DB>> db_list_;
    size_t init_count_;
    size_t curr_count_;
    size_t max_count_;

    std::atomic<bool> exit_atm_;
    std::mutex exit_mutex_;
    std::condition_variable exit_cv_;

    bool initialized_ = false;
private:
    void recycleThread();
};

template<typename DB>
MySqlConnPool<DB>::MySqlConnPool()
{
    exit_atm_ = false;
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
void MySqlConnPool<DB>::onConnDisconnect(DB *db)
{
    if(exit_atm_) {//如果是退出情况下的删除，不用再处理了
        return;
    }

    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    curr_count_--;
    if(curr_count_ < init_count_) {//比初始值小，忘记归还
        std::shared_ptr<DB> new_db = std::make_shared<DB>();
        new_db->onDisconnect(std::bind(&MySqlConnPool::onConnDisconnect, this, std::placeholders::_1));
        if(0 == new_db->connect()) {
            db_list_.emplace_back(std::move(new_db));
        }
    }
}

template<typename DB>
void MySqlConnPool<DB>::uninit()
{
    if(!initialized_) {
        return;
    }

    exit_atm_ = true;

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
typename MySqlConnPool<DB>::DB_PTR MySqlConnPool<DB>::getConnDB()
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
            std::weak_ptr<MySqlConnPool<DB>> weak_pool(this->shared_from_this());
            std::shared_ptr<MySqlConn<DB>> db_wrapper = std::make_shared<MySqlConn<DB>>(db, weak_pool);
            db_list_.pop_front();
            return db_wrapper;
        }
    }
    
    if(need_add) {
        std::shared_ptr<DB> db = std::make_shared<DB>();
        db->onDisconnect(std::bind(&MySqlConnPool::onConnDisconnect, this, std::placeholders::_1));
        if(0 == db->connect()) {
            std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
            std::weak_ptr<MySqlConnPool<DB>> weak_pool(this->shared_from_this());
            std::shared_ptr<MySqlConn<DB>> db_wrapper = std::make_shared<MySqlConn<DB>>(db, weak_pool);
            return db_wrapper;
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

    if(std::count_if(db_list_.begin(), db_list_.end(), [=](std::shared_ptr<DB> d) {
        return d.get() == db.get();
    }) <= 0) {
        db_list_.emplace_back(std::move(db));
    }
}

template<typename DB>
void MySqlConnPool<DB>::recycleThread()
{
    while(1) {
        std::unique_lock<std::mutex> lck(exit_mutex_);
        if(exit_cv_.wait_for(lck, std::chrono::seconds(10)) == std::cv_status::timeout) {//10秒钟回收
            std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
            if(curr_count_ > init_count_ && db_list_.size() > 0) {//回收超过的
                int recy_count = curr_count_ - init_count_ - 3;//保持3个吧
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