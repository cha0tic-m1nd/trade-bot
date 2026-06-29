#include "test.h"


signal::signal(std::unique_ptr<indicator> ind) : ind(std::move(ind))
{}

char signal::action(std::vector<candle>& data, int index) {}