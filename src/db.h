#include <memory>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>


constexpr char FRAME_M1 = 0, FRAME_M5 = 1, FRAME_M15 = 2, FRAME_M30 = 3, FRAME_H1 = 4, FRAME_H4 = 5,
               FRAME_D = 6;
constexpr char FEG = 0, INDX = 1, SPBFUT = 2, TQBR = 3, CETS = 4, CETS_FX = 5;

std::string get_frame(char frame);
std::string get_classcode(char class_code);
class Slice
{
    double* arr;
    int size;
    int curr_zero;

public:
    Slice();
    Slice(int size);
    double operator[](int i);
    void rotate(int shift);
    void shift_mod(double new_end);
    void resize(int new_size);
};
class Date
{
    /* copy of struct tm with normal year & month numeration, time is represented as minutes since
     * 00:00*/
    std::tm date;
    short _min;

public:
    Date();
    Date(int year, int month, int day);
    int year() const;
    int month() const;
    int day() const;
    short min() const;
    int mm() const;
    int hh() const;
    std::tm* get_tm();
    bool operator%(Date other) const;
    bool operator==(Date other) const;
    friend Date operator<(Date date, int days);
    Date& operator<<(int days);
};
class DB_day
{
    /* represents a full day in the loaded db */

public:
   Date date;
   char frame;
   std::vector<double> open, close, high, low;

   DB_day();

   DB_day(char frame, Date date);

   DB_day(char frame, Date date, char reserve_shape);

   void add_bar(double add_open, double add_high, double add_low, double add_close);

   const std::vector<double>& get_data(char source) const;
};
class DB_sym
{
    char class_code;
    std::string ticker;
    std::vector<DB_day> days;
    void load_day(char frame, Date& day, int flag);

public:
    DB_sym(char class_code, std::string ticker);

    int operator<<(std::pair<char, Date>& day);
    char get_class_code() const;

    std::string get_class_code_str() const;

    const std::string& get_ticker() const;

    void print_days();
    void load_day(char frame, Date& day, bool update);
    const DB_day& get_day(char frame, Date& day);
    void sort();
};
class Database
{
public:
    std::vector<std::shared_ptr<DB_sym>> db;
    Database();
    std::shared_ptr<DB_sym> operator()(char class_code, const std::string& ticker);
    std::shared_ptr<DB_sym> add_symbol(DB_sym symbol);
};



