#include "HistoryStore.hpp"
#include <sqlite3.h>
#include <climits>
#include <cstddef>
namespace brcode {
namespace {
class Statement {
public:
    Statement(sqlite3 *db, const char *sql): stmt(0) { code=sqlite3_prepare_v2(db,sql,-1,&stmt,0); }
    ~Statement() { if(stmt) sqlite3_finalize(stmt); }
    bool bind(int index,const std::string &s) {
        return s.size()<=INT_MAX && sqlite3_bind_text(stmt,index,s.data(),static_cast<int>(s.size()),SQLITE_TRANSIENT)==SQLITE_OK;
    }
    sqlite3_stmt *stmt;
    int code;
};
std::string column(sqlite3_stmt *s,int i) {
    const char *p=reinterpret_cast<const char *>(sqlite3_column_text(s,i));
    return p ? std::string(p,sqlite3_column_bytes(s,i)) : std::string();
}
void read(sqlite3_stmt *s,Record &r) {
    r.id=column(s,0);r.source=column(s,1);r.body=column(s,2);r.title=column(s,3);
    r.notes=column(s,4);r.created=column(s,5);r.updated=column(s,6);
}
}
HistoryStore::HistoryStore():db_(0) {}
HistoryStore::~HistoryStore(){close();}
void HistoryStore::close(){if(db_)sqlite3_close(db_);db_=0;}
bool HistoryStore::fail(const std::string &message){error_=message;return false;}
bool HistoryStore::exec(const char *sql){
    if(!db_)return fail("Database is not open");
    if(sqlite3_exec(db_,sql,0,0,0)!=SQLITE_OK)return fail(sqlite3_errmsg(db_));
    return true;
}
bool HistoryStore::open(const std::string &path){
    close();error_.clear();
    if(sqlite3_open_v2(path.c_str(),&db_,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE,0)!=SQLITE_OK){
        fail(db_?sqlite3_errmsg(db_):"Cannot open database");close();return false;
    }
    sqlite3_busy_timeout(db_,1500);
    if(!exec("PRAGMA synchronous=FULL")||!exec("BEGIN IMMEDIATE"))return false;
    int version=-1;
    {Statement q(db_,"PRAGMA user_version");if(q.code==SQLITE_OK&&sqlite3_step(q.stmt)==SQLITE_ROW)version=sqlite3_column_int(q.stmt,0);}
    if(version<0||version>2){exec("ROLLBACK");return fail("Unsupported database schema; history was preserved");}
    if(version==0){
        if(!exec("CREATE TABLE records(id TEXT PRIMARY KEY NOT NULL,source TEXT NOT NULL CHECK(source IN ('scan','generate')),body TEXT NOT NULL,title TEXT NOT NULL,notes TEXT NOT NULL,created TEXT NOT NULL)")||!exec("PRAGMA user_version=1")){
            std::string reason=error_;exec("ROLLBACK");return fail(reason);
        }
        version=1;
    }
    if(version==1){
        if(!exec("ALTER TABLE records ADD COLUMN updated TEXT NOT NULL DEFAULT ''")||
           !exec("UPDATE records SET updated=created WHERE updated=''")||
           !exec("CREATE INDEX IF NOT EXISTS records_time ON records(created DESC,id DESC)")||!exec("PRAGMA user_version=2")){
            std::string reason=error_;exec("ROLLBACK");return fail(reason);
        }
    }
    if(!exec("COMMIT")){std::string reason=error_;exec("ROLLBACK");return fail(reason);}
    return true;
}
bool HistoryStore::save(const Record &r){
    if(!db_)return fail("Database is not open");
    error_.clear();
    if(!exec("BEGIN IMMEDIATE"))return false;
    bool ok=false;
    {Statement s(db_,"INSERT INTO records(id,source,body,title,notes,created,updated) VALUES(?,?,?,?,?,?,?)");
    if(s.code==SQLITE_OK&&s.bind(1,r.id)&&s.bind(2,r.source)&&s.bind(3,r.body)&&s.bind(4,r.title)&&s.bind(5,r.notes)&&s.bind(6,r.created)&&s.bind(7,r.updated))
        ok=sqlite3_step(s.stmt)==SQLITE_DONE;
    if(!ok)fail(sqlite3_errmsg(db_));}
    if(!ok){std::string reason=error_;exec("ROLLBACK");return fail(reason);}
    if(!exec("COMMIT")){std::string reason=error_;exec("ROLLBACK");return fail(reason);}
    return true;
}
bool HistoryStore::get(const std::string &id,Record &r){
    if(!db_)return fail("Database is not open");
    Statement s(db_,"SELECT id,source,body,title,notes,created,updated FROM records WHERE id=?");
    if(s.code!=SQLITE_OK||!s.bind(1,id))return fail(sqlite3_errmsg(db_));
    int rc=sqlite3_step(s.stmt);
    if(rc==SQLITE_ROW){read(s.stmt,r);return true;}
    return fail(rc==SQLITE_DONE?"Record no longer exists":sqlite3_errmsg(db_));
}
bool HistoryStore::updateNotes(const std::string &id,const std::string &title,const std::string &notes,const std::string &updated){
    if(!db_)return fail("Database is not open");
    if(!exec("BEGIN IMMEDIATE"))return false;
    bool ok=false;
    {Statement s(db_,"UPDATE records SET title=?,notes=?,updated=? WHERE id=?");
    ok=s.code==SQLITE_OK&&s.bind(1,title)&&s.bind(2,notes)&&s.bind(3,updated)&&s.bind(4,id)&&sqlite3_step(s.stmt)==SQLITE_DONE&&sqlite3_changes(db_)==1;}
    if(!ok){std::string reason=sqlite3_errmsg(db_);exec("ROLLBACK");return fail("Notes update failed: "+reason);}
    if(!exec("COMMIT")){std::string reason=error_;exec("ROLLBACK");return fail(reason);}return true;
}
bool HistoryStore::remove(const std::string &id){
    if(!db_)return fail("Database is not open");
    Statement s(db_,"DELETE FROM records WHERE id=?");
    if(s.code!=SQLITE_OK||!s.bind(1,id)||sqlite3_step(s.stmt)!=SQLITE_DONE)return fail(sqlite3_errmsg(db_));
    return sqlite3_changes(db_)==1||fail("Record no longer exists");
}
bool HistoryStore::page(const std::string &search,const std::string &time,const std::string &id,int limit,std::vector<Record> &records){
    records.clear();if(!db_)return fail("Database is not open");
    if(limit<1||limit>100)return fail("Invalid page size");
    // SQLite 3.7.14.1 has no instr(). Escape LIKE metacharacters so search is literal.
    std::string pattern="%";
    for(std::size_t i=0;i<search.size();++i){if(search[i]=='%'||search[i]=='_'||search[i]=='\\')pattern+='\\';pattern+=search[i];}
    pattern+='%';
    Statement s(db_,"SELECT id,source,body,title,notes,created,updated FROM records WHERE (?='' OR body LIKE ? ESCAPE '\\' OR title LIKE ? ESCAPE '\\' OR notes LIKE ? ESCAPE '\\') AND (?='' OR created<? OR (created=? AND id<?)) ORDER BY created DESC,id DESC LIMIT ?");
    if(s.code!=SQLITE_OK)return fail(sqlite3_errmsg(db_));
    if(!s.bind(1,search))return fail("Search bind failed");
    for(int i=2;i<=4;++i)if(!s.bind(i,pattern))return fail("Search bind failed");
    if(!s.bind(5,time)||!s.bind(6,time)||!s.bind(7,time)||!s.bind(8,id)||sqlite3_bind_int(s.stmt,9,limit)!=SQLITE_OK)return fail("Pagination bind failed");
    int rc;
    while((rc=sqlite3_step(s.stmt))==SQLITE_ROW){Record r;read(s.stmt,r);records.push_back(r);}
    return rc==SQLITE_DONE||fail(sqlite3_errmsg(db_));
}
}
