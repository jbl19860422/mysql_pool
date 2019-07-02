#ifndef DB_TABLE_H_
#define DB_TABLE_H_
#include <string>
#include <utility>
#include <type_traits>
#include <functional>
#include <iostream>
#include <boost/any.hpp>
#include "mysql/mysql_driver.h"
#include "mysql/cppconn/prepared_statement.h"
#include "boost/any.hpp"
#include "boost/algorithm/string/join.hpp"

enum E_QUERY_CONNECTOR
{
    E_QUERY_AND = 0,
    E_QUERY_OR = 1
};

enum E_MYSQL_OP
{ //CURD
    E_OP_NONE = 0,
    E_OP_SELECT = 1,
    E_OP_UPDATE = 2,
    E_OP_INSERT = 3,
    E_OP_DELETE = 4
};

class Query
{
  public:
    Query()
    {
    }
    operator std::string() const
    {
        std::string s;
        for (auto it = queries_.begin(); it != queries_.end(); it++)
        {
            auto c = std::get<0>(*it);
            if (c == E_QUERY_AND)
            {
                if (it == queries_.begin())
                {
                    s += std::get<1>(*it);
                }
                else
                {
                    s += " AND " + std::get<1>(*it);
                }
            }
            else if (c == E_QUERY_OR)
            {
                if (it == queries_.begin())
                {
                    s += std::get<1>(*it);
                }
                else
                {
                    s += " OR " + std::get<1>(*it);
                }
            }
        }
        return s;
    }

    Query(E_QUERY_CONNECTOR c, const std::string &s)
    {
        queries_.emplace_back(std::make_tuple(c, std::move(s)));
    }

    void addQuery(const Query &t)
    {
        queries_.emplace_back(std::make_tuple(E_QUERY_AND, (std::string)t));
    }

