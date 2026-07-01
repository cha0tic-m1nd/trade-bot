#include "loader.h"
#include <numeric>

template<typename T>
concept Candle = requires (T c) { { c.open }  -> std::convertible_to<double>;
    { c.high }  -> std::convertible_to<double>;
    { c.low }   -> std::convertible_to<double>;
    { c.close } -> std::convertible_to<double>;};

template<typename T>
concept CandleRange = std::ranges::range<T> && Candle<std::ranges::range_value_t<T>>;

template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept NumericRange = std::ranges::range<T> && Numeric<std::ranges::range_value_t<T>>;


class indicator
{
protected:
    unsigned int length;
public:
    virtual ~indicator() = default;

    indicator()
    {
        this->length = 0;
    }
    friend class signal;
};

class sma : public indicator
{
public:
    sma(unsigned int length)
    {
        this->length = length;
    }

    auto calc(NumericRange auto&& source)
    {
        return source 
                | std::views::slide(this->length) 
                | std::views::transform([this](auto&& window) { return std::ranges::fold_left(window, 0., std::plus<double>()) / this->length; });
    }
};


class stoch : public indicator
{
public:
    stoch(unsigned int length)
    {
        this->length = length;
    }

    auto calc(NumericRange auto&& source_close, NumericRange auto&& source_high, NumericRange auto&& source_low)
    {
        auto max_highs  = source_high 
                                    | std::views::slide(this->length)
                                    | std::views::transform([](auto&& window) {
                                          return std::ranges::max(window);
                                      });

        auto min_lows   = source_low 
                                    | std::views::slide(this->length)
                                    | std::views::transform([](auto&& window) {
                                          return std::ranges::min(window);
                                      });
        auto cur_closes = source_close 
                                    | std::views::slide(this->length)
                                    | std::views::transform([](auto&& window) {
                                          return window.back(); 
                                      });
        return std::views::zip_transform([](double close, double high, double low){
                                                                                    double range = high - low;
                                                                                    if (range == 0.) return 0.5;
                                                                                    return (close - low) / range;}, 
                                                                                    cur_closes, max_highs, min_lows);
    }
    

    auto calc(CandleRange auto&& source)
    {
        return source | std::views::slide(this->length) 
                    | std::views::transform([](auto&& window) {
                        auto high = std::ranges::max(window | std::views::transform(&candle::high));
                        auto low  = std::ranges::min(window | std::views::transform(&candle::low ));
                        auto range = high - low;
                        if (range == 0.) return 0.5;
                        return (window.back().close - low) / range; });
    }
    
};
/*
template <std::ranges::range S, std::ranges::range R>
class atr : public indicator<S, R>
{
public:
    atr(int length) : length(length)
    {
        // source: o = high - low, h = high - open, l = low - open, c = close - open
    }

    R calc(S&& source)
    {

        source | std::views::slide(length) | std::views::transform([length](auto&& window) {

            return std::ranges::fold_left(window, 0., std::plus<double>()) / length; });
        res["full"] = std::ranges::max(arr) - std::ranges::min(arr);
        return res;
    }
};
*/