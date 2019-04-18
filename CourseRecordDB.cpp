#include "CourseRecordDB.h"
#include "logger.h"
#include "config.h"
CourseRecordDB::CourseRecordDB() {

}

CourseRecordDB::~CourseRecordDB() {
    disconnect();
}

void CourseRecordDB::disconnect()
{
    if(con_) {
        con_.reset();
    }

    if(driver_) {
        driver_.reset();
    }

    if(disconnect_cb_) {
        (*disconnect_cb_)(shared_from_this());   
    }
}

void CourseRecordDB::onDisconnect(const DISCONNECT_CB &cb) {
    disconnect_cb_ = std::make_shared<DISCONNECT_CB>(cb);
}

int CourseRecordDB::connect() {
    try {
        driver_ .reset(sql::mysql::get_mysql_driver_instance());
        sql::ConnectOptionsMap connection_properties;
        connection_properties["hostName"] = Config::getInstance()->mysql_hostname;
        connection_properties["userName"] = Config::getInstance()->mysql_username;
        connection_properties["password"] = Config::getInstance()->mysql_password;
        connection_properties["schema"] = MYSQL_DBNAME;
        connection_properties["port"] = Config::getInstance()->mysql_port;
        connection_properties["OPT_RECONNECT"] = true;
        con_ .reset(driver_->connect(connection_properties));
        if(!con_->isValid()) {
            con_.reset();
            driver_.reset();
            return -1;
        }
        LOG4J(INFO, "connect mysql succeed.");
        return 0;
    } catch(sql::SQLException &e) {
        LOG4J(ERROR, "error connect mysql code=" << e.getErrorCode() << ",connection_info:host=" << Config::getInstance()->mysql_hostname << ",port=" << Config::getInstance()->mysql_port << ",username=" << Config::getInstance()->mysql_username << ",password=" << Config::getInstance()->mysql_password);
        return -2;
    }
}

int CourseRecordDB::queryByStreamId(const std::string &stream_id, T_CourseRecord& record) {
   
}
