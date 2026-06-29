#include "indicators.h"

indicator::indicator()
{
    this->length = 0;
}

sma::sma(unsigned int length, char source = 'c')
{
    /* source: c = close (default), o = open, l = low, h = high */
    this->length = length;
    this->source = source;
}

const nlohmann::json sma::calc(const std::vector<candle>& arr, unsigned int index)
{   
    nlohmann::json res = nlohmann::json();
    if (index >= arr.size() || index < length)
    {
        return res;
    }
    double s = 0;
    for (unsigned int i = 0; i < length; i++)
    {
        s += arr[index - i].get_data(source);
    }
    res["sma"] = s / length;
    return res;
}

const nlohmann::json sma::calc(const std::vector<double>& arr, unsigned int index)
{
    nlohmann::json res = nlohmann::json();
    if (index >= arr.size() || index < length)
    {
        return res;
    }
    double s = 0;
    for (unsigned int i = 0; i < length; i++)
    {
        s += arr[index - i];
    }
    res["sma"] = s / length;
    return res;
}

stoch::stoch(unsigned int length)
{
    this->length = length;
}

const nlohmann::json stoch::calc(const std::vector<candle>& arr, unsigned int index)
{
    nlohmann::json res = nlohmann::json();
    if (index >= arr.size() || index < length)
    {
        return res;
    }
    double high = arr[index].high, low = arr[index].low;
    for (unsigned int i = 0; i < length; i++)
    {
        high = arr[index - i].high > high ? arr[index - i].high : high;
        low = arr[index - i].low < low ? arr[index - i].low : low;
    }
    res["stoch"] = (arr[index].close - low) / (high - low);
    return res;
}

const nlohmann::json stoch::calc(const std::vector<double>& arr, unsigned int index)
{
    nlohmann::json res = nlohmann::json();
    if (index >= arr.size() || index < length)
    {
        return res;
    }
    double high = arr[index], low = arr[index];
    for (unsigned int i = 0; i < length; i++)
    {
        high = arr[index - i] > high ? arr[index - i] : high;
        low = arr[index - i] < low ? arr[index - i] : low;
    }
    res["stoch"] = (arr[index] - low) / (high - low);
    return res;
}

atr::atr()
{
    /* source: o = high - low, h = high - open, l = low - open, c = close - open */
    this->length = 0;
}

const nlohmann::json atr::calc(const std::vector<candle>& arr, unsigned int index)
{
    nlohmann::json res = nlohmann::json();
    double f, h = 0;
    for (auto i : arr)
    {
        f += i.high - i.low;
        h += i.high - i.open;
    }
    f /= arr.size();
    h /= arr.size();
    res["full"] = f;
    res["high"] = h;
    res["low"] = h - f;

    /* TODO: add mean-squared deviation */
    return res;
}

const nlohmann::json atr::calc(const std::vector<double>& arr, unsigned int index)
{
    nlohmann::json res = nlohmann::json();
    double l, h = arr[0];
    for (auto i : arr)
    {
        h = i > h ? i : h;
        l = i < h ? i : l;
    }
    res["full"] = h - l;
    return res;
}