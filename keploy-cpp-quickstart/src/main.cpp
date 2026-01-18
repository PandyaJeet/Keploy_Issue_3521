#include "crow.h"
#include <pqxx/pqxx>
#include <vector>
#include <string>

std::string DB_CONN = "host=db user=postgres password=postgres dbname=keploy_db";

int main() {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        return "Keploy C++ + DB Quickstart";
    });

    CROW_ROUTE(app, "/todos").methods("GET"_method)([]() {
        pqxx::connection c(DB_CONN);
        pqxx::work txn(c);

        auto rows = txn.exec("SELECT id, task FROM todos");

        crow::json::wvalue res;
        int i = 0;
        for (auto row : rows) {
            res[i]["id"] = row["id"].as<int>();
            res[i]["task"] = row["task"].c_str();
            i++;
        }
        return res;
    });

    CROW_ROUTE(app, "/todos").methods("POST"_method)(
        [](const crow::request& req) {
            auto body = crow::json::load(req.body);
            std::string task = body["task"].s();

            pqxx::connection c(DB_CONN);
            pqxx::work txn(c);

            txn.exec(
                "INSERT INTO todos (task) VALUES (" +
                txn.quote(task) + ")"
            );
            txn.commit();

            return crow::response(201);
        });

    app.port(8080).run();
}
