#include <cstring>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <ranges>

constexpr char FRAME_M1 = 0, FRAME_M5 = 1, FRAME_M15 = 2, FRAME_M30 = 3, FRAME_H1 = 4, FRAME_H4 = 5,
               FRAME_D = 6;
constexpr char FEG = 0, INDX = 1, SPBFUT = 2, TQBR = 3, CETS = 4, CETS_FX = 5;

std::string get_frame(char frame);
std::string get_classcode(char class_code);
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::chrono::year_month_day shift(const std::chrono::year_month_day& start, int days);

class candle
{
    candle();
    std::string get_date_str(bool shift) const;
    std::string get_time_str(bool shift) const;
    friend std::ostream& operator<<(std::ostream& os, const candle& cn);
    friend class DB_manager;

public:
    candle(char class_code,
           char frame,
           std::string ticker,
           time_t date,
           double open,
           double high,
           double low,
           double close,
           double volume);
    char class_code, frame;
    std::string ticker;
    time_t date;
    double open, high, low, close, volume;
    double get_data(char source) const;
    
};

std::ostream& operator<<(std::ostream& os, const candle& cn);

class Auth
{
    std::string token, client_id, refresh_token;
    std::time_t token_expiry;
    const std::string base_url = "https://be.broker.ru";
    const std::string token_endpoint =
            "/trade-api-keycloak/realms/tradeapi/protocol/openid-connect/token";

public:
    Auth();
    std::string get_access_token();
};

class DB_manager
{
    SQLite::Database db;
    std::vector<candle> buf;
    time_t parse_time(const std::string& s) const;
    time_t parse_time_TZ(const std::string& s) const;
    std::chrono::system_clock::time_point parse_date_tp(const std::string& s) const;
    int pack(char frame) const;
    
    int load_pack(char class_code, const std::string& ticker, char frame, const std::chrono::year_month_day& start, const std::chrono::year_month_day& end);
    int load_pack(char class_code, const std::string& ticker, char frame, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end);

public:
    DB_manager();
    int insert_data(const std::vector<candle>& data, bool update);
    std::vector<candle> get_day(char class_code, const std::string& ticker, char frame, const std::string& day) const;
    std::vector<candle> get_range(
            char class_code,
            const std::string& ticker,
            char frame,
            const std::string& start,
            const std::string& end,
            bool weekend = false) const;
    int load_data(char class_code, const std::string& ticker, char frame, const std::string& day);
    int load_data(char class_code, const std::string& ticker, char frame, const std::string& start, const std::string& end);
    int load_data(char class_code, const std::string& ticker, char frame, const std::chrono::year_month_day& day);
    int load_data(char class_code, const std::string& ticker, char frame, const std::chrono::year_month_day& start, const std::chrono::year_month_day& end);

    DB_manager(const DB_manager&) = delete;
    DB_manager& operator=(const DB_manager&) = delete;
};