#include "iostream"
#include "random"
#include "chrono"
#include "string"
#include "vector"
#include "map"
#include <windows.h>
#include <sqlite3.h>

using namespace std;

// prototype
void create_credentials_to_db();

void login (string &login_username, string &login_password) {
    
    cout << "Enter username: ";
    cin >> login_username;

    cout << "Enter passsword: ";
    cin >> login_password;
}

int check_credentials(string username, string password) {
    int result = 0;

    if (username == "user" && password == "pass12!") {
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

void copy_to_clipboard(const string& random_password) {
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
        cout << "new password copied to clip board" << endl;
    } else {
        cout << "Failed to open clipbroad" << endl;
        GlobalFree(hMem);
    }
}

void new_password() {
    string random_password, tag;
    pair<string, string> np;

    np = generate_new_password();
    tag = np.first;
    random_password = np.second;
    cout << "new password created under tag " << tag << endl;
    copy_to_clipboard(random_password);
}

int sqlite_data_base_creation() {
    sqlite3* db;

    int exit = sqlite3_open("passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        return 1;
    } else {
        cout << "Opened SQLite database successfully!\n";
    }
    string db_password_table =  "CREATE TABLE IF NOT EXISTS PASSWORDS(ID INT PRIMARY KEY NOT NULL," \
                                "NAME STRING NOT NULL," \
                                "PASSWORD STRING NOT NULL)";
    char* message_error;
    exit = sqlite3_exec(db, db_password_table.c_str(), NULL, 0, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in Creating password table" << endl;
        sqlite3_free(message_error);
        message_error = nullptr;
        return 1;
    } else{
        cout << "Password table was successfully created" << endl;
    }
    string db_user_credentials_table =  "CREATE TABLE IF NOT EXISTS CREDENTIALS(ID INT PRIMARY KEY NOT NULL," \
                                        "TABLE_NAME STRING NOT NULL," \
                                        "USERNAME STRING NOT NULL," \
                                        "PASSWORD STRING NOT NULL)";
    exit = sqlite3_exec(db, db_user_credentials_table.c_str(), NULL, 0, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in Creating credentials table" << endl;
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

    int exit = sqlite3_open("passwords.db", &db);
    
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
        cout << "password is empty" << endl;
        create_credentials_to_db();    
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

    int exit = sqlite3_open("passwords.db", &db);

    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for  creating credentials" << sqlite3_errmsg(db) << endl;
        return;
    }

    string username, password;

    login(username, password);
    cout << "username " << username << endl;
    cout << "password " << password << endl;
    string sql_credentials("INSERT INTO credentials (ID, TABLE_NAME, USERNAME, PASSWORD) VALUES(?, ?, ?, ?);");
    sqlite3_stmt* stmt;

    exit = sqlite3_prepare_v2(db, sql_credentials.c_str(), -1, &stmt, nullptr);

    if (exit != SQLITE_OK) {
        cerr << "Failed to prepare insert statement" << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        return;
    }

    sqlite3_bind_int(stmt, 1, 1);
    sqlite3_bind_text(stmt, 2, "main", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, password.c_str(), -1, SQLITE_TRANSIENT);

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

// int get_password() {

// }

// int browse_password() {

// }

int main () {
    int result, check_login, db_check, db_credentials;
    string login_username, login_password;
    string username, password;

    for (int i = 0; i < 2; i++) {
        bool found = check_credentials_from_database(username, password);
        if (!found) {
            cout << "Credentials not found, creating new..." << endl;
            create_credentials_to_db();
            continue;  // try again
        } else {
            break;  // exit the loop, credentials found
        }
    }
    // login(login_username, login_password);

    cout << "given username: " << username << endl;
    cout << "given password: " << password << endl;

    check_login = check_credentials(username, password);
    if (check_login == 1) {
        int action;
        cout << "What would you like to do" << endl;
        cout << "1. Get old password by name" << endl;
        cout << "2. Browse passwords" << endl;
        cout << "3. Generate passwords" << endl;
        cout << "Input number for the action you would like to do: ";
        cin >> action;
        switch(action) {
            case 1:
                // get_password()                
            case 2:
                // browse_password()
            case 3:
                new_password();
            default:
                cout << "Invalid action given: " << action << endl;
        }
    } else {
        return 0;
    }
}