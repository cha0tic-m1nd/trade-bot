#include "indicators.h"

/* need an update for frame/class_code names in db (char-ify them)*/

/*
class Tester
{

};

void daytest(
        Symbol sym,
        Date day,
        int strategy, // bitwise sum
        char frame,
        double stop = 0.03,
        int stoch_length = 14,
        int k = 3,
        int d = 3,
        double stoch_fix = 0.5,
        bool debug_flag = false)
{
    double res = 0, pos = 0;
    sym.load_day(frame, day);
    int dead_time = stoch_length + k + d;
    std::vector<double> stoch_arr = std::vector<double>(k+d - 1);

    std::cout << "making stoch pre-init; stoch_length, k, d = " << stoch_length << ", " << k << ", " << d << std::endl;

    for (int i = 0; i < k + d - 1; i++)
    {
        std::cout << i << std::endl;
        stoch_arr[i] = sym.stoch(
                db[3][stoch_length + k + d - 3 + i],
                db[1].subspan(0, stoch_length + k + d - 3 + i),
                db[2].subspan(0, stoch_length + k + d - 3 + i),
                stoch_length);
        std::cout << "stoch arr [" << i << "] :" << stoch_arr[i] << std::endl;
    }
    
    double stoch_k2 = 0, stoch_k1 = 0, stoch_kcurr = 0;
    double stoch_d2 = 0, stoch_d1 = 0, stoch_dcurr = 0;

    stoch_k1 = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size() - 1), k);
    stoch_d1 = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size() - 1), d);
    stoch_kcurr = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size()), k);
    stoch_dcurr = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size()), d);
    int win = 0, loss = 0;
    std::cout << "stoch/sma init done" << std::endl;
    for (int i = dead_time; i < db[0].size(); i++)
    {
        stoch_k2 = stoch_k1;
        stoch_d2 = stoch_d1;
        stoch_k1 = stoch_kcurr;
        stoch_d1 = stoch_dcurr;
        stoch_kcurr = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size()), k);
        stoch_dcurr = sym.sma(std::span(stoch_arr).subspan(0, stoch_arr.size()), d);
        if (pos != 0)
        {
            if ((pos < 0 && -pos < stop && stop < db[0][i]) || (pos > 0 && pos > stop && stop > db[0][i]))
            {
                res -= 1.5 * stop;
                loss++;
            }
            if (pos > 0 && ((stoch_kcurr > stoch_fix && stoch_dcurr > stoch_fix) || (stoch_kcurr < stoch_dcurr)))
            {
                res += db[3][i] - pos;
                win++;
            }
            if (pos < 0 && ((stoch_kcurr < 1 - stoch_fix && stoch_dcurr < 1 - stoch_fix) || (stoch_kcurr > stoch_dcurr)))
            {
                res -= db[3][i] + pos;
                win++;
            }
            pos = 0;
            continue;
        }
        if (pos == 0)
        {
            if (0.8 < stoch_d2 && stoch_d2 < stoch_k2 && stoch_k1 < stoch_d1)
            {
                pos -= db[0][i];
            }
            if (0.2 > stoch_d2 && stoch_d2 > stoch_k2 && stoch_k1 > stoch_d1)
            {
                pos -= db[0][i];
            }
        }
    }
    if (pos < 0)
    {
        res -= db[2].back() + pos;
    }
    if (pos > 0)
    {
        res += db[2].back() - pos;
    }
    std::cout << "res: " << res << std::endl;
    std::cout << "wr: " << win  / (win + loss) << std::endl;
    std::cout << "deal count: " << win + loss << std::endl;
}
*/
int main()
{
    DB_manager db;
    std::vector<double> _kbase;
    std::vector<double> _dbase;
    _kbase.resize(1);
    _dbase.resize(2);
    //db.load_data(SPBFUT, "SVM6", FRAME_M1, "2026-03-01", "2026-06-12");
    //db.load_data(INDX, "IMOEX2", FRAME_D, "2025-11-01", "2026-06-12");
    std::chrono::year_month_day d = std::chrono::year_month_day(std::chrono::year(2026), std::chrono::month(3), std::chrono::day(1));
    std::chrono::year_month_day end = std::chrono::
            year_month_day(std::chrono::year(2026), std::chrono::month(6), std::chrono::day(12));
    sma s = sma(50);
    std::vector<candle> c = db.get_range(INDX, "IMOEX2", FRAME_D, "2026-03-01", "2026-06-11");
    for (auto i : c)
    {
        std::cout << i << std::endl;
    }
    std::cout << std::endl;
    auto sma_ = s.calc(c | std::views::transform(&candle::close));

    std::cout << "sma: ";
    for (auto i : sma_)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    stoch st = stoch(5);

    auto stoch_ = st.calc(c);
std::cout << "stoch: ";
    for (auto i : stoch_)
    {
        std::cout << i << " ";
    } 
std::cout << std::endl;
    auto cl = c | std::views::transform(&candle::close);
    auto hi = c | std::views::transform(&candle::high);
    auto lo = c | std::views::transform(&candle::low);
    auto stoch_1 = st.calc(cl, hi, lo);
    std::cout << "split-data stoch: ";
    for (auto i : stoch_1)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    /*
    while (d < end)
    {
        std::vector<candle> c = db.get_day(SPBFUT, "SVM6", FRAME_M1, "2026-06-12");
        for (int s_length = 4; s_length < 70; s_length++)
        {
            for (int k_length = 1; k_length < 50; k_length++)
            {
                for (int d_length = 1; d_length < 50; d_length++)
                {

                }
            }
        }
        d = shift(d, 1);
    }
    */
    /*
    for (auto i : c)
    {
        std::cout << get_classcode(i.class_code) << " " << i.ticker << " " << get_frame(i.frame)
                  << " " << i.get_date_str() << " " << i.get_time_str() << " open: " << i.open
                  << " high: " << i.high << " low: " << i.low << " close: " << i.close
                  << " volume: " << i.volume << std::endl;
    }
    */
    return 0;
}
