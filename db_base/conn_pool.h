#ifndef MYSQL_CONN_POOL_H_
#define MYSQL_CONN_POOL_H_
#include <memory>
#include <mutex>
#include <utility>
#include <atomic>
#include <iostream>

#include <chrono>
#include <type_traits>
#include <condition_variable>
#include "db_table.h"
#include "mysql/mysql_driver.h"
#include "mysql/cppconn/prepared_statement.h"
template <typename DB>
class ConnPool;

template <typename DB>
class Conn
{
  public:
    Conn(std::shared_ptr<DB> db, std::weak_ptr<ConnPool<DB>> pool)
    {
        db_ = db;
        weak_pool_ = pool;
    }

    Conn(const Conn<DB> &conn) = delete;//禁止
    Conn(Conn<DB> &conn)
    { 
        weak_pool_ = conn.weak_pool_;
        db_ = conn.db_;
        conn.db_ = nullptr;
        conn.weak_pool_.reset();
    }

    Conn(const Conn<DB> &&conn) = delete;
    Conn(Conn<DB> &&conn)
    {
        weak_pool_ = conn.weak_pool_;
        db_ = conn.db_;
        conn.db_ = nullptr;
        conn.weak_pool_.reset();
    }

    Conn<DB> &operator=(const Conn<DB> &conn) = delete;
    Conn<DB> &operator=(Conn<DB> &conn)
    {
        weak_pool_ = conn.weak_pool_;
        db_ = conn.db_;
        conn.db_ = nullptr;
        conn.weak_pool_.reset();
        return *this;
    }

    Conn<DB> &operator=(const Conn<DB> &&conn) = delete;
    Conn<DB> &operator=(Conn<DB> &&conn)
    {
        weak_pool_ = conn.weak_pool_;
        db_ = conn.db_;
        conn.db_ = nullptr;
        conn.weak_pool_.reset();
        return *this;
    }

    Conn() = default;

    virtual ~Conn()
    {
        if (weak_pool_.use_count() > 0)
        {
            std::shared_ptr<ConnPool<DB>> shr_pool_ = weak_pool_.lock();
            if (shr_pool_)
            {
                if (db_)
                {
                    shr_pool_->recycleConn(db_);
                }
            }
        }
    }

    std::shared_ptr<DB> operator->()
    {
        return db_;
    }

    operator bool()
    {
        return db_ != nullptr;
    }

  private:
    std::shared_ptr<DB> db_ = nullptr;
    std::weak_ptr<ConnPool<DB>> weak_pool_;
};

template <typename DB>
class ConnPool : public std::enable_shared_from_this<ConnPool<DB>>
{
  public:
    using DB_PTR = std::shared_ptr<Conn<DB>>;
    ConnPool();
    ~ConnPool();

    int addConn(std::shared_ptr<DB> conn);
    void uninit();

    void removeConn(const std::function<bool(std::shared_ptr<DB> conn)> remove_fun);

    bool findConn(const std::function<bool(std::shared_ptr<DB> conn)> search_fun)
    {
        std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
        for(typename std::list<std::shared_ptr<DB>>::iterator it = db_list_.begin(); it != db_list_.end();it++)
        {
            if(search_fun(*it)) {
                return true;
            }
        }
        return false;
    }

    ConnPool &operator=(const ConnPool &) = delete;
    ConnPool &operator=(const ConnPool &) volatile = delete;

    Conn<DB> getConn();
    /*
    * @fun:回收一个连接
    * @param[in] db 
    */
    void recycleConn(std::shared_ptr<DB> db);
  private:
    std::recursive_mutex db_list_mutex_;
    std::list<std::shared_ptr<DB>> db_list_;

    std::atomic<bool> exit_atm_;
    bool initialized_ = false;

};

template <typename DB>
ConnPool<DB>::ConnPool()
{
}

template <typename DB>
ConnPool<DB>::~ConnPool()
{
    uninit();
}

template <typename DB>
int ConnPool<DB>::addConn(std::shared_ptr<DB> db)
{   
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    db_list_.emplace_back(db);
}

template <typename DB>
void ConnPool<DB>::removeConn(const std::function<bool(std::shared_ptr<DB> conn)> remove_fun) {
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    for(typename std::list<std::shared_ptr<DB>>::iterator it = db_list_.begin(); it != db_list_.end();)
    {
        if(remove_fun(*it)) {
            db_list_.erase(it++);
        } else {
            it++;
        }
    }
}

template <typename DB>
void ConnPool<DB>::uninit()
{
    if (!initialized_)
    {
        return;
    }

    exit_atm_ = true;

    {
        std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
        db_list_.clear();
    }
}

template <typename DB>
Conn<DB> ConnPool<DB>::getConn()
{
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);
    if (db_list_.size() <= 0)
    {
        return Conn<DB>();
    }
    else
    {
        std::shared_ptr<DB> db = db_list_.front();
        std::weak_ptr<ConnPool<DB>> weak_pool(this->shared_from_this());
        Conn<DB> db_wrapper(db, weak_pool);
        db_list_.pop_front();
        return db_wrapper;
    }
}

template <typename DB>
void ConnPool<DB>::recycleConn(std::shared_ptr<DB> db) //如果忘了归还，外面析构掉？
{
    if (!db)
    {
        return;
    }
    std::lock_guard<std::recursive_mutex> lck(db_list_mutex_);

    if (std::count_if(db_list_.begin(), db_list_.end(), [=](std::shared_ptr<DB> d) {
            return d.get() == db.get();
        }) <= 0)
    {
        db_list_.emplace_back(std::move(db));
    }
}

#endif