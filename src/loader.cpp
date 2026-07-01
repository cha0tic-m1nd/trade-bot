#include "loader.h"

constexpr int MAX_RETRIES = 5;

std::string get_frame(char frame)
{
    switch (frame)
    {
    case FRAME_M1:
        return "M1";
    case FRAME_M5:
        return "M5";
    case FRAME_M15:
        return "M15";
    case FRAME_M30:
        return "M30";
    case FRAME_H1:
        return "H1";
    case FRAME_H4:
        return "H4";
    case FRAME_D:
        return "D";
    default:
        return "M1";
    }
}

std::string get_classcode(char class_code)
{
    switch (class_code)
    {
    case FEG:
        return "FEG";
    case INDX:
        return "INDX";
    case SPBFUT:
        return "SPBFUT";
    case TQBR:
        return "TQBR";
    default:
        return "FEG";
    }
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::chrono::year_month_day shift(const std::chrono::year_month_day& start, int days)
{
    return std::chrono::year_month_day{std::chrono::sys_days{start} + std::chrono::days{days}};
}

candle::candle() {}

candle::candle(char class_code, char frame, std::string ticker, time_t date, 
    double open, double high, double low, double close, double volume)
    : class_code(class_code),
      frame(frame),
      ticker(ticker),
      date(date),
      open(open),
      high(high),
      low(low),
      close(close),
      volume(volume) 
{}

std::string candle::get_date_str(bool shift) const
{
    auto tp = std::chrono::system_clock::from_time_t(date);
    if (shift)
    {
        tp += std::chrono::hours(3);
    }
    std::string s = std::format("{:%Y-%m-%d}", tp);
    return s;
}

std::string candle::get_time_str(bool shift) const
{
    auto tp = std::chrono::system_clock::from_time_t(date);
    if (shift)
    {
        tp += std::chrono::hours(3);
    }
    std::string s = std::format("{:%H:%M:%S}", tp);
    return s;
}

double candle::get_data(char source) const
{
    switch (source)
    {
    case 'c':
        return close;
    case 'o':
        return open;
    case 'h':
        return high;
    case 'l':
        return low;
    default:
        throw std::invalid_argument(
                "wrong argument in DB_day::get_data(char source), possible arguments are: 'c', "
                "'l', 'o', 'h'");
    }
}

std::ostream& operator<<(std::ostream& os, const candle& cn)
{
    return os
        << get_classcode(cn.class_code) << ", " << cn.ticker << ", " << get_frame(cn.frame) << ", "
        << cn.get_date_str(false) << ", " << cn.get_time_str(false) << ", open: " << cn.open
        << ", high: " << cn.high << ", low: " << cn.low << ", close: " << cn.close
        << ", volume: " << cn.volume << std::endl;
}

Auth::Auth()
{
    token = "";
    token_expiry = 0;
    std::ifstream file("credentials.json");
    if (file.is_open())
    {
        using json = nlohmann::json;
        json data = json::parse(file);
        client_id = data["client_id"];
        refresh_token = data["refresh_token"];
    }
    else
    {
        throw std::filesystem::filesystem_error("File does not exist", 
            std::filesystem::path("credentials.json"), 
            std::make_error_code(std::errc::no_such_file_or_directory));
    }
}

std::string Auth::get_access_token()
{
    if (token != "" && token_expiry && token_expiry < std::time(nullptr))
    {
        return token;
    }

    CURL* curl;
    curl = curl_easy_init();
    if (curl)
    {
        struct curl_slist* headers = NULL;
        std::string data =
                "client_id=trade-api-write&grant_type=refresh_token&refresh_token=" + refresh_token;
        std::string responseString = std::string();

        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(
                curl,
                CURLOPT_URL,
                "https://be.broker.ru/trade-api-keycloak/realms/tradeapi/protocol/openid-connect/token");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        headers = curl_slist_append(headers, "Accept: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.data());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            try
            {
                // Parse raw string directly into a JSON object
                auto jsonResponse = nlohmann::json::parse(responseString);
                token_expiry = std::time(nullptr) + static_cast<int>(jsonResponse["expires_in"]) - 60;
                return jsonResponse["access_token"];
            }
            catch (const nlohmann::json::parse_error& e)
            {
                std::cerr << "JSON Parsing failed: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    else
    {
        curl_easy_cleanup(curl);
        throw std::runtime_error("curl init failed");
    }   
    return "";
}

DB_manager::DB_manager() : db("trading_data.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
    buf = std::vector<candle>();
    std::cout << "Initializing trading_data.db:" << std::endl;
    db.exec("CREATE TABLE IF NOT EXISTS candles ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "class_code INTEGER NOT NULL,"
               "ticker TEXT NOT NULL,"
               "timeframe INTEGER NOT NULL,"
               "date DATE,"
               "time TIME,"
               "open REAL,"
               "high REAL,"
               "low REAL,"
               "close REAL,"
               "volume REAL,"
               "UNIQUE (class_code, ticker, timeframe, date, time))");
    db.exec("CREATE INDEX IF NOT EXISTS idx_candle_lookup ON candles (class_code, ticker, timeframe, date)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_ticker_timeframe ON candles(ticker, timeframe)");
    std::cout << "trading_data.db has been initialized" << std::endl;
}

time_t DB_manager::parse_time(const std::string& s) const
{
    std::chrono::system_clock::time_point tp;

    std::istringstream iss{s};
    iss >> std::chrono::parse("%Y-%m-%d %H:%M:%S", tp);

    if (iss.fail())
    {
        throw std::runtime_error("Invalid timestamp");
    }

    return std::chrono::system_clock::to_time_t(tp);
}

time_t DB_manager::parse_time_TZ(const std::string& s) const
{
    std::chrono::system_clock::time_point tp;

    std::istringstream iss{s};
    iss >> std::chrono::parse("%Y-%m-%dT%H:%M:%SZ", tp);
    
    if (iss.fail())
    {
        throw std::runtime_error("Invalid timestamp");
    }
    return std::chrono::system_clock::to_time_t(tp);
}

std::chrono::system_clock::time_point DB_manager::parse_date_tp(const std::string& s) const
{
    std::chrono::system_clock::time_point tp;

    std::istringstream iss{s};
    iss >> std::chrono::parse("%Y-%m-%d", tp);

    if (iss.fail())
    {
        throw std::runtime_error("Invalid timestamp");
    }

    return tp;
}

int DB_manager::insert_data(const std::vector<candle>& data, bool update)
{
    if (data.empty())
    {
        return 0;
    }

    SQLite::Transaction tr(db);
    std::cout << "Transaction initialized" << std::endl;
    SQLite::Statement query(db, "");
    std::cout << "Query initialized" << std::endl;
    if (update)
    {
        query = SQLite::Statement(
                db,
                "INSERT OR REPLACE INTO candles\
                (class_code, ticker, timeframe, date, time, open, high, low, close, volume)\
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    }
    else
    {
        query = SQLite::Statement(db, "INSERT OR IGNORE INTO candles\
                (class_code, ticker, timeframe, date, time, open, high, low, close, volume)\
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    }
    //std::cout << "Query created: " << query.getQuery() << std::endl;
    for (auto i : data)
    {
        query.bind(1, i.class_code);
        query.bind(2, i.ticker);
        query.bind(3, i.frame);
        query.bind(4, i.get_date_str(true));
        query.bind(5, i.get_time_str(true));
        query.bind(6, i.open);
        query.bind(7, i.high);
        query.bind(8, i.low);
        query.bind(9, i.close);
        query.bind(10, i.volume);
        query.exec();
        query.reset();
    }
    tr.commit();
    std::cout << "Total changes in db: " << db.getTotalChanges() << std::endl;
    return db.getTotalChanges();
}

std::vector<candle> DB_manager::get_day(char class_code, const std::string& ticker, char frame, const std::string& day) const
{
    SQLite::Statement query = SQLite::Statement(db, 
        " SELECT date, time, open, high, low, close, volume \
          FROM candles WHERE class_code = ? AND ticker = ? AND timeframe = ? AND date(date) = ? \
          ORDER BY time ASC ");

    query.bind(1, class_code);
    query.bind(2, ticker);
    query.bind(3, frame);
    query.bind(4, day);

    std::vector<candle> res = std::vector<candle>();
    while (query.executeStep())
    {
        std::string str = std::string(query.getColumn(0).getText()) + " ";
        str += query.getColumn(1).getText();
        candle row = candle(
                class_code,
                frame,
                ticker,
                parse_time(str),
                static_cast<double>(query.getColumn(2)),
                static_cast<double>(query.getColumn(3)),
                static_cast<double>(query.getColumn(4)),
                static_cast<double>(query.getColumn(5)),
                static_cast<double>(query.getColumn(6)));
        res.push_back(row);
    }
    return res;
}

std::vector<candle> DB_manager::get_range(char class_code, const std::string& ticker, char frame, const std::string& start, const std::string& end, bool weekend) const
{
    SQLite::Statement query = SQLite::Statement(db, "");
    if (weekend)
    {
        query = SQLite::Statement(
                db,
                " SELECT date, time, open, high, low, close, volume \
          FROM candles WHERE "
                "class_code = ? AND ticker = ? AND timeframe = ? AND date >= ? AND date <= ? ORDER BY date ASC, time ASC ");
    }
    else
    {
        query = SQLite::Statement(
                db,
                " SELECT date, time, open, high, low, close, volume \
          FROM candles WHERE "
                "class_code = ? AND ticker = ? AND timeframe = ? AND date >= ? AND date <= ? AND "
                "strftime('%w', date) <= '5' AND strftime('%w', date) >= '1' ORDER BY date ASC, time ASC ");
    }
    //std::cout << query.getQuery() << std::endl;
    query.bind(1, class_code);
    query.bind(2, ticker);
    query.bind(3, frame);
    query.bind(4, start);
    query.bind(5, end);

    std::vector<candle> res = std::vector<candle>();

    while (query.executeStep())
    {
        std::string str = std::string(query.getColumn(0).getText()) + " ";
        str += query.getColumn(1).getText();
        candle row = candle(
                class_code,
                frame,
                ticker,
                parse_time(str),
                static_cast<double>(query.getColumn(2)),
                static_cast<double>(query.getColumn(3)),
                static_cast<double>(query.getColumn(4)),
                static_cast<double>(query.getColumn(5)),
                static_cast<double>(query.getColumn(6)));
        res.push_back(row);
    }
    return res;
}

int DB_manager::pack(char frame) const
{
    switch (frame)
    {
    case 0:
        return 1;
    case 1:
        return 5;
    case 2:
        return 15;
    case 3:
        return 30;
    case 4:
        return 60;
    case 5:
        return 240;
    case 6:
        return 1440;
    default:
        throw std::invalid_argument("invalid timeframe");
    }
}

int DB_manager::load_pack(char class_code, const std::string& ticker, char frame, const std::chrono::year_month_day& start, const std::chrono::year_month_day& end)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl)
    {
        std::string responseString = "";
        Auth x;
        std::chrono::system_clock::time_point startDate = std::chrono::sys_days{start};
        std::chrono::system_clock::time_point endDate = std::chrono::sys_days{end};
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        std::string endDay = std::format("{:%Y-%m-%dT%H:%M:%SZ}", endDate);
        std::string startDay = std::format("{:%Y-%m-%dT%H:%M:%SZ}", startDate);
        curl_easy_setopt(
                curl,
                CURLOPT_URL,
                ("https://be.broker.ru/trade-api-market-data-connector/api/v1/"
                 "candles-chart?classCode="
                 + get_classcode(class_code) + "&ticker=" + ticker + "&startDate=" + startDay
                 + "&endDate=" + endDay + "&timeFrame=" + get_frame(frame))
                        .c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(
                headers,
                ("Authorization: Bearer " + x.get_access_token()).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        for (int i = 0; i < MAX_RETRIES; i++)
        {
            std::cout << "attempt #" << i << std::endl;
            res = curl_easy_perform(curl);
            if (res == CURLE_OK)
            {
                try
                {
                    auto jsonResponse = nlohmann::json::parse(responseString);
                    for (auto i : jsonResponse["bars"])
                    {
                        buf.push_back(candle(
                                class_code,
                                frame,
                                ticker,
                                parse_time_TZ(i["time"]),
                                i["open"],
                                i["high"],
                                i["low"],
                                i["close"],
                                i["volume"]));
                    }
                }
                catch (const nlohmann::json::parse_error& e)
                {
                    std::cout << "JSON failed at parsing response: " << responseString << std::endl;
                    std::cerr << "JSON Parsing failed: " << e.what() << " at period: " << startDay
                              << " to " << endDay << std::endl;
                }
                break;
            }
            std::cout << "Retrying to load period from " << startDay << " to " << endDay << ", attempt #" << i << std::endl;
            std::this_thread::sleep_for(
                    std::chrono::milliseconds(100 * static_cast<int>(std::pow<int, int>(2, i))));
        }

        if (res != CURLE_OK)
        {
            std::cout << "Couldn't load data from " << startDay << " to " << endDay << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return 0;
    }
    curl_easy_cleanup(curl);
    return -2;
}

int DB_manager::load_pack(char class_code, const std::string& ticker, char frame, const std::chrono::system_clock::time_point& start, const std::chrono::system_clock::time_point& end)
{
    CURL* curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl)
    {
        std::string responseString = "";
        Auth x;
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        std::string endDay = std::format("{:%Y-%m-%dT%H:%M:%SZ}", end);
        std::string startDay = std::format("{:%Y-%m-%dT%H:%M:%SZ}", start);
        curl_easy_setopt(
                curl,
                CURLOPT_URL,
                ("https://be.broker.ru/trade-api-market-data-connector/api/v1/"
                 "candles-chart?classCode="
                 + get_classcode(class_code) + "&ticker=" + ticker + "&startDate=" + startDay
                 + "&endDate=" + endDay + "&timeFrame=" + get_frame(frame))
                        .c_str());

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(
                headers,
                ("Authorization: Bearer " + x.get_access_token()).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);
        for (int i = 0; i < MAX_RETRIES; i++)
        {
            res = curl_easy_perform(curl);
            if (res == CURLE_OK)
            {
                try
                {
                    auto jsonResponse = nlohmann::json::parse(responseString);
                    for (auto i : jsonResponse["bars"])
                    {
                        buf.push_back(candle(
                                class_code,
                                frame,
                                ticker,
                                parse_time_TZ(i["time"]),
                                i["open"],
                                i["high"],
                                i["low"],
                                i["close"],
                                i["volume"]));
                    }
                }
                catch (const nlohmann::json::parse_error& e)
                {
                    std::cout << "JSON failed at parsing response: " << responseString << std::endl;
                    std::cerr << "JSON Parsing failed: " << e.what() << " at period: " << startDay
                              << " to " << endDay << std::endl;
                }
                break;
            }
            std::cout << "Retrying to load period from " << startDay << " to " << endDay << ", attempt #" << i << std::endl;
            std::this_thread::sleep_for(
                    std::chrono::milliseconds(100 * static_cast<int>(std::pow<int, int>(2, i))));
        }

        if (res != CURLE_OK)
        {
            std::cout << "Couldn't load data from " << startDay << " to " << endDay << std::endl;
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return 0;
    }
    curl_easy_cleanup(curl);
    return -2;
}

int DB_manager::load_data(char class_code, const std::string& ticker, char frame, const std::string& day)
{
    
    std::chrono::system_clock::time_point start = parse_date_tp(day);
    std::chrono::system_clock::time_point end = start + std::chrono::days(1);
    
    load_pack(class_code, ticker, frame, start, end);
    int ret_code = insert_data(buf, true);
    buf.clear();
    return ret_code;
}

int DB_manager::load_data(
        char class_code,
        const std::string& ticker,
        char frame,
        const std::string& start,
        const std::string& end)
{
    std::chrono::system_clock::time_point startDate = parse_date_tp(start);
    std::chrono::system_clock::time_point endDate = parse_date_tp(end);
    std::chrono::days p = std::chrono::days(pack(frame));
    while (startDate + p < endDate)
    {
        load_pack(class_code, ticker, frame, startDate, startDate + p);
        startDate += p;
    }
    load_pack(class_code, ticker, frame, startDate, endDate);
    int ret_code = insert_data(buf, true);
    buf.clear();
    return ret_code;
}

int DB_manager::load_data(
        char class_code,
        const std::string& ticker,
        char frame,
        const std::chrono::year_month_day& day)
{
    return 1;
}
int DB_manager::load_data(
        char class_code,
        const std::string& ticker,
        char frame,
        const std::chrono::year_month_day& start,
        const std::chrono::year_month_day& end)
{
    std::chrono::system_clock::time_point startDate = std::chrono::sys_days(start);
    std::chrono::system_clock::time_point endDate = std::chrono::sys_days(end);
    std::chrono::days p = std::chrono::days(pack(frame));
    while (startDate + p < endDate)
    {
        load_pack(class_code, ticker, frame, startDate, startDate + p);
        startDate += p;
    }
    load_pack(class_code, ticker, frame, startDate, endDate);
    int ret_code = insert_data(buf, true);
    buf.clear();
    return ret_code;
}