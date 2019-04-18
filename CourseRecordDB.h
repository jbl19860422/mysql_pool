#ifndef _COURSE_RECORD_DB_
#define _COURSE_RECORD_DB_
#include <memory>
#include <mutex>
#include "mysql/mysql_driver.h"
#include "mysql/cppconn/prepared_statement.h"
#include "table_define.h"

class CourseRecordDB : public std::enable_shared_from_this<CourseRecordDB> {
public:
    using DISCONNECT_CB = std::function<void(std::shared_ptr<CourseRecordDB>)>;
public:
    CourseRecordDB();
    virtual ~CourseRecordDB();
    int connect();
    void onDisconnect(const DISCONNECT_CB &cb);
    /*
        自己的函数
    */
    
private:
    void disconnect();
    std::shared_ptr<sql::mysql::MySQL_Driver> driver_;
    std::mutex con_mutex_;
	std::shared_ptr<sql::Connection> con_;
    std::shared_ptr<DISCONNECT_CB> disconnect_cb_ = nullptr;
};
#endif
