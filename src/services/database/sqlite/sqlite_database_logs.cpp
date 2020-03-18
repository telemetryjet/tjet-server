#include <fmt/format.h>
#include "sqlite_database.h"

std::vector<record_log_t> SqliteDatabase::getLogs(int system_id) {
    const std::lock_guard<std::mutex> lock(databaseMutex); // Acquire database lock for this scope

    std::vector<record_log_t> logs;
    try {
        SQLite::Statement query(*db, fmt::format("select * from logs where system_id={}", system_id));
        while (query.executeStep()) {
            logs.push_back({
                  query.getColumn(0),
                  query.getColumn(1),
                  query.getColumn(2)
            });
        }
    } catch (std::exception& e) {
        throwError(fmt::format("Error in getLogs: {}", e.what()));
    }
    return logs;
}

record_log_t SqliteDatabase::getLog(int id) {
    const std::lock_guard<std::mutex> lock(databaseMutex); // Acquire database lock for this scope

    SQLite::Statement query(*db, "select * from logs where id=?");
    query.bind(1, id);
    query.executeStep();
    if (query.hasRow()){
        return {
                query.getColumn(0),
                query.getColumn(1),
                query.getColumn(2)
        };
    } else {
        throwError(fmt::format("Error in getLog: Log with id = {} not found.",id));
    }
}

record_log_t SqliteDatabase::createLog(record_log_t log) {
    const std::lock_guard<std::mutex> lock(databaseMutex); // Acquire database lock for this scope

    try {
        SQLite::Statement insertStatement(*db, "insert into logs values (null,?,?)");
        insertStatement.bind(1, log.system_id);
        insertStatement.bind(2, log.message);
        insertStatement.exec();
        log.id = db->getLastInsertRowid();
        return log;
    } catch (std::exception &e) {
        throwError(fmt::format("Error in createLog: {}", e.what()));
    }
}

void SqliteDatabase::updateLog(record_log_t log) {
    const std::lock_guard<std::mutex> lock(databaseMutex); // Acquire database lock for this scope

    try {
        SQLite::Statement updateStatement(*db, "update logs set system_id=?, message=? where id=?");
        updateStatement.bind(1, log.system_id);
        updateStatement.bind(2, log.message);
        updateStatement.bind(3, log.id);
        updateStatement.exec();
    } catch (std::exception &e) {
        throwError(fmt::format("Error in updateLog: {}", e.what()));
    }
}