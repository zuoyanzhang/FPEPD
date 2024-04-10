//
// Created by 99065 on 2024/3/25.
//

#include "common.h"
int jiou(int x)
{
    if (x % 2 == 0)
        return 1;
    else
        return 0;
}

double computeULP(double y)
{
    double x = 0, res = 0, powermin = 0, powermax = 0, powermiddle = 0;
    int expmin = 0, expmax = 0, expmiddle = 0, jioupanduan = 0;
    x = fabs(y);

    //  printf("res=%.8f\n",pow(2,logb(x)+1-53));

    if (x < pow(2, -1021))
        res = pow(2, -1074);
    else if (x > (1 - pow(2, -53)) * pow(2, 1024))
        res = pow(2, 971);
    else
    {
        powermin = pow(2, -1021);
        expmin = -1021;
        powermax = pow(2, 1024);
        expmax = 1024;
    }

    while (expmax - expmin > 1)
    {
        jioupanduan = jiou(expmin + expmax);
        if (jioupanduan == 1)
            expmiddle = (expmax + expmin) / 2;
        else
            expmiddle = (expmax + expmin + 1) / 2;

        powermiddle = pow(2, expmiddle);

        if (x >= powermiddle)
        {
            powermin = powermiddle;
            expmin = expmiddle;
        }
        else
        {
            powermax = powermiddle;
            expmax = expmiddle;
        }

        if (x == powermin)
            res = pow(2, expmin - 53);
        else
            res = pow(2, expmin - 52);
    }

    return res;
}

float computeULPf(float y)
{
    float x = 0, res = 0, powermin = 0, powermax = 0, powermiddle = 0;
    int expmin = 0, expmax = 0, expmiddle = 0, jioupanduan = 0;
    x = fabs(y);

    //  printf("res=%.8f\n",pow(2,logb(x)+1-53));

    if (x < pow(2, -125))
        res = pow(2, -149);
    else if (x > (1 - pow(2, -24)) * pow(2, 128))
        res = pow(2, 104);
    else
    {
        powermin = pow(2, -125);
        expmin = -125;
        powermax = pow(2, 128);
        expmax = 128;
    }

    while (expmax - expmin > 1)
    {
        jioupanduan = jiou(expmin + expmax);
        if (jioupanduan == 1)
            expmiddle = (expmax + expmin) / 2;
        else
            expmiddle = (expmax + expmin + 1) / 2;

        powermiddle = pow(2, expmiddle);

        if (x >= powermiddle)
        {
            powermin = powermiddle;
            expmin = expmiddle;
        }
        else
        {
            powermax = powermiddle;
            expmax = expmiddle;
        }

        if (x == powermin)
            res = pow(2, expmin - 24);
        else
            res = pow(2, expmin - 23);
    }
    return res;
}

double computeULPDiff(mpfr_t origin, mpfr_t oracle) {
    mpfr_t mp1, mp2, mp3, mp4;
    mpfr_inits2(128, mp1, mp2, mp3, mp4, (mpfr_ptr) 0);
    mpfr_sub(mp1, origin, oracle, MPFR_RNDN);
    double ulp_oracle = mpfr_get_d(oracle, MPFR_RNDN);
    double c = computeULP(ulp_oracle);
    mpfr_set_d(mp2, c, MPFR_RNDN);
    mpfr_div(mp3, mp1, mp2, MPFR_RNDN);
    double result = fabs(mpfr_get_d(mp3, MPFR_RNDN));
    mpfr_clears(mp1, mp2, mp3, mp4, (mpfr_ptr) 0);
    mpfr_free_cache();
    return result;
}

float computeULPFDiff(mpfr_t origin, mpfr_t oracle) {
    mpfr_t mp1, mp2, mp3, mp4;
    mpfr_inits2(128, mp1, mp2, mp3, mp4, (mpfr_ptr) 0);
    mpfr_sub(mp1, origin, oracle, MPFR_RNDN);
    float ulp_oracle = mpfr_get_flt(oracle, MPFR_RNDN);
    float c = computeULPf(ulp_oracle);
    mpfr_set_flt(mp2, c, MPFR_RNDN);
    mpfr_div(mp3, mp1, mp2, MPFR_RNDN);
    double result = abs(mpfr_get_d(mp3, MPFR_RNDN));
    mpfr_clears(mp1, mp2, mp3, mp4, (mpfr_ptr) 0);
    mpfr_free_cache();
    return result;
}