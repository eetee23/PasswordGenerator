#include "iostream"
#include "random"
#include "chrono"
#include "string"
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
    cout << "Row found" << endl;
    cout << "data:" << data << endl;
    cout << "argc: " << argc << endl;
    cout << "argv: " << argv << endl;
    cout << "azColName: " << azColName << endl;
    if (argc > 0 && argv[0]) {
        string* result = static_cast<string*>(data);
        *result = argv[0];
    }
    return 0;
}

int check_credentials_from_database() {
    sqlite3* db;
    string password;

    int exit = sqlite3_open("passwords.db", &db);
    
    if (exit != SQLITE_OK) {
        cerr << "ERROR opening database for credential check" << sqlite3_errmsg(db) << endl;
        return 1;
    }

    string check_credentials = "SELECT * FROM credentials WHERE TABLE_NAME = 'main'";
    char* message_error;
    exit = sqlite3_exec(db, check_credentials.c_str(), callback, &password, &message_error);
    if (exit != SQLITE_OK) {
        cerr << "ERROR in main credentials table select" << endl;
        sqlite3_free(message_error);
        message_error = nullptr;
    }

    if (password.empty()) {
        cout << "password is empty" << endl;
        create_credentials_to_db();    
        sqlite3_close(db);
        return 1;
    }


    sqlite3_close(db);
    return 0;
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

    cout << "Creating credentials" << endl;

    login(username, password);

    string sql_credentials("INSERT INTO credentials (1, 'main', username,password) VALUES(?, ?, ?, ?);");
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
    // sqlite3* DB;

    // int exit = sqlite3_open("passwords_db", &DB);
    // if (exit =! SQLITE_OK) {
    //     cerr << "Error opening SQLITE3 DB: " << sqlite3_errmsg(DB) <<endl;
    // }
    
    
    db_check = sqlite_data_base_creation();

    if (db_check != 0) {
        cerr << "ERROR in database creation" << endl;
        return -1;
    }

    cout << "password manager" << endl;
    for (int i=0; i < 2; i++){
        db_credentials = check_credentials_from_database();
        if (db_credentials == 1) {
            continue;
        }
        else {
            break;
        }
    }
    login(login_username, login_password);

    cout << "given username: " << login_username << endl;
    cout << "given password: " << login_password << endl;

    check_login = check_credentials(login_username, login_password);
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