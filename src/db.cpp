#include "db.h"
/* MAKE CASE WITH DAY WHICH CAN'T BE LOADED IN DB_sym */

SQLite::Database dbase = SQLite::Database("trading_data.db");

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
/* Slice */
Slice::Slice()
{
    this->size = 1;
    this->arr = new double[1];
    this->curr_zero = 0;
}

Slice::Slice(int size)
{
    this->size = size;
    this->arr = new double[size];
    this->curr_zero = 0;
}

double Slice::operator[](int i)
{
    /* access to the i-th element relatively to [curr_zero] */
    return arr[(this->curr_zero + i) % this->size];
}

void Slice::rotate(int shift)
{
    /* rotates curr_zero to(curr_zero + shift) % size */
    this->curr_zero = (this->curr_zero + shift) % size;
}

void Slice::shift_mod(double new_end)
{
    /* replaces the [curr_zero] element and rotates the buffer by 1 */
    this->arr[this->curr_zero] = new_end;
    this->rotate(1);
}

void Slice::resize(int new_size)
{
    /* resizes the buffer by reinitialization
    removes all the elements after [new_size] if new_size < size
    copies all other elements
    curr_zero saves its position if curr_zero < new_size, otherwise becomes the last element of
    the new array
    */
    double* new_arr = new double[new_size];
    int sz = new_size < this->size ? new_size : this->size;
    for (int i = 0; i < sz; i++)
    {
        new_arr[i] = this->arr[i];
    }
    delete[] this->arr;
    this->size = new_size;
    this->arr = new_arr;
    this->curr_zero = curr_zero < new_size ? curr_zero : new_size - 1;
}
/* Date */
Date::Date()
{
    std::time_t t = std::time(nullptr);
    date = *std::localtime(&t);
    date.tm_sec = 0;
    _min = date.tm_min + date.tm_hour * 60;
}

Date::Date(int year, int month, int day)
{
    date = std::tm{};
    date.tm_year = year - 1900;
    date.tm_mon = month - 1;
    date.tm_mday = day;
    _min = 0;
}

int Date::year() const
{
    return date.tm_year + 1900;
}
int Date::month() const
{
    return date.tm_mon + 1;
}
int Date::day() const
{
    return date.tm_mday;
}
short Date::min() const
{
    /* returns minutes since 00:00 */
    return _min;
}
int Date::mm() const
{
    /* returns minutes in hh:mm representation */
    return _min % 60;
}
int Date::hh() const
{
    /* returns hours in hh:mm representation */
    return _min / 60;
}

std::tm* Date::get_tm()
{
    return &date;
}

bool Date::operator%(Date other) const
{
    /* operator"==" from ymd part */
    std::tm* oth = other.get_tm();
    if (oth->tm_mday == date.tm_mday && oth->tm_mon == date.tm_mon
        && oth->tm_year == date.tm_year)
    {
        return true;
    }
    return false;
}

bool Date::operator==(Date other) const
{
    std::tm* oth = other.get_tm();
    if (other.min() == _min && oth->tm_mday == date.tm_mday && oth->tm_mon == date.tm_mon
        && oth->tm_year == date.tm_year)
    {
        return true;
    }
    return false;
}

Date operator<(Date date, int days)
{
    Date new_date = date;
    new_date.date.tm_mday += days;
    std::time_t epoch_seconds = std::mktime(&(new_date.date));
    new_date.date = *std::localtime(&epoch_seconds);
    return new_date;
}

Date& Date::operator<<(int days)
{
    std::time_t epoch_seconds = std::mktime(&(this->date)) + days * 86400;
    this->date = *std::localtime(&epoch_seconds);
    return *this;
}
/* DB_day */
DB_day::DB_day()
{
    this->frame = FRAME_M1;
    this->date = Date();
    this->open = std::vector<double>();
    this->close = std::vector<double>();
    this->high = std::vector<double>();
    this->low = std::vector<double>();
}

DB_day::DB_day(char frame, Date date)
{
    this->frame = frame;
    this->date = date;
    this->open = std::vector<double>();
    this->close = std::vector<double>();
    this->high = std::vector<double>();
    this->low = std::vector<double>();
}

DB_day::DB_day(char frame, Date date, char reserve_shape)
{
    this->frame = frame;
    this->date = date;
    int rsrv;
    int sz;
    switch (reserve_shape)
    {
    case FEG:
        sz = 1080;
        break;
    case INDX:
        sz = 960;
        break;
    case SPBFUT:
        sz = 840;
        break;
    case TQBR:
        sz = 960;
        break;
    case CETS:
        sz = 540;
        break;
    case CETS_FX:
        sz = 840;
        break;
    default:
        sz = 1080;
        break;
    }
    switch (frame)
    {
    case FRAME_M1:
        rsrv = sz;
        break;
    case FRAME_M5:
        rsrv = sz / 5;
        break;
    case FRAME_M15:
        rsrv = sz / 15;
        break;
    case FRAME_M30:
        rsrv = sz / 30;
        break;
    case FRAME_H1:
        rsrv = sz / 60;
        break;
    case FRAME_H4:
        if (sz == 1080)
        {
            rsrv = 5;
        }
        if (sz == 960 || sz == 840)
        {
            rsrv = 4;
        }
        if (sz == 540)
        {
            rsrv = 3;
        }
        break;
    case FRAME_D:
        rsrv = 1;
        break;
    default:
        rsrv = sz;
        break;
    }
    this->open = std::vector<double>(rsrv);
    this->close = std::vector <double> (rsrv);
    this->high = std::vector<double>(rsrv);
    this->low = std::vector<double>(rsrv);
}

