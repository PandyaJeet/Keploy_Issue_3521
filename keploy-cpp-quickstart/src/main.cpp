#include <crow.h>
#include <pqxx/pqxx>
#include <string>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string get_env(const char* name, const char* default_val) {
    const char* val = std::getenv(name);
    return val ? val : default_val;
}

/// Try to open a PostgreSQL connection, retrying up to `max_retries` times.
/// This is important in Docker where the app container may start before
/// PostgreSQL is fully ready.
static std::unique_ptr<pqxx::connection>
connect_with_retry(const std::string& conn_str, int max_retries = 10,
                   int delay_seconds = 2)
{
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        try {
            auto conn = std::make_unique<pqxx::connection>(conn_str);
            if (conn->is_open()) {
                std::cout << "[db] Connected to PostgreSQL (attempt "
                          << attempt << ")\n";
                return conn;
            }
        } catch (const std::exception& e) {
            std::cerr << "[db] Attempt " << attempt << "/" << max_retries
                      << " failed: " << e.what() << "\n";
        }
        if (attempt < max_retries) {
            std::cerr << "[db] Retrying in " << delay_seconds << "s …\n";
            std::this_thread::sleep_for(
                std::chrono::seconds(delay_seconds));
        }
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    crow::SimpleApp app;

    // ── Database configuration (overridable via env vars) ───────────────
    std::string db_host = get_env("DB_HOST", "localhost");
    std::string db_port = get_env("DB_PORT", "5432");
    std::string db_name = get_env("DB_NAME", "keploy_db");
    std::string db_user = get_env("DB_USER", "postgres");
    std::string db_pass = get_env("DB_PASS", "postgres");

    std::string conn_str = "host=" + db_host +
                           " port=" + db_port +
                           " dbname=" + db_name +
                           " user=" + db_user +
                           " password=" + db_pass;

    // ── Connect to PostgreSQL (with retry) ─────────────────────────────
    auto conn = connect_with_retry(conn_str);
    if (!conn) {
        std::cerr << "[db] Could not connect after retries – exiting.\n";
        return 1;
    }

    // ── Run migrations ─────────────────────────────────────────────────
    try {
        pqxx::work txn(*conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS todos (
                id    SERIAL PRIMARY KEY,
                task  TEXT NOT NULL,
                done  BOOLEAN NOT NULL DEFAULT FALSE
            );
        )");
        txn.commit();
        std::cout << "[db] Migration complete.\n";
    } catch (const std::exception& e) {
        std::cerr << "[db] Migration failed: " << e.what() << "\n";
        return 1;
    }

    // Shared raw pointer (connection is not copied; Crow handlers are
    // invoked sequentially per‑connection so this is safe for a demo).
    pqxx::connection* db = conn.get();

    // ── Health check ───────────────────────────────────────────────────
    CROW_ROUTE(app, "/health")
        .methods("GET"_method)([db]() {
            crow::json::wvalue res;
            res["status"] = "ok";
            try {
                pqxx::work txn(*db);
                txn.exec("SELECT 1");
                res["database"] = "connected";
            } catch (...) {
                res["database"] = "unavailable";
            }
            return crow::response(200, res);
        });

    // ── GET /todos — list all todos ────────────────────────────────────
    CROW_ROUTE(app, "/todos")
        .methods("GET"_method)([db]() {
            try {
                pqxx::work txn(*db);
                auto result = txn.exec(
                    "SELECT id, task, done FROM todos ORDER BY id");

                crow::json::wvalue::list items;
                for (const auto& row : result) {
                    crow::json::wvalue item;
                    item["id"]   = row["id"].as<int>();
                    item["task"] = row["task"].as<std::string>();
                    item["done"] = row["done"].as<bool>();
                    items.push_back(std::move(item));
                }

                crow::json::wvalue res(std::move(items));
                return crow::response(200, res);
            } catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                return crow::response(500, err);
            }
        });

    // ── GET /todos/<id> — get a single todo ────────────────────────────
    CROW_ROUTE(app, "/todos/<int>")
        .methods("GET"_method)([db](int id) {
            try {
                pqxx::work txn(*db);
                auto result = txn.exec_params(
                    "SELECT id, task, done FROM todos WHERE id = $1", id);

                if (result.empty()) {
                    crow::json::wvalue err;
                    err["error"] = "Todo not found";
                    return crow::response(404, err);
                }

                crow::json::wvalue item;
                item["id"]   = result[0]["id"].as<int>();
                item["task"] = result[0]["task"].as<std::string>();
                item["done"] = result[0]["done"].as<bool>();
                return crow::response(200, item);
            } catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                return crow::response(500, err);
            }
        });

    // ── POST /todos — create a new todo ────────────────────────────────
    CROW_ROUTE(app, "/todos")
        .methods("POST"_method)([db](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body || !body.has("task")) {
                crow::json::wvalue err;
                err["error"] = "Missing required field: task";
                return crow::response(400, err);
            }

            std::string task = body["task"].s();

            try {
                pqxx::work txn(*db);
                auto result = txn.exec_params(
                    "INSERT INTO todos (task) VALUES ($1) RETURNING id, task, done",
                    task);
                txn.commit();

                crow::json::wvalue item;
                item["id"]   = result[0]["id"].as<int>();
                item["task"] = result[0]["task"].as<std::string>();
                item["done"] = result[0]["done"].as<bool>();
                return crow::response(201, item);
            } catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                return crow::response(500, err);
            }
        });

    // ── PUT /todos/<id> — update an existing todo ──────────────────────
    CROW_ROUTE(app, "/todos/<int>")
        .methods("PUT"_method)([db](const crow::request& req, int id) {
            auto body = crow::json::load(req.body);
            if (!body) {
                crow::json::wvalue err;
                err["error"] = "Invalid JSON body";
                return crow::response(400, err);
            }

            try {
                pqxx::work txn(*db);

                // Check existence
                auto check = txn.exec_params(
                    "SELECT id FROM todos WHERE id = $1", id);
                if (check.empty()) {
                    crow::json::wvalue err;
                    err["error"] = "Todo not found";
                    return crow::response(404, err);
                }

                if (body.has("task")) {
                    std::string task = body["task"].s();
                    txn.exec_params(
                        "UPDATE todos SET task = $1 WHERE id = $2",
                        task, id);
                }
                if (body.has("done")) {
                    bool done = body["done"].b();
                    txn.exec_params(
                        "UPDATE todos SET done = $1 WHERE id = $2",
                        done, id);
                }

                auto result = txn.exec_params(
                    "SELECT id, task, done FROM todos WHERE id = $1", id);
                txn.commit();

                crow::json::wvalue item;
                item["id"]   = result[0]["id"].as<int>();
                item["task"] = result[0]["task"].as<std::string>();
                item["done"] = result[0]["done"].as<bool>();
                return crow::response(200, item);
            } catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                return crow::response(500, err);
            }
        });

    // ── DELETE /todos/<id> — delete a todo ─────────────────────────────
    CROW_ROUTE(app, "/todos/<int>")
        .methods("DELETE"_method)([db](int id) {
            try {
                pqxx::work txn(*db);
                auto result = txn.exec_params(
                    "DELETE FROM todos WHERE id = $1 RETURNING id", id);
                txn.commit();

                if (result.empty()) {
                    crow::json::wvalue err;
                    err["error"] = "Todo not found";
                    return crow::response(404, err);
                }

                crow::json::wvalue msg;
                msg["message"] = "Todo deleted";
                msg["id"]      = id;
                return crow::response(200, msg);
            } catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                return crow::response(500, err);
            }
        });

    // ── Start the server ───────────────────────────────────────────────
    std::cout << "[app] Starting on port 8080 …\n";
    app.port(8080).multithreaded().run();

    return 0;
}
