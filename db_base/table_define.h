#ifndef _TYPE_DEF_H_
#define _TYPE_DEF_H_
#include <tuple>
#include <iostream>
#include <sstream>
#include <boost/any.hpp>
#include "json/json.h"

enum
{
	RECORD_STAUS_INIT = 0,
	RECORD_STATUS_START = 1,
	RECORD_STATUS_START_SUCCESS = 2,
	RECORD_STATUS_STOP = 3,
	RECORD_STATUS_STOP_SUCCESS = 4,

	RECORD_STATUS_START_ERROR = -2,
	RECORD_STATUS_STOP_ERROR = -4,
	RECORD_STATUS_CONVERT_ERROR = -5,
};

struct T_TaskRecord
{ //录制任务记录格式
	uint32_t id;
	std::string stream_id;
	uint64_t uid;
	uint64_t sid;
	uint32_t channel_id;
	std::string audio_stream_name;
	std::string video_stream_name;
	uint32_t live_type; //0
	std::string task_id;
	int32_t status;			  //0：未开始，1：开始，2：失败，其他：未定义
	uint32_t start_by_switch; //0：非切换原因，1：切换生成的任务
	uint32_t stop_by_switch;
	uint64_t start_timestamp;	//操作时间戳
	uint64_t stop_timestamp;	 //操作时间戳
	std::string yy_record_file;  //yy录制生成的MP4文件
	uint32_t yy_gen_mp4;		 //yy是否生成了Mp4
	std::string ago_record_path; //声网生成文件路径
	std::string ago_bs2_file;	//上传到bs2上的文件
	uint32_t ago_gen_mp4;		 //是否合成完mp4并上传bs2
	uint32_t ago_gen_time;		 //声网合成完成时间
	std::string record_server_ip;//录制任务执行的服务器ip，用于判断声网的任务在本机

	T_TaskRecord()
	{
	}

	operator std::string() const 
	{
		std::ostringstream oss;
		oss << "id=" << id << "&stream_id=" << stream_id << "&uid=" << uid << "&sid=" << sid 
			<< "&channel_id=" << channel_id << "&live_type=" << live_type << "&task_id=" << task_id
			<< "&status=" << status;
		return oss.str();
	}

	T_TaskRecord(const std::shared_ptr<sql::ResultSet> &res)
	{
		id = res->getUInt("id");
		stream_id = res->getString("stream_id");
		uid = res->getUInt64("uid");
		sid = res->getUInt64("sid");
		channel_id = res->getUInt("channel_id");
		audio_stream_name = res->getString("audio_stream_name");
		video_stream_name = res->getString("video_stream_name");
		live_type = res->getUInt("live_type");
		task_id = res->getString("task_id");
		status = res->getInt("status");
		start_by_switch = res->getUInt("start_by_switch");
		stop_by_switch = res->getUInt("stop_by_switch");
		start_timestamp = res->getUInt64("start_timestamp");
		stop_timestamp = res->getUInt64("stop_timestamp");
		yy_record_file = res->getString("yy_record_file");
		yy_gen_mp4 = res->getUInt("yy_gen_mp4");
		ago_record_path = res->getString("ago_record_path");
		ago_bs2_file = res->getString("ago_bs2_file");
		ago_gen_mp4 = res->getUInt("ago_gen_mp4");
		ago_gen_time = res->getUInt("ago_gen_time");
		record_server_ip = res->getString("record_server_ip");
	}

	T_TaskRecord &operator=(const std::shared_ptr<sql::ResultSet> &res)
	{
		id = res->getUInt("id");
		stream_id = res->getString("stream_id");
		uid = res->getUInt64("uid");
		sid = res->getUInt64("sid");
		channel_id = res->getUInt("channel_id");
		audio_stream_name = res->getString("audio_stream_name");
		video_stream_name = res->getString("video_stream_name");
		live_type = res->getUInt("live_type");
		task_id = res->getString("task_id");
		status = res->getInt("status");
		start_by_switch = res->getUInt("start_by_switch");
		stop_by_switch = res->getUInt("stop_by_switch");
		start_timestamp = res->getUInt64("start_timestamp");
		stop_timestamp = res->getUInt64("stop_timestamp");
		yy_record_file = res->getString("yy_record_file");
		yy_gen_mp4 = res->getUInt("yy_gen_mp4");
		ago_record_path = res->getString("ago_record_path");
		ago_bs2_file = res->getString("ago_bs2_file");
		ago_gen_mp4 = res->getUInt("ago_gen_mp4");
		ago_gen_time = res->getUInt("ago_gen_time");
		record_server_ip = res->getString("record_server_ip");
		return *this;
	}
};

struct T_CourseRecord
{ //录制任务记录格式
	uint32_t id;
	uint64_t uid;
	uint64_t sid;
	int32_t status;
	uint32_t channel_id;
	std::string mp4_file;
	int32_t create_timestamp;
	int32_t update_timestamp;
};

struct T_TSRecord
{
	int64_t id;
	uint32_t sort;
	uint32_t appid;
	std::string task_id;
	int64_t video_file_id;
	std::string video_address;
	uint32_t file_size;
	int64_t start_time;
	uint32_t start_dts;
	uint32_t duration;
	uint64_t offset;
	std::string sps_pps;
	uint32_t tc;
	std::string extension;
	uint32_t status;
	uint32_t timestamp;
};

struct T_Bs2Record
{
	int64_t file_id;
	uint32_t appid;
	std::string task_id;
	int64_t start_time;
	int64_t end_time;
	int64_t file_size;
	int64_t duration;
	std::string file_name;
	uint32_t state;
	std::string source_ip;
	std::string ext_info;
	int64_t upload_id;
	std::string upload_zone;
	std::string bucket_name;
	std::string bs2_key;
	std::string bs2_secret;
	int32_t timestamp;
};

#endif
