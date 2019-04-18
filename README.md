# mysql_pool
在mysql库的基础上，封装了线程池模板，
mysql_conn_pool.h：连接池模板；
CourseRecordDB.h：具体的数据库连接处理方法，需要实现connect及onDisconnect方法，可能还要把锁去掉；
main.cpp：简单的使用
