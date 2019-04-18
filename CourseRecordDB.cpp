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
    if(!con_) {
        return -1;
    }
    
    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt;
        pstmt.reset(con_->prepareStatement("SELECT * FROM CourseRecord WHERE stream_id=?"));
        pstmt->setString(1, stream_id);
        std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if(res->next()) {
            record.stream_id = res->getString("stream_id");
            record.uid = res->getUInt64("uid");
            record.sid = res->getUInt64("sid");
            record.task_id = res->getString("task_id");
            record.status = res->getUInt("status");
            record.start_timestamp = res->getUInt("start_timestamp");
            record.stop_timestamp = res->getUInt("stop_timestamp");
            record.gen_video = res->getUInt("gen_video");
            return 1;
        }
        return -2;
    } catch(sql::SQLException &e) {
        LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -3;
    }
}

int CourseRecordDB::queryByTaskId(const std::string &task_id, T_CourseRecord& record) {
    if(!con_) {
        return -1;
    }
    
    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt;
        pstmt.reset(con_->prepareStatement("SELECT * FROM CourseRecord WHERE task_id=?"));
        pstmt->setString(1, task_id);
        std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
        if(res->next()) {
            record.stream_id = res->getString("stream_id");
            record.uid = res->getUInt64("uid");
            record.sid = res->getUInt64("sid");
            record.task_id = res->getString("task_id");
            record.status = res->getUInt("status");
            record.start_timestamp = res->getUInt("start_timestamp");
            record.stop_timestamp = res->getUInt("stop_timestamp");
            record.gen_video = res->getUInt("gen_video");
            return 1;
        }
        return -2;
    } catch(sql::SQLException &e) {
        LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -3;
    }
}

int CourseRecordDB::queryAll(std::vector<T_CourseRecord> &records) {
    if(!con_) {
        return -1;
    }
    
    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt;
        pstmt.reset(con_->prepareStatement("SELECT * FROM CourseRecord"));
        std::shared_ptr<sql::ResultSet> res(pstmt->executeQuery());
        while(res->next()) {
            T_CourseRecord record;
            record.stream_id = res->getString("stream_id");
            record.uid = res->getUInt64("uid");
            record.sid = res->getUInt64("sid");
            record.task_id = res->getString("task_id");
            record.status = res->getUInt("status");
            record.start_timestamp = res->getUInt("start_timestamp");
            record.stop_timestamp = res->getUInt("stop_timestamp");
            record.gen_video = res->getUInt("gen_video");
            records.push_back(record);
        }
        return records.size();
    } catch(sql::SQLException &e) {
        LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -3;
    }
}

int CourseRecordDB::insertToCourseRecord(const T_CourseRecord& record) {
    if(!con_) {
        return -1;
    }

    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt(con_->prepareStatement("INSERT INTO CourseRecord(stream_id, uid, sid, task_id, status, start_timestamp, stop_timestamp) VALUES (?, ?, ?, ?, ?, ?, ?)"));
        pstmt->setString(1, record.stream_id);
        pstmt->setUInt64(2, record.uid);
        pstmt->setUInt64(3, record.sid);
        pstmt->setString(4, record.task_id);
        pstmt->setUInt(5, record.status);
        pstmt->setUInt(6, record.start_timestamp);
        pstmt->setUInt(7, record.stop_timestamp);
        return pstmt->executeUpdate();
    } catch(sql::SQLException &e) {
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
            con_->reconnect();
        } else if(e.getErrorCode() == 1062) {//duplicate key
            LOG4J(WARN, "error code=" << e.getErrorCode() << ".");
        } else {
            LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        }
        return -2;
    }
}

int CourseRecordDB::updateCourseRecord(const std::string& task_id, const T_CourseRecord &new_record) {
    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt(con_->prepareStatement("UPDATE CourseRecord SET uid=?,sid=?,stream_id=?,status=?,start_timestamp=?,stop_timestamp=?, gen_video=? where task_id=?"));
        pstmt->setUInt64(1, new_record.uid);
        pstmt->setUInt64(2, new_record.sid);
        pstmt->setString(3, new_record.stream_id);
        pstmt->setUInt(4, new_record.status);
        pstmt->setUInt(5, new_record.start_timestamp);
        pstmt->setUInt(6, new_record.stop_timestamp);
        pstmt->setUInt(7, new_record.gen_video);
        pstmt->setString(8, task_id);
        return pstmt->executeUpdate();
    } catch(sql::SQLException &e) {
		LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -2;
    }
    return 0;
}

int CourseRecordDB::insertToTsRecord(const T_TSRecord& record) {
    if(!con_) {
        LOG4J(ERROR, "connect mysql error.");
        return -1;
    }

    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt(con_->prepareStatement("INSERT INTO TsRecord(id, sort, appid, task_id, video_file_id, video_address, file_size,start_time, start_dts, duration, offset, sps_pps, tc, extension, status, timestamp) VALUES (?, ?, ?, ?, ?, ?,?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
        pstmt->setUInt64(1, record.id);
        pstmt->setUInt(2, record.sort);
        pstmt->setUInt(3, record.appid);
        pstmt->setString(4, record.task_id);
        pstmt->setUInt64(5, record.video_file_id);
        pstmt->setString(6, record.video_address);
        pstmt->setUInt(7, record.file_size);
        pstmt->setUInt64(8, record.start_time);
        pstmt->setUInt(9, record.start_dts);
        pstmt->setUInt(10, record.duration);
        pstmt->setUInt64(11, record.offset);
        pstmt->setString(12, record.sps_pps);
        pstmt->setUInt(13, record.tc);
        pstmt->setString(14, record.extension);
        pstmt->setUInt(15, record.status);
        pstmt->setUInt(16, record.timestamp);
        return pstmt->executeUpdate();
    } catch(sql::SQLException &e) {
		LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -2;
    }
}

 int CourseRecordDB::insertToBs2Record(const T_Bs2Record &record)
 {
     if(!con_) {
        return -1;
    }

    try {
        std::lock_guard<std::mutex> lck(con_mutex_);
        std::shared_ptr<sql::PreparedStatement> pstmt(con_->prepareStatement("INSERT INTO Bs2Record(file_id, appid, task_id, start_time, end_time, file_size, duration,file_name, state, source_ip, ext_info, upload_id, upload_zone, bucket_name, bs2_key, bs2_secret, timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?,?, ?, ?, ?,?, ?, ?, ?, ?)"));
        pstmt->setUInt64(1, record.file_id);
        pstmt->setUInt(2, record.appid);
        pstmt->setString(3, record.task_id);
        pstmt->setUInt64(4, record.start_time);
        pstmt->setUInt64(5, record.end_time);
        pstmt->setUInt64(6, record.file_size);
        pstmt->setUInt64(7, record.duration);
        pstmt->setString(8, record.file_name);
        pstmt->setUInt(9, record.state);
        pstmt->setString(10, record.source_ip);
        pstmt->setString(11, record.ext_info);
        pstmt->setUInt64(12, record.upload_id);
        pstmt->setString(13, record.upload_zone);
        pstmt->setString(14, record.bucket_name);
        pstmt->setString(15, record.bs2_key);
        pstmt->setString(16, record.bs2_secret);
        pstmt->setInt(17, record.timestamp);
        return pstmt->executeUpdate();
    } catch(sql::SQLException &e) {
		LOG4J(ERROR, "error code=" << e.getErrorCode() << ".");
        if(e.getErrorCode() == 2006 || e.getErrorCode() == 2013) {
            con_->reconnect();
        }
        return -2;
    }
 }
