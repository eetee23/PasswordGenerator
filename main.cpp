#include "iostream"
#include "random"
#include "chrono"
#include "string"
#include "vector"
#include "map"

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdlib>
#endif

#include <sqlite3.h>

using namespace std;

// prototype
void create_credentials_to_db();

void copy_to_clipboard(const string& random_password) {
    #ifdef _WIN32
        const size_t lrp = random_password.length() + 1;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, lrp);
        if (!hMem) {
            cout << "Failed to allocate memory for coping" << endl;
            return;
        }
        memcpy(GlobalLock(hMem), random_password.c_str(), lrp);
        GlobalUnlock(hMem);

        if (OpenClipboard(0)) {
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
            cout << "Password copied to clip board" << endl;
        } else {
            cout << "Failed to open clipbroad" << endl;
            GlobalFree(hMem);
        }
    #else
        string cmd = "echo \"" + random_password + "\" | xclip -selection clipboard";
    #endif
}

map<string, string> search_by_id(string input) {
    sqlite3* db;
    char message_error;
    int id = stoi(input);
    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for password id search" << sqlite3_errmsg(db) << endl;
    }

    string check_by_id = "SELECT * FROM passwords WHERE ID = ?;";
    sqlite3_stmt* stmt;
    
    exit = sqlite3_prepare_v2(db, check_by_id.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare select statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return {};
    }

    sqlite3_bind_int(stmt, 1, id);

    exit = sqlite3_step(stmt);

    map<string, string> result;

    if (exit == SQLITE_ROW) {
        int fetched_id = sqlite3_column_int(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        const unsigned char* password = sqlite3_column_text(stmt, 2);

        cout << "id: " << fetched_id << " name: " << name << endl;


        result["id"] = fetched_id;
        result["name"] = name ? reinterpret_cast<const char*>(name): "";
        result["password"] = password ? reinterpret_cast<const char*>(password): "";
    } else {
        cerr << "ERROR in selecting password: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return {};
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void login (string &login_username, string &login_password) {
    cout << "Enter username: ";
    cin >> login_username;

    cout << "Enter passsword: ";
    cin >> login_password;
}

map<string, string> search_by_name(string input) {
    sqlite3* db;
    char message_error;
    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for password name search" << sqlite3_errmsg(db) << endl;
    }

    string check_by_id = "SELECT * FROM passwords WHERE NAME = ?;";
    sqlite3_stmt* stmt;
    
    exit = sqlite3_prepare_v2(db, check_by_id.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare select statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return {};
    }

    sqlite3_bind_text(stmt, 1, input.c_str(), -1, SQLITE_TRANSIENT);

    vector<map<string, string>> all_rows;
    map<string,string> row;
    map<string,string> result;

    for(int i = 0; i < 99; i++) {
        exit = sqlite3_step(stmt);
        if (exit == SQLITE_ROW) {
            int fetched_id = sqlite3_column_int(stmt, 0);
            const unsigned char* name = sqlite3_column_text(stmt, 1);
            const unsigned char* password = sqlite3_column_text(stmt, 2);

            row.clear();
            row["id"] = to_string(fetched_id);
            row["name"] = name ? reinterpret_cast<const char*>(name): "";
            row["password"] = password ? reinterpret_cast<const char*>(password): "";
            all_rows.push_back(row);

            cout << "id: " << fetched_id << " name: " << name << endl;
        } else if (exit == SQLITE_DONE) {
            if (i == 0) {
                cout << "Password name now found" << endl;
            }
            else if (i > 1) {
                int id;
                cout << "Give id of the password you would like to copy: ";
                cin >> id;
                
                for (const auto& result_row : all_rows) {
                    auto id_check_row = result_row.find("id");
                    if (id_check_row != result_row.end()) {
                        int id_check = stoi(id_check_row->second);
                        if (id == id_check) {
                            result = result_row;
                            break;
                        }
                    }
                }
            } else {
                result = row;
            }
            break;
        }else {
            cerr << "ERROR in selecting password: " << sqlite3_errmsg(db) << endl;
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return {};
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

int check_credentials(string username, string password) {
    int result = 0;
    string login_username, login_password;

    cout << "Enter username: ";
    cin >> login_username;

    cout << "Enter password: ";
    cin >> login_password;

    if (username == login_username && password == login_password) {
        result = 1;
        cout << "correct credentials" << endl;
        return result;
    } else {
        cout << "username or password is incorrect" << endl;
        return result;
    }
}

pair<string, string> generate_new_password(){
    string password_tag, password;
    int password_length;
    const string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!#¤%&";
    auto seed = chrono::steady_clock::now().time_since_epoch().count();
    mt19937 generator(seed);

    uniform_int_distribution<> distribution(0, characters.size() - 1);

    cout << "Give tag for the password: ";
    cin >> password_tag;

    cout << "Give password lenght: ";
    cin >> password_length;

    cout << "Generating password" << endl;
    string random_string;
    for (int i = 0; i < password_length; ++i) {
        random_string += characters[distribution(generator)];
    }

    return make_pair(password_tag, random_string);
}

bool add_password_to_database(string tag, string password) {
    sqlite3* db;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for credential check" << sqlite3_errmsg(db) << endl;
        return false;
    }

    string password_insert ("INSERT INTO PASSWORDS (NAME, PASSWORD) VALUES(?, ?);");
    sqlite3_stmt* stmt;

    exit = sqlite3_prepare_v2(db, password_insert.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare insert statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, tag.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

    exit = sqlite3_step(stmt);

    if (exit != SQLITE_DONE) {
        cerr << "ERROR in new password insert: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}

void new_password() {
    string password, tag;
    pair<string, string> np;
    bool db_add;

    np = generate_new_password();
    tag = np.first;
    password = np.second;
    db_add = add_password_to_database(tag, password);
    if (db_add == true) {
        cout << "new password created under tag " << tag << endl;
        copy_to_clipboard(password);
    } else {
        cout << "Failed to generate password for " << tag << endl;
    }
}

int sqlite_data_base_creation() {
    sqlite3* db;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    } else {
        cout << "Opened SQLite database successfully!\n";
    }
    string db_password_table =  "CREATE TABLE IF NOT EXISTS PASSWORDS(ID INTEGER PRIMARY KEY AUTOINCREMENT," \
                                "NAME STRING NOT NULL," \
                                "PASSWORD STRING NOT NULL)";
    char* message_error;
    exit = sqlite3_exec(db, db_password_table.c_str(), NULL, 0, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in Creating password table: " << message_error << endl;
        sqlite3_free(message_error);
        message_error = nullptr;
        return 1;
    } else{
        cout << "Password table was successfully created" << endl;
    }
    string db_user_credentials_table =  "CREATE TABLE IF NOT EXISTS CREDENTIALS(ID INTEGER PRIMARY KEY AUTOINCREMENT," \
                                        "TABLE_NAME TEXT NOT NULL," \
                                        "USERNAME TEXT NOT NULL," \
                                        "PASSWORD TEXT NOT NULL);";
    exit = sqlite3_exec(db, db_user_credentials_table.c_str(), NULL, 0, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in Creating credentials table: " << message_error << endl;
        sqlite3_free(message_error);
        message_error = nullptr;
        return 1;
    } else{
        cout << "Credentials table was successfully created" << endl;
    }
    sqlite3_close(db);
    return 0;
}

int callback(void* data, int argc, char** argv, char** azColName) {
    // Data is the return value to calling function
    // Number of columns in the current row
    // Array of strings representing the values of each column in the current row
    // Array of strings representing the names of each column in the current row
    auto* results = static_cast<vector<map<string, string>>*>(data);

    map<string, string> row;

    for (int i = 0; i < argc; i++) {
        string key = azColName[i];
        string value = (argv[i] ? argv[i] : "NULL");

        row[key] = value;
    }

    results->push_back(row);

    return 0;
}


bool check_credentials_from_database(string& out_username, string& out_password) {
    sqlite3* db;
    vector<map<string, string>> result_rows;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for credential check" << sqlite3_errmsg(db) << endl;
        return false;
    }

    string check_credentials = "SELECT * FROM credentials WHERE TABLE_NAME = 'main'";
    char* message_error;
    exit = sqlite3_exec(db, check_credentials.c_str(), callback, &result_rows, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in main credentials table select" << endl;
        sqlite3_free(message_error);
        sqlite3_close(db);
        message_error = nullptr;
        return false;
    }

    sqlite3_close(db);

    if (result_rows.empty()) {
        cout << "creadentials is empty" << endl;
        return false;
    }

    for (const auto& row : result_rows) {
        auto it_user = row.find("USERNAME");
        auto it_pass = row.find("PASSWORD");

        if (it_user != row.end() && !it_user->second.empty() &&
            it_pass != row.end() && !it_pass->second.empty()) {
            out_username = it_user->second;
            out_password = it_pass->second;
            return true;
        }
    }

    return false;
}

void create_credentials_to_db() {
    sqlite3* db;
    char* messageError;

    cout << "creating credentials" << endl;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for  creating credentials" << sqlite3_errmsg(db) << endl;
        return;
    }

    string username, password;

    login(username, password);
    cout << "username " << username << endl;
    cout << "password " << password << endl;
    string sql_credentials("INSERT INTO credentials (TABLE_NAME, USERNAME, PASSWORD) VALUES(?, ?, ?);");
    sqlite3_stmt* stmt;

    exit = sqlite3_prepare_v2(db, sql_credentials.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare insert statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_text(stmt, 1, "main", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, password.c_str(), -1, SQLITE_TRANSIENT);

    exit = sqlite3_step(stmt);

    if (exit != SQLITE_DONE) {
        cerr << "Login credential insert ERROR" << sqlite3_errmsg(db) << endl;
        sqlite3_free(messageError);
    }
    else {
        cout << "Login credentials added" << endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void get_password() {
    string search_input;
    bool str_value = false;
    map<string, string> result;

    cout << "Input password id or name: ";
    cin >> search_input;

    for (char ch : search_input) {
        if (!(ch >= 48 && ch <= 57)) {
            str_value = true;
            break;
        }
    }

    if (str_value != true) {
        result = search_by_id(search_input);
        if (!result.empty()) {
            auto pw_entry = result.find("password");
            string password = pw_entry->second;
            copy_to_clipboard(password);
        }
     } else {
        result = search_by_name(search_input);
        if (!result.empty()) {
            auto pw_entry = result.find("password");
            string password = pw_entry->second;
            copy_to_clipboard(password);
        }
    }
}

void delete_entry(int id) {
    sqlite3* db;
    char* message_error;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for deleting passwords" << sqlite3_errmsg(db) << endl;
        return;
    }

    string delete_password = "DELETE FROM passwords WHERE ID = ?;";
    sqlite3_stmt* stmt;

    exit = sqlite3_prepare_v2(db, delete_password.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare select statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, id);

    exit = sqlite3_step(stmt);

    if (exit == SQLITE_DONE) {
        cout << "Entry deletion compelete" << endl;
    } else {
        cerr << "Entry deletion FAILED" << sqlite3_errmsg(db) << endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return;
}

void delete_password() {
    string input;
    bool str_value = false;
    map<string, string> result;
    cout << "For deleting give entry id or name: ";
    cin >> input;

    for (char ch : input) {
        if (!(ch >= 48 && ch <= 57)) {
            str_value = true;
            break;
        }
    }

    if (str_value != true) {
        int int_input = stoi(input);
        delete_entry(int_input);
    } else {
        result = search_by_name(input);
        if (!result.empty()) {
            auto id_entry = result.find("id");
            int id = stoi(id_entry->second);
            delete_entry(id);
        }
    }
}

void browse_passwords() {
    sqlite3* db;
    char* message_error;
    vector<map<string, string>> result_rows;

    int exit = sqlite3_open("database/passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for browsing passwords" << sqlite3_errmsg(db) << endl;
        return;
    }

    string get_passwords = "SELECT * from passwords;";

    exit = sqlite3_exec(db, get_passwords.c_str(), callback, &result_rows, &message_error);

    if (exit != SQLITE_OK) {
        cerr << "ERROR in selecting from passwords table" << message_error << endl;
        sqlite3_free(message_error);
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);

    if (result_rows.empty()) {
        cout << "Password storage is empty" << endl;
        return;
    }

    for (const auto& row : result_rows) {
        auto id = row.find("ID");
        auto tag = row.find("NAME");
        if (id != row.end() && tag != row.end()) {
            cout << id->second << ": " << tag->second << endl;
        }
    }
    return;
}

int main () {
    int result, check_login, db_check, db_credentials, start;
    string login_username, login_password;
    string username, password;

    start = sqlite_data_base_creation();
    if (start != 0) {
        return 0;
    }

    for (int i = 0; i < 2; i++) {
        bool found = check_credentials_from_database(username, password);
        if (!found) {
            cout << "Credentials not found, creating new..." << endl;
            create_credentials_to_db();
            continue;
        } else {
            break;
        }
    }
    cout << "Login to password manager" << endl;
    check_login = check_credentials(username, password);
    if (check_login == 1) {
        int action;
        cout << "What would you like to do" << endl;
        cout << "1. Get password by id or name" << endl;
        cout << "2. Browse passwords" << endl;
        cout << "3. Generate passwords" << endl;
        cout << "4. Delete password by name" << endl;
        cout << "Input number for the action you would like to do: ";
        cin >> action;
        switch(action) {
            case 1:
                get_password();
            case 2:
                browse_passwords();
            case 3:
                new_password();
            case 4:
                delete_password();
            default:
                cout << "Invalid action given: " << action << endl;
        }
    } else {
        return 0;
    }
}