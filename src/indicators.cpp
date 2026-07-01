#include "indicators.h"

template <std::ranges::range T1, std::ranges::range T2>
indicator<T1, T2>::indicator()
{
    this->length = 0;
}

template <std::ranges::range S, std::ranges::range R>
sma<S, R>::sma(unsigned int length)
{
    /* source: c = close (default), o = open, l = low, h = high */
    this->length = length;
}

template <std::ranges::range S, std::ranges::range R>
R sma<S, R>::calc(S&& source)
{   
    return source | std::views::slide(length) | std::views::transform([length](auto&& window){ return std::ranges::fold_left(window, 0., std::plus<double>()) / length;});
}

template <std::ranges::range S, std::ranges::range R>
stoch<S, R>::stoch(unsigned int length)
{
    this->length = length;
}

template <std::ranges::range S, std::ranges::range R>
atr<S, R>::atr()
{
    /* source: o = high - low, h = high - open, l = low - open, c = close - open */
    this->length = 0;
}


template <std::ranges::range S, std::ranges::range R>
R atr<S, R>::calc(S&& source)
{

    source | std::views::slide(length) | std::views::transform([length](auto&& window){

        return std::ranges::fold_left(window, 0., std::plus<double>()) / length;});
    res["full"] = std::ranges::max(arr) - std::ranges::min(arr);
    return res;
}