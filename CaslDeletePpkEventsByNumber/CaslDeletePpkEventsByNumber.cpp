#include <iostream>
#include <vector>
#include <string>
#include "sqlite/sqlite3.h"
#include <filesystem>
#include <Windows.h>

using namespace std;
namespace fs = std::filesystem;

wstring getWideStringPath(const fs::path& path) {
    return path.wstring();
}

string convertWideStringToString(const wstring& wstr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (bufferSize == 0) {
        throw runtime_error("Error converting wide string to UTF-8 string");
    }
    vector<char> buffer(bufferSize);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), bufferSize, nullptr, nullptr);
    return string(buffer.data());
}

int main()
{
    vector<string> tables = { "event", "event_converted", "event_d128", "event_dozor", "event_sia", "event_vbd4" };
    int deviceNumber;
    wstring wideDatabasePath = getWideStringPath(fs::current_path() / "data.db");
    string databasePath = convertWideStringToString(wideDatabasePath);

    if (!fs::exists(wideDatabasePath))
    {
        wcout << L"Database file does not exist at path: " << wideDatabasePath << endl;
        wcin.get();
        return 1;
    }

    while (true)
    {
        cout << "Enter device number: ";
        string input;
        getline(cin, input);

        try
        {
            deviceNumber = stoi(input);
            break;
        }
        catch (invalid_argument& e)
        {
            cout << "Invalid input. Please enter a valid number.\n" << endl;
        }
    }

    sqlite3* db;
    int rc = sqlite3_open(databasePath.c_str(), &db);

    if (rc)
    {
        cout << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return rc;
    }
    else
    {
        cout << "Connected to DB!" << endl;
    }

    for (const auto& tableName : tables)
    {
        string query = "DELETE FROM " + tableName + " WHERE device_id = (SELECT device_id FROM device WHERE number = ?);";
        sqlite3_stmt* stmt;
        rc = sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK)
        {
            cout << "Failed to prepare statement for table " << tableName << ": " << sqlite3_errmsg(db) << endl;
            continue;
        }

        sqlite3_bind_int(stmt, 1, deviceNumber);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            cout << "\nError deleting from table " << tableName << ": " << sqlite3_errmsg(db) << endl;
        }
        else
        {
            int rowsAffected = sqlite3_changes(db);
            cout << rowsAffected << " rows deleted from " << tableName << " table." << endl;
        }

        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
    cout << "\nDisconnected from DB!" << endl;
    cin.get();
    return 0;
}
