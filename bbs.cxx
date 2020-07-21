#include <memory>
#include <stdexcept>
#include <sqlite3.h>
#include "crow_all.h"
#include <string>
#include <fstream>
#include <iostream>

int
main() {
  sqlite3 *db = nullptr;
  
  //just practice. maybe .lib makes db file automatically
  std::ifstream ifs("./bbs.db");
  if (!ifs.is_open()){
    std::ofstream("./bbs.db");  
  }
  
  //open
  int r = sqlite3_open("bbs.db", &db);
  if (SQLITE_OK != r) {
    throw std::runtime_error("can't open database");
  }
  
    
    sqlite3_stmt* stmt; 
    const char *sql = "select count(*) from sqlite_master where type='table' and name='bbs';";
    sqlite3_prepare(db, sql, -1, &stmt, NULL);
    sqlite3_step(stmt);
    int table_num = sqlite3_column_int(stmt, 0);
    sqlite3_reset(stmt);

    if(table_num == 0){
        const char *sql1 = "CREATE TABLE bbs (id integer primary key,text text,created timestamp default current_timestamp)";
        sqlite3_prepare(db, sql1, -1, &stmt, NULL);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
        const char *sql2 = "INSERT INTO bbs(id, text) VALUES (1, 'DEMO');";
        sqlite3_prepare(db, sql2, -1, &stmt, NULL);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);        
    }

    
  crow::SimpleApp app;
  crow::mustache::set_base(".");

  CROW_ROUTE(app, "/")
  ([&] {
    crow::mustache::context ctx;
    const char *sql = "select id, text from bbs order by created";
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare(db, sql, -1, &stmt, nullptr);
    int n = 0;
    while (SQLITE_DONE != sqlite3_step(stmt)) {
      ctx["posts"][n]["id"] = (std::string) (char*) sqlite3_column_text(stmt, 0);
      ctx["posts"][n]["text"] = (std::string) (char*) sqlite3_column_text(stmt, 1);
      n++;
    }
    sqlite3_finalize(stmt);
    return crow::mustache::load("bbs.html").render(ctx);
  });

  CROW_ROUTE(app, "/post").methods("POST"_method)
  ([&](const crow::request& req, crow::response& res) {
    crow::query_string params(std::string("?") + req.body);
    char* q = params.get("text");
    if (q == nullptr) {
      res = crow::response(400);
      res.write("bad request");
      res.end();
      return;
    }
    const char *sql = "insert into bbs(text) values(?)";
    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, q, -1,
      (sqlite3_destructor_type) SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    res = crow::response(302);
    res.set_header("Location", "/");
    res.end();
  });

  app.port(18080)
    //.multithreaded()
    .run();
}
