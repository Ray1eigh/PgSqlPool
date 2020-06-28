#ifndef MYSQLPOOL_H
#define MYSQLPOOL_H

#include<iostream>
#include<mysql/mysql.h>
#include<queue>
#include<map>
#include<vector>
#include<utility>
#include<string>
#include<mutex>
#include<thread>
#include<pqxx/pqxx>

class dbConnection
{
    friend class dbConnectionPool;
    bool is_using; //未使用

public:
    pqxx::connection pqxx_conn;

    dbConnection(std::string opt) 
        : pqxx_conn(opt),is_using(false)
        {}
};

typedef std::shared_ptr<dbConnection> dbConnection_ptr;

class MysqlPool {
  public:
    ~MysqlPool();
    pqxx::result executeSql(const char* sql);//sql查询语句的执行函数
    void executeAddSql(const char* sql);//增删改sql语句的执行函数
    static MysqlPool* getMysqlPoolObject();              //单列模式获取本类的对象
    void setParameter( std::string opt,
                       unsigned long _client_flag = 0,
                       unsigned int  MAX_CONNECT = 50 );              //设置数据库参数
  private:
    MysqlPool();
    dbConnection_ptr createOneConnect();                    //创建一个新的连接对象
    dbConnection_ptr getOneConnect();                       //获取一个连接对象
    void close(dbConnection_ptr conn);                      //关闭连接对象
    bool isEmpty();                               //连接池队列池是否为空
    dbConnection_ptr poolFront();                           //连接池队列的队头
    unsigned int poolSize();                      //获取连接池的大小
    void poolPop();                               //弹出连接池队列的队头
  private:
    std::queue<dbConnection_ptr> mysqlpool;                 //连接池队列
    unsigned long _client_flag;                   //设置为0
    unsigned int  MAX_CONNECT;                    //同时允许最大连接对象数量
    unsigned int  connect_count;                  //目前连接池的连接对象数量
    static std::mutex objectlock;                 //对象锁
    static std::mutex poollock;                   //连接池锁
    static MysqlPool* mysqlpool_object;           //类的对象
    std::string m_conn_opt;                 //初始化命令
};

#endif