void DB_day::add_bar(double add_open, double add_high, double add_low, double add_close)
{
    this->open.push_back(add_open);
    this->close.push_back(add_close);
    this->high.push_back(add_high);
    this->low.push_back(add_low);
}

const std::vector<double>& DB_day::get_data(char source) const
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
/* DB_sym */
DB_sym::DB_sym(char class_code, std::string ticker)
{
    this->class_code = class_code;
    this->ticker = ticker;
    days = std::vector<DB_day>();
}

int DB_sym::operator<<(std::pair<char, Date>& day)
{
    /* checks if day is in db, returns index of day in db or -1 if day is not in db */
    for (int i = 0; i < days.size(); i++)
    {
        if (days[i].date % day.second && days[i].frame == day.first)
        {
            return i;
        }
    }
    return -1;
}

char DB_sym::get_class_code() const
{
    return class_code;
}

std::string DB_sym::get_class_code_str() const
{
    return get_classcode(class_code);
}

const std::string& DB_sym::get_ticker() const
{
    return ticker;
}

void DB_sym::print_days()
{
    for (auto i : days)
    {
        std::cout << i.date.year() << " " << i.date.month() << " " << i.date.day() << " "
                  << std::endl;
    }
}

void DB_sym::load_day(char frame, Date& day, bool update = true)
{
    std::pair<char, Date> d(frame, day);
    int index = *this << d;
    if (update || index == -1)
    {
        std::string fr;
        fr = get_frame(frame);
        SQLite::Statement query(
                dbase,
                "SELECT open, high, low, close FROM candles WHERE class_code = ? AND ticker = "
                "? AND timeframe = ? AND year = ? AND month = ? AND day = ? ORDER BY timestamp "
                "ASC");
        DB_day new_day = DB_day(frame, day);
        query.bind(1, get_classcode(class_code));
        query.bind(2, this->ticker);
        query.bind(3, fr);
        query.bind(4, day.year());
        query.bind(5, day.month());
        query.bind(6, day.day());
        std::cout << query.getExpandedSQL() << std::endl;
        while (query.executeStep())
        {
            new_day.add_bar(
                    query.getColumn(0),
                    query.getColumn(1),
                    query.getColumn(2),
                    query.getColumn(3));
        }

        if (index != -1)
        {
            days[index] = new_day;
        }
        else
        {
            days.push_back(new_day);
        }
        // for (auto i : days.back().close) std::cout << i << std::endl;
        return;
    }
}

void DB_sym::load_day(char frame, Date& day, int flag)
{
    std::string fr;
    fr = get_frame(frame);
    SQLite::Statement query(
            dbase,
            "SELECT open, high, low, close FROM candles WHERE class_code = ? AND ticker = "
            "? AND timeframe = ? AND year = ? AND month = ? AND day = ? ORDER BY timestamp "
            "ASC");
    DB_day new_day = DB_day(frame, day);
    query.bind(1, get_classcode(class_code));
    query.bind(2, this->ticker);
    query.bind(3, fr);
    query.bind(4, day.year());
    query.bind(5, day.month());
    query.bind(6, day.day());
    std::cout << query.getExpandedSQL() << std::endl;
    while (query.executeStep())
    {
        new_day.add_bar(
                query.getColumn(0),
                query.getColumn(1),
                query.getColumn(2),
                query.getColumn(3));
    }

    days.push_back(new_day);
}

const DB_day& DB_sym::get_day(char frame, Date& day)
{
    std::pair<char, Date> d(frame, day);
    int index = *this << d;
    if (index == -1)
    {
        load_day(frame, day, 1);
        index = days.size() - 1;
    }
    return days[index];
}

void DB_sym::sort()
{
    /* empty, needs to be written */
    return;
}
/* Database */
Database::Database()
{
    db = std::vector<std::shared_ptr<DB_sym>>();
}

std::shared_ptr<DB_sym> Database::operator()(char class_code, const std::string& ticker)
{
    for (auto i : db)
    {
        if (i->get_class_code() == class_code && i->get_ticker() == ticker)
        {
            return i;
        }
    }
    return nullptr;
}

std::shared_ptr<DB_sym> Database::add_symbol(DB_sym symbol)
{
    std::shared_ptr<DB_sym> res = (*this)(symbol.get_class_code(), symbol.get_ticker());
    if (res == nullptr)
    {
        std::shared_ptr<DB_sym> new_ = std::make_shared<DB_sym>(symbol);
        db.push_back(new_);
        return new_;
    }
    return res;
}

