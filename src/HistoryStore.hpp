#ifndef BRCODE_HISTORYSTORE_HPP
#define BRCODE_HISTORYSTORE_HPP
#include <string>
#include <vector>
struct sqlite3;
namespace brcode {
struct Record {
    std::string id, source, body, title, notes, created, updated;
};
class HistoryStore {
public:
    HistoryStore();
    ~HistoryStore();
    bool open(const std::string &path);
    void close();
    bool save(const Record &record);
    bool get(const std::string &id, Record &record);
    bool updateNotes(const std::string &id, const std::string &title,
                     const std::string &notes, const std::string &updated);
    bool remove(const std::string &id);
    bool page(const std::string &search, const std::string &beforeTime,
              const std::string &beforeId, int limit, std::vector<Record> &records);
    std::string error() const { return error_; }
private:
    HistoryStore(const HistoryStore &);
    HistoryStore &operator=(const HistoryStore &);
    bool exec(const char *sql);
    bool fail(const std::string &message);
    sqlite3 *db_;
    std::string error_;
};
}
#endif
