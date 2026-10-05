#include "HistoryStore.hpp"
#include "ScanGate.hpp"
#include <sqlite3.h>
#include <cstdio>
#include <cstdlib>
#include <set>
using namespace brcode;
static int checks=0;
static void check(bool ok,const char *name){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}std::printf("PASS %s\n",name);}
static Record record(const std::string &id,const std::string &body){Record r;r.id=id;r.source="scan";r.body=body;r.title="标题";r.notes="备注\n' \" &";r.created="2026-10-05T02:00:00.000Z";r.updated=r.created;return r;}
static void sql(const char *path,const char *statement){sqlite3 *db=0;check(sqlite3_open(path,&db)==SQLITE_OK,"test fixture database open");int rc=sqlite3_exec(db,statement,0,0,0);if(rc!=SQLITE_OK)std::fprintf(stderr,"fixture error %s\n",sqlite3_errmsg(db));check(rc==SQLITE_OK,"test fixture SQL");sqlite3_close(db);}
int main(){
    std::setvbuf(stdout,0,_IONBF,0);
    HistoryStore store;check(store.open("history.sqlite"),"new schema opens");
    const char *samples[]={"ASCII 123","中文二维码","a\nb\r\nc","  leading and trailing  ","'\"<&>\\%_","https://example.org/a?q=%20#b","🙂🚀"," \t\n"};
    for(int i=0;i<8;++i){char id[16];std::sprintf(id,"sample-%02d",i);Record r=record(id,samples[i]);check(store.save(r),"save text fixture");Record loaded;check(store.get(r.id,loaded)&&loaded.body==r.body&&loaded.notes==r.notes,"exact UTF-8/body/notes readback");}
    Record nul=record("nul",std::string("a\0b",3));check(store.save(nul),"bound NUL preserved");Record loaded;check(store.get(nul.id,loaded)&&loaded.body==nul.body,"NUL storage readback");
    Record original=record("immutable","original\n正文");check(store.save(original),"save immutable original");
    check(store.updateNotes(original.id,"新的标题","仅改备注", "2026-10-05T03:00:00.000Z"),"edit title and notes");
    check(store.get(original.id,loaded)&&loaded.body==original.body&&loaded.title=="新的标题"&&loaded.notes=="仅改备注"&&loaded.created==original.created&&loaded.updated!=original.created,"note edit never changes original body/created");
    std::vector<Record> rows;check(store.page("新的标题","","",40,rows)&&rows.size()==1,"search title");
    check(store.page("仅改备注","","",40,rows)&&rows.size()==1,"search notes");
    check(store.page("中文","","",40,rows)&&rows.size()==1&&rows[0].body==samples[1],"search full body");
    check(store.page("' OR 1=1 --","","",40,rows)&&rows.empty(),"SQL injection treated as literal search");
    check(store.page("%_","","",40,rows)&&rows.size()==1,"percent and underscore search remain literal");
    Record duplicate=original;duplicate.body="overwrite";check(!store.save(duplicate),"duplicate ID fails without overwrite");
    check(store.get(original.id,loaded)&&loaded.body==original.body,"failed write retains original");
    sql("history.sqlite","CREATE TRIGGER fail_save BEFORE INSERT ON records WHEN NEW.id='fail' BEGIN SELECT RAISE(ABORT,'synthetic write refusal'); END");
    check(!store.save(record("fail","unsaved")),"transaction failure reports false");
    check(!store.get("fail",loaded),"aborted record is absent");
    sql("history.sqlite","DROP TRIGGER fail_save");check(store.save(record("fail","retry body")),"retry after rollback succeeds");
    sql("history.sqlite","CREATE TRIGGER fail_notes BEFORE UPDATE ON records BEGIN SELECT RAISE(ABORT,'synthetic notes refusal'); END");
    check(!store.updateNotes(original.id,"bad","bad","later"),"notes transaction failure reports false");
    check(store.get(original.id,loaded)&&loaded.title=="新的标题"&&loaded.body==original.body,"failed notes transaction keeps data");
    sql("history.sqlite","DROP TRIGGER fail_notes");
    for(int i=0;i<85;++i){char id[16];std::sprintf(id,"page-%03d",i);Record r=record(id,"pagination");check(store.save(r),"save pagination fixture");}
    std::set<std::string> seen;std::string before,id;
    do{check(store.page("pagination",before,id,40,rows),"keyset page loads");for(size_t i=0;i<rows.size();++i){check(seen.insert(rows[i].id).second,"keyset page has no duplicate");before=rows[i].created;id=rows[i].id;}}while(rows.size()==40);
    check(seen.size()==85,"pagination reaches every tied timestamp record");
    store.close();check(store.open("history.sqlite"),"reopen schema without clearing");
    check(store.get(original.id,loaded)&&loaded.body==original.body&&loaded.notes=="仅改备注","restart persists full body and notes");
    check(store.remove(original.id)&&!store.get(original.id,loaded),"delete one record");
    check(store.get("sample-00",loaded),"delete retains other records");
    sql("legacy.sqlite","CREATE TABLE records(id TEXT PRIMARY KEY NOT NULL,source TEXT NOT NULL,body TEXT NOT NULL,title TEXT NOT NULL,notes TEXT NOT NULL,created TEXT NOT NULL); INSERT INTO records VALUES('old','scan','  原文\nline  ','旧标题','旧备注','2025-01-01'); PRAGMA user_version=1");
    HistoryStore legacy;check(legacy.open("legacy.sqlite"),"schema v1 migrates transactionally");
    check(legacy.get("old",loaded)&&loaded.body=="  原文\nline  "&&loaded.title=="旧标题"&&loaded.notes=="旧备注"&&loaded.updated==loaded.created,"migration retains exact existing data");
    legacy.close();sql("legacy.sqlite","PRAGMA user_version=99");
    check(!legacy.open("legacy.sqlite"),"future schema refused without dropping history");
    sqlite3 *db=0;sqlite3_open("legacy.sqlite",&db);sqlite3_stmt *s=0;sqlite3_prepare_v2(db,"SELECT count(*) FROM records WHERE id='old'",-1,&s,0);check(sqlite3_step(s)==SQLITE_ROW&&sqlite3_column_int(s,0)==1,"refused upgrade does not delete old records");sqlite3_finalize(s);sqlite3_close(db);
    ScanGate gate;unsigned long epoch=gate.begin();check(gate.accept(epoch),"first frame accepted");
    for(int i=0;i<20;++i)check(!gate.accept(epoch),"repeated frame rejected");
    unsigned long second=gate.begin();check(!gate.accept(epoch),"old epoch frame rejected");check(gate.accept(second),"same text can be accepted after explicit new session");
    unsigned long third=gate.begin();gate.stop();check(!gate.accept(third),"late frame after stop rejected");
    std::printf("RESULT {\"status\":\"PASS\",\"assertions\":%d}\n",checks);return 0;
}
