#include <crow.h>
#include <pqxx/pqxx>
#include <string>

int main() {
    crow::SimpleApp app;

    // PostgreSQL connection
    pqxx::connection conn(
        "host=db port=5432 dbname=keploy_db user=postgres password=postgres"
    );

    if (!conn.is_open()) {
        std::cerr << "Failed to connect to DB\n";
        return 1;
    }

    // Create table if not exists
    {
        pqxx::work txn(conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS todos (
                id SERIAL PRIMARY KEY,
                task TEXT NOT NULL
            );
        )");
        txn.commit();
    }

    // GET todos
    CROW_ROUTE(app, "/todos").methods("GET"_method)(
        [&conn]() {
            pqxx::work txn(conn);
            auto res = txn.exec("SELECT * FROM todos");

            crow::json::wvalue result;
            int i = 0;

            for (auto row : res) {
                result[i]["id"] = row["id"].as<int>();
                result[i]["task"] = row["task"].as<std::string>();
                i++;
            }
            return result;
        }
    );

    // POST todo
    CROW_ROUTE(app, "/todos").methods("POST"_method)(
        [&conn](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("task"))
                return crow::response(400, "Invalid JSON");

            std::string task = body["task"].s();

            pqxx::work txn(conn);
            txn.exec_params(
                "INSERT INTO todos (task) VALUES ($1)",
                task
            );
            txn.commit();

            return crow::response(201, "Inserted");
        }
    );

    app.port(8080).multithreaded().run();
}