    template <typename T>
    Query &where(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + std::to_string(val);
        queries_.emplace_back(std::make_tuple(E_QUERY_AND, std::move(query)));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<char *, typename std::decay<T>::type>::value, Query &>::type //处理char*类参数
    where(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + "'" + std::string(val) + "'";
        queries_.emplace_back(std::make_tuple(E_QUERY_AND, std::move(query)));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, Query &>::type //处理std::string类参数
    where(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + val;
        queries_.emplace_back(std::make_tuple(E_QUERY_AND, std::move(query)));
        return *this;
    }

    template <typename T>
    Query &orWhere(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + std::to_string(val);
        queries_.emplace_back(std::make_tuple(E_QUERY_OR, std::move(query)));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<char *, typename std::decay<T>::type>::value, Query &>::type //处理char*类参数
    orWhere(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + "'" + std::string(val) + "'";
        queries_.emplace_back(std::make_tuple(E_QUERY_OR, std::move(query)));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, Query &>::type //处理char*类参数
    orWhere(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + "'" + val + "'";
        queries_.emplace_back(std::make_tuple(E_QUERY_OR, std::move(query)));
        return *this;
    }

  public:
    std::vector<std::tuple<E_QUERY_CONNECTOR, std::string>> queries_;
};

class Table
{
  public:
    Table(const std::string &table)
    {
        table_name_ = table;
    }
    virtual ~Table()
    {
    }
    void setConn(std::weak_ptr<sql::Connection> conn)
    {
        weak_conn_ = conn;
    }

  public:
    template <typename T>
    typename std::enable_if<!std::is_same<std::string, typename std::decay<T>::type>::value &&
                                !std::is_same<const char *, typename std::decay<T>::type>::value,
                            Table &>::type
    where(const std::string &field, const std::string &expr, T &&val)
    {
        std::string query;
        query = field + expr + std::to_string(val);
        Query q(E_QUERY_AND, std::move(query));
        queries_.emplace_back(std::move(q));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, const char *>::value, Table &>::type //const char*
    where(const std::string &field, const std::string &expr, T &&val)
    { //const char*
        std::string query;
        query = field + expr + "'" + std::string(val) + "'";
        Query q(E_QUERY_AND, std::move(query));
        queries_.emplace_back(std::move(q));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, Table &>::type //std::string
    where(const std::string &field, const std::string &expr, T &&val)
    { //
        std::string query;
        query = field + expr + "'" + val + "'";
        Query q(E_QUERY_AND, std::move(query));
        queries_.emplace_back(std::move(q));
        return *this;
    }

    template <typename T>
    typename std::enable_if<!std::is_same<std::string, typename std::decay<T>::type>::value &&
                                !std::is_same<const char *, typename std::decay<T>::type>::value,
                            Table &>::type
    orWhere(const std::string &field, const std::string &expr, T &&val)
    {
        std::string query;
        query = field + expr + std::to_string(val);
        queries_.emplace_back(std::move(std::make_tuple(E_QUERY_OR, std::move(query))));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, std::string>::value, Table &>::type //std::string
    orWhere(const std::string &field, const std::string &expr, T &&val)
    {
        std::string query;
        query = field + expr + "'" + val + "'";
        queries_.emplace_back(std::move(std::make_tuple(E_QUERY_OR, std::move(query))));
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<typename std::decay<T>::type, const char *>::value, Table &>::type //char*
    orWhere(const std::string &field, const std::string &expr, T val)
    {
        std::string query;
        query = field + expr + "'" + std::string(val) + "'";
        queries_.emplace_back(std::move(std::make_tuple(E_QUERY_OR, std::move(query))));
        return *this;
    }

    template <typename... T>
    Table &whereOrg(T... args)
    { //直接写表达式的方式，例如直接写 id = 12345 AND name = 'liming'，这些需要用AND连接起来
        Query query;
        std::vector<std::string> ws = {(0, args)...};
        for (const auto &w : ws)
        {
            Query q(E_QUERY_AND, std::move(w));
            query.addQuery(std::move(q));
        }
        queries_.emplace_back(std::move(query));
        return *this;
    }

    template <typename... T>
    Table &orWhereOrg(T... args)
    { //直接写表达式,所有参数都要传string类型
        Query query;
        std::vector<std::string> ws = {(0, args)...};
        for (const auto &w : ws)
        {
            Query q(E_QUERY_AND, std::move(w));
            query.addQuery(std::move(q));
        }
        queries_.emplace_back(std::move(query));
        return *this;
    }

    using QUERY_GENERATOR = std::function<void(Query&)>;
    Table &where(const QUERY_GENERATOR &query_generator)
    {
        Query query;
        query_generator(query);
        Query q(E_QUERY_AND, "(" + std::string(query) + ")");
        queries_.emplace_back(std::move(q));
        return *this;
    }

    template <typename... FIELDS>
    Table &select(FIELDS... f)
    { //无类型检查
        reset();
        std::string sql;
        std::vector<std::string> fields = {(0, f)...};
        fields_ = fields;
        op_ = E_OP_SELECT;
        return *this;
    }

    Table &update()
    {
        reset();
        op_ = E_OP_UPDATE;
        return *this;
    }

    template <typename T> //common template
    typename std::enable_if<!std::is_same<std::string, typename std::decay<T>::type>::value &&
                                !std::is_same<const char *, typename std::decay<T>::type>::value,
                            Table &>::type
    set(const std::string &field, T v)
    {
        if (op_ == E_OP_UPDATE)
        {
            update_fields_values_.insert(std::make_pair(field, std::to_string(v)));
        }
        return *this;
    }

    template <typename T> //string template
    typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, Table &>::type
    set(const std::string &field, T v)
    {
        if (op_ == E_OP_UPDATE)
        {
            update_fields_values_.insert(std::make_pair(field, "'" + v + "'"));
        }
        return *this;
    }

    template <typename T> //char * template
    typename std::enable_if<std::is_same<const char *, typename std::decay<T>::type>::value, Table &>::type
    set(const std::string &field, T v)
    {
        if (op_ == E_OP_UPDATE)
        {
            update_fields_values_.insert(std::make_pair(field, "'" + std::string(v) + "'"));
        }
        return *this;
    }

    template <typename... FIELDS>
    Table &insert(FIELDS... f)
    {
        reset();
        op_ = E_OP_INSERT;
        std::vector<std::string> fields = {(0, f)...};
        fields_ = fields;
        return *this;
    }

    Table &values()
    {
        return *this;
    }

    template <typename T> //一个参数的
    typename std::enable_if<!std::is_same<std::string, typename std::decay<T>::type>::value &&
                                !std::is_same<const char *, typename std::decay<T>::type>::value,
                            Table &>::type
    values(T && v)
    {
        if (op_ == E_OP_INSERT)
        {
            values_.push_back(std::to_string(v));
        }
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<std::string, typename std::decay<T>::type>::value, Table &>::type // std::string
    values(T && v)
    {
        if (op_ == E_OP_INSERT)
        {
            values_.push_back("'" + v + "'");
        }
        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_same<const char *, typename std::decay<T>::type>::value, Table &>::type // char*
    values(T && v)
    {
        if (op_ == E_OP_INSERT)
        {
            values_.push_back("'" + std::string(v) + "'");
        }
        return *this;
    }

    template <typename T, typename... REST> //至少两个参数的
    Table &values(T && v, REST... vs)
    {
        if (op_ == E_OP_INSERT)
        {
            return values(std::forward<T>(v)).values(vs...);
        }
        return *this;
    }

    Table &del()
    {
        reset();
        op_ = E_OP_DELETE;
        return *this;
    }

    std::string get_sql()
    {
        std::string sql;
        if (op_ == E_OP_SELECT)
        {
            sql = genSelectSql();
        }
        else if (op_ == E_OP_UPDATE)
        {
            sql = genUpdateSql();
        }
        else if (op_ == E_OP_INSERT)
        {
            sql = genInsertSql();
        }
        else if (op_ == E_OP_DELETE)
        {
            sql = genDeleteSql();
        }
        reset();
        return sql;
    }

    std::shared_ptr<sql::ResultSet> executeQuery()
    {
        std::string sql = genSelectSql();
        if (sql.empty())
        {
            reset();
            return nullptr;
        }
        std::shared_ptr<sql::ResultSet> res;
        auto shr_conn = weak_conn_.lock();
        if (!shr_conn)
        {
            reset();
            return nullptr;
        }

        try
        {
            std::shared_ptr<sql::PreparedStatement> pstmt;
            pstmt.reset(shr_conn->prepareStatement(sql));
            res.reset(pstmt->executeQuery());
        }
        catch (sql::SQLException &e)
        {
            if (e.getErrorCode() == 2006 || e.getErrorCode() == 2013)
            {
                shr_conn->reconnect();
            }
        }

        reset();
        return res;
    }

    int executeInsert()
    {
        std::string sql = genInsertSql();
        // std::cout << "executeInsert:"<< sql<<std::endl;
        if (sql.empty())
        {
            reset();
            return -1;
        }

        auto shr_conn = weak_conn_.lock();
        if (!shr_conn)
        {
            reset();
            return -2;
        }

        int ret = 0;
        try
        {
            std::shared_ptr<sql::PreparedStatement> pstmt;
            pstmt.reset(shr_conn->prepareStatement(sql));
            ret = pstmt->executeUpdate();
        }
        catch (sql::SQLException &e)
        {
            if (e.getErrorCode() == 2006 || e.getErrorCode() == 2013)
            {
                shr_conn->reconnect();
            }
            else if (e.getErrorCode() == 1062)
            { //duplicate key
                reset();
                return -3;
            }
        }
        reset();
        return ret;
    }

    int executeUpdate()
    {
        std::string sql = genUpdateSql();
        // std::cout << "executeUpdate:"<< sql<<std::endl;
        if (sql.empty())
        {
            reset();
            return -1;
        }

        auto shr_conn = weak_conn_.lock();
        if (!shr_conn)
        {
            reset();
            return -2;
        }

        int ret = 0;
        try
        {
            std::shared_ptr<sql::PreparedStatement> pstmt;
            pstmt.reset(shr_conn->prepareStatement(sql));
            ret = pstmt->executeUpdate();
        }
        catch (sql::SQLException &e)
        {
            if (e.getErrorCode() == 2006 || e.getErrorCode() == 2013)
            {
                shr_conn->reconnect();
            }
        }
        reset();
        return ret;
    }

    int executeDelete()
    {
        std::string sql = genDeleteSql();
        // std::cout << "executeUpdate:"<< sql<<std::endl;
        if (sql.empty())
        {
            reset();
            return -1;
        }

        auto shr_conn = weak_conn_.lock();
        if (!shr_conn)
        {
            reset();
            return -2;
        }

        bool ret = false;
        try
        {
            std::shared_ptr<sql::PreparedStatement> pstmt;
            pstmt.reset(shr_conn->prepareStatement(sql));
            ret = pstmt->execute();
        }
        catch (sql::SQLException &e)
        {
            if (e.getErrorCode() == 2006 || e.getErrorCode() == 2013)
            {
                shr_conn->reconnect();
            }
        }
        reset();
        return ret ? 0 : -2;
    }

  private:
    std::string genSelectSql()
    {
        if (E_OP_SELECT != op_)
        {
            return "";
        }
        
        std::string select_fields;
        if (fields_.size() <= 0)
        {
            return "";
        }

        if (std::count_if(fields_.begin(), fields_.end(), [](const std::string &f) { return f == "*"; }) > 0)
        {
            select_fields = "*";
        }
        else
        {
            select_fields = boost::join(fields_, ",");
        }

        std::string query_where;
        for (auto it = queries_.begin(); it != queries_.end(); it++)
        {
            if (it != queries_.begin())
            {
                query_where += " AND (" + std::string(*it) + ")";
            }
            else
            {
                query_where += "(" + std::string(*it) + ")";
            }
        }

        std::string sql = "SELECT " + select_fields + " FROM " + table_name_ + " WHERE " + query_where;
        return sql;
    }

    std::string genUpdateSql()
    {
        if (E_OP_UPDATE != op_)
        {
            return "";
        }

        if (update_fields_values_.size() <= 0)
        {
            return "";
        }

        std::vector<std::string> field_values_vec;
        for (const auto &fv : update_fields_values_)
        {
            field_values_vec.emplace_back(fv.first + "=" + fv.second);
        }

        std::string field_values = boost::join(field_values_vec, ",");

        std::string update_where;
        for (auto it = queries_.begin(); it != queries_.end(); it++)
        {
            if (it != queries_.begin())
            {
                update_where += " AND (" + std::string(*it) + ")";
            }
            else
            {
                update_where += "(" + std::string(*it) + ")";
            }
        }

        std::string sql = "UPDATE " + table_name_ + " SET " + field_values + " WHERE " + update_where;
        return sql;
    }

    std::string genInsertSql()
    {
        if (E_OP_INSERT != op_)
        {
            return "";
        }

        std::string insert_fields;
        if (fields_.size() <= 0)
        {
            return "";
        }
        insert_fields = boost::join(fields_, ",");

        std::string str_values;
        str_values = boost::join(values_, ",");

        std::string sql = "INSERT INTO " + table_name_ + "(" + insert_fields + ")" + " VALUES(" + str_values + ")";
        return sql;
    }

    std::string genDeleteSql()
    {
        if (E_OP_DELETE != op_)
        {
            return "";
        }

        if (queries_.size() <= 0)
        { //不能全部删
            return "";
        }

        std::string delete_where;
        for (auto it = queries_.begin(); it != queries_.end(); it++)
        {
            if (it != queries_.begin())
            {
                delete_where += " AND (" + std::string(*it) + ")";
            }
            else
            {
                delete_where += "(" + std::string(*it) + ")";
            }
        }

        std::string sql = "DELETE FROM " + table_name_ + " WHERE " + delete_where;
        return sql;
    }

    void reset()
    {
        op_ = E_OP_NONE;
        update_fields_values_.clear();
        fields_.clear();
        values_.clear();
        queries_.clear();
    }

    E_MYSQL_OP op_ = E_OP_NONE;
    std::weak_ptr<sql::Connection> weak_conn_;
    std::string table_name_;
    std::map<std::string, std::string> update_fields_values_;
    std::vector<std::string> fields_;
    std::vector<std::string> values_;
    std::vector<Query> queries_;
};
#endif