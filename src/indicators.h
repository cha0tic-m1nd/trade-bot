#include "loader.h"

class indicator
{
protected:
    unsigned int length;
public:
    virtual ~indicator() = default;

    indicator();
    virtual const nlohmann::json calc(const std::vector<double>& arr, unsigned int index) = 0;
    virtual const nlohmann::json calc(const std::vector<candle>& arr, unsigned int index) = 0;
    friend class signal;
};

class sma : public indicator
{
    char source;
public:
    sma(unsigned int length, char source);
    using indicator::calc;
    const nlohmann::json calc(const std::vector<double>& arr, unsigned int index) override;
    const nlohmann::json calc(const std::vector<candle>& arr, unsigned int index) override;
};

class stoch : public indicator
{
public:
    stoch(unsigned int length);
    using indicator::calc;
    const nlohmann::json calc(const std::vector<double>& arr, unsigned int index) override;
    const nlohmann::json calc(const std::vector<candle>& arr, unsigned int index) override;
};

class atr : public indicator
{
public:
    atr();
    using indicator::calc;
    const nlohmann::json calc(const std::vector<double>& arr, unsigned int index) override;
    const nlohmann::json calc(const std::vector<candle>& arr, unsigned int index) override;
};