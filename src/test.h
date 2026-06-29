#include "indicators.h"
#include <memory>

constexpr char ACTION = 1, BUY = 2;

class signal
{
    std::unique_ptr<indicator> ind;
public:
    signal(std::unique_ptr<indicator> ind);
    virtual char action(std::vector<candle>& arr, int index);
};

