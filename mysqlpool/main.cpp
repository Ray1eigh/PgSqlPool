#include"include/mysqlpool.h"
using namespace pqxx;
using namespace std;

static MysqlPool *mysql;

int main() {
  mysql = MysqlPool::getMysqlPoolObject();
  mysql->setParameter("dbname=mydb user=postgres password=postgres hostaddr=127.0.0.1 port=5432");
  mysql->executeSql("insert into xxx_user(id,name,age) values (1,'aaa',18)");


  delete mysql;
  return 0;
}
