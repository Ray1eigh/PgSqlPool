#include "include/mysqlpool.h"

MysqlPool* MysqlPool::mysqlpool_object = NULL;
std::mutex MysqlPool::objectlock;
std::mutex MysqlPool::poollock;

MysqlPool::MysqlPool() {};

/*
 *配置数据库参数
 */
void MysqlPool::setParameter( std::string opt,
                              unsigned long client_flag,
                              unsigned int  max_connect ) {
  
  _client_flag  = client_flag;
  MAX_CONNECT   = max_connect;
  m_conn_opt = opt;
}
  
/*
 *有参的单例函数，用于第一次获取连接池对象，初始化数据库信息。
 */
MysqlPool* MysqlPool::getMysqlPoolObject() {
  if (mysqlpool_object == NULL) { 
    objectlock.lock();
    if (mysqlpool_object == NULL) {
      mysqlpool_object = new MysqlPool();
    }
    objectlock.unlock();
  }
  return mysqlpool_object;
}
                                                 
/*
 *创建一个连接对象
 */
dbConnection_ptr MysqlPool::createOneConnect() {
  dbConnection_ptr conn(new dbConnection(m_conn_opt));

  if (conn->pqxx_conn.is_open()) {
      std::cout<<"connect to postgresql successfully. dbname=" <<conn->pqxx_conn.dbname()<<std::endl;
      return conn;   
    } else {
      std::cout << "connect to postgresql failed!" << std::endl;
      return NULL;
    }
}

/*
 *判断当前MySQL连接池的是否空
 */
bool MysqlPool::isEmpty() {
  return mysqlpool.empty();
}
/*
 *获取当前连接池队列的队头
 */
dbConnection_ptr MysqlPool::poolFront() {
  return mysqlpool.front();
}
/*
 *
 */
unsigned int MysqlPool::poolSize() {
  return mysqlpool.size();
}
/*
 *弹出当前连接池队列的队头
 */
void MysqlPool::poolPop() {
  mysqlpool.pop();
}
/*
 *获取连接对象，如果连接池中有连接，就取用;没有，就重新创建一个连接对象。
 *同时注意到MySQL的连接的时效性，即在连接队列中,连接对象在超过一定的时间后没有进行操作，
 *MySQL会自动关闭连接，当然还有其他原因，比如：网络不稳定，带来的连接中断。
 *所以在获取连接对象前，需要先判断连接池中连接对象是否有效。
 *考虑到数据库同时建立的连接数量有限制，在创建新连接需提前判断当前开启的连接数不超过设定值。
 */
dbConnection_ptr MysqlPool::getOneConnect() {
  poollock.lock();
  dbConnection_ptr conn = NULL;
  if (!isEmpty()) {
    while (!isEmpty() && !(poolFront()->pqxx_conn.is_open())) {
      poolFront()->pqxx_conn.disconnect();
      poolPop();
      connect_count--;
    }
    if (!isEmpty()) {
      conn = poolFront();
      poolPop();
    } else {
      if (connect_count < MAX_CONNECT)
        conn = createOneConnect(); 
      else 
        std::cerr << "the number of mysql connections is too much!" << std::endl;
    }
  } else {
    if (connect_count < MAX_CONNECT)
      conn = createOneConnect(); 
    else 
      std::cerr << "the number of mysql connections is too much!" << std::endl;
  }
  poollock.unlock();
  return conn;
}
/*
 *将有效的链接对象放回链接池队列中，以待下次的取用。
 */
void MysqlPool::close(dbConnection_ptr conn) {
  if (conn != NULL) {
    poollock.lock();
    mysqlpool.push(conn);
    poollock.unlock();
  }
}
/*
 * sql语句执行函数，并返回结果，没有结果的SQL语句返回空结果，
 * 每次执行SQL语句都会先去连接队列中去一个连接对象，
 * 执行完SQL语句，就把连接对象放回连接池队列中。
 * 返回对象用map主要考虑，用户可以通过数据库字段，直接获得查询的字。
 * 例如：m["字段"][index]。
 */
pqxx::result MysqlPool::executeSql(const char* sql) {
  
  dbConnection_ptr conn = getOneConnect();
  pqxx::nontransaction ntx(conn->pqxx_conn);
  pqxx::result res(ntx.exec(sql));
  //这里应该对执行结果进行判断
  close(conn);

  return res;
}
/*
*
* 增删改的sql执行语句，执行完成后返回
*
*/
void MysqlPool::executeAddSql(const char* sql){
  dbConnection_ptr conn = getOneConnect();
  pqxx::work tnx(conn->pqxx_conn);
  tnx.exec(sql);
  tnx.commit();
  close(conn);
}

/*
 * 析构函数，将连接池队列中的连接全部关闭
 */
MysqlPool::~MysqlPool() {
  while (poolSize() != 0) {
    poolFront()->pqxx_conn.disconnect();
    poolPop();
    connect_count--;
  }
}


