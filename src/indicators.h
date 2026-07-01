#include "loader.h"
#include <numeric>

template <std::ranges::range S, std::ranges::range R>
class indicator
{
protected:
    unsigned int length;
public:
    virtual ~indicator() = default;

    indicator();
    friend class signal;
};

template <std::ranges::range S, std::ranges::range R>
class sma : public indicator<S, R>
{
public:
    sma(unsigned int length);
    R calc(S&& arr);
};

template <std::ranges::range S, std::ranges::range R>
class stoch : public indicator<S, R>
{
public:
    stoch(unsigned int length);
    R calc(S&& source);
};

template<typename T>
concept Candle = requires (T cn){cn.high; cn.low; cn.close; cn.open; cn.volume;};

template<typename T>
concept CandleRange = std::ranges::range<T> && Candle<std::ranges::range_value_t<T>>;

template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept NumericRange = std::ranges::range<T> && Numeric<std::ranges::range_value_t<T>>;

template <std::ranges::range S, std::ranges::range R>
class stoch : public indicator<S, R>
{
public:
    stoch(unsigned int length);
    R calc(S&& source_close, S&& source_high, S&& source_low) requires NumericRange<S>
    {
        if (index >= source.size() || index < length)
        {
            return res;
        }
        double high = std::ranges::max(std::ranges::subrange(source.begin() + index - length, source.begin() + index) | std::views::transform(&candle::high));
        double low = std::ranges::min(std::ranges::subrange(arr.begin() + index - length, arr.begin() + index) | std::views::transform(&candle::low));
        res["stoch"] = (arr[index].close - low) / (high - low);
        return res;
    }
    R calc(S&& source) requires CandleRange<S>
    {
        if (index >= arr.size() || index < length)
        {
            return res;
        }
        double high = std::ranges::max(std::ranges::subrange(arr.begin() + index - length, arr.begin() + index));
        double low = std::ranges::min(std::ranges::subrange(arr.begin() + index - length, arr.begin() + index));
        res["stoch"] = (arr[index] - low) / (high - low);
        return res;
    }
};

template <std::ranges::range S, std::ranges::range R>
class atr : public indicator<S, R>
{
public:
    atr();
    R calc(S&& source);
};