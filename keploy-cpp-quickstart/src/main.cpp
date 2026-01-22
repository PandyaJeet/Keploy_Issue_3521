#include <crow.h>
#include <pqxx/pqxx>
#include <string>
#include <cstdlib>
#include <iostream>

std::string get_env(const char* name, const char* default_val) {
    const char* val = std::getenv(name);
    return val ? val : default_val;
}

int main() {
    crow::SimpleApp app;

    std::string db_host = get_env("DB_HOST", "db");
    std::string db_port = get_env("DB_PORT", "5432");
    std::string db_name = get_env("DB_NAME", "keploy_db");
    std::string db_user = get_env("DB_USER", "postgres");
    std::string db_pass = get_env("DB_PASS", "postgres");

    std::string conn_str = "host=" + db_host + 
                           " port=" + db_port + 
                           " dbname=" + db_name + 
                           " user=" + db_user + 
                           " password=" + db_pass;

    // PostgreSQL connection
    // We'll wrap this in a retry loop or similar because in Docker, 
    // app might perform faster than db startup. Or valid error handling.
    // For quickstart, simple connection attempt.
    
    pqxx::connection* conn = nullptr;
    try {
         conn = new pqxx::connection(conn_str);
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to DB: " << e.what() << "\n";
        return 1;
    }

    if (!conn->is_open()) {
        std::cerr << "Failed to connect to DB\n";
        return 1;
    }

    // Create table if not exists
    try {
        pqxx::work txn(*conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS todos (
                id SERIAL PRIMARY KEY,
                task TEXT NOT NULL
            );
        )");
        txn.commit();
    } catch (const std::exception& e) {
         std::cerr << "Migration failed: " << e.what() << "\n";
         return 1;
    }

    // GET todos
    CROW_ROUTE(app, "/todos").methods("GET"_method)(
        [conn]() {
            try {
                pqxx::work txn(*conn);
                auto res = txn.exec("SELECT * FROM todos");

                crow::json::wvalue result;
                // Make result an empty array by default
                result = std::vector<crow::json::wvalue>(); 
                int i = 0;

                for (auto row : res) {
                    result[i]["id"] = row["id"].as<int>();
                    result[i]["task"] = row["task"].as<std::string>();
                    i++;
                }
                return crow::response(result);
            } catch (const std::exception& e) {
                return crow::response(500, e.what());
            }
        }
    );

    // POST todo
    CROW_ROUTE(app, "/todos").methods("POST"_method)(
        [conn](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("task"))
                return crow::response(400, "Invalid JSON");

            std::string task = body["task"].s();

            try {
                pqxx::work txn(*conn);
                txn.exec_params(
                    "INSERT INTO todos (task) VALUES ($1)",
                    task
                );
                txn.commit();
            } catch (const std::exception& e) {
                return crow::response(500, e.what());
            }

            return crow::response(201, "Inserted");
        }
    );

    app.port(8080).multithreaded().run();
}
