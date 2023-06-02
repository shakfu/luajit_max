
#include <math.h>

// from: https://www.desmos.com/calculator/ewnq4hyrbz

double scale_linear(double x, double i_min, double i_max, double o_min, double o_max)
{
    double slope = 1.0 * (o_max - o_min) / (i_max - i_min);
    return o_min + round(slope * (x - i_min));
}


double scale_sine1(double x, double i_min, double i_max, double o_min, double o_max)
{
    return - (o_max - o_min) / 2.0 * cos(M_PI * (i_min - x) / (i_min - i_max)) + (o_max + o_min) / 2;
}

double scale_sine2(double x, double i_min, double i_max, double o_min, double o_max)
{
    return ((o_max - o_min) / M_PI) * asin((2 / (i_max - i_min)) * ( x - ((i_min + i_max) / 2) )) + ((o_max + o_min) / 2);
}


double scale_exp1(double x, double s, double i_min, double i_max, double o_min, double o_max)
{
    return -s * powf(fabs(o_min - o_max - s), (x - i_max) / (i_min  - i_max)) + o_max + s;
}

double scale_exp2(double x, double s, double i_min, double i_max, double o_min, double o_max)
{
    return s * powf(fabs(o_max - o_min + s), (x - i_min) / (i_max  - i_min)) + o_min - s;
}


double scale_log1(double x, double p, double i_min, double i_max, double o_min, double o_max)
{
    return ((o_max - o_min) * log(fabs(x - i_min + p))) / log(fabs(i_max - i_min + p)) + o_min;
}

double scale_log2(double x, double p, double i_min, double i_max, double o_min, double o_max)
{
    return ((o_min - o_max) * log(fabs(x - i_max - p))) / log(fabs(i_min - i_max - p)) + o_max;
}
