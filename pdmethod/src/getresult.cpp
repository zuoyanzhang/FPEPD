#include "common.h"
double getULP2(double x1, double x2, double origin) {
	return 1.0;
}
double getULP3(double x1, double x2, double x3, double origin) {
	return 1.0;
}
double getRelativeError2(double x1, double x2, double origin) {
	return 1.0;
}
double getRelativeError3(double x1, double x2, double x3, double origin) {
	return 1.0;
}
double getDoubleOfOrigin2(double inputx1, double inputx2) {
	return 1.0;
}
double getDoubleOfOrigin3(double inputx1, double inputx2, double inputx3) {
	return 1.0;
}
double getULP(double x, double origin) {
	mpfr_t mpfr_origin, mpfr_oracle, mp1, mp2, mp3;
	mpfr_inits2(128, mpfr_origin, mpfr_oracle, mp1, mp2, mp3, (mpfr_ptr) 0);
	mpfr_set_d(mp1, x, MPFR_RNDN);
	mpfr_set_d(mp2, x, MPFR_RNDN);
	mpfr_mul(mp3, mp1, mp2, MPFR_RNDN);

	mpfr_set(mpfr_oracle, mp3, MPFR_RNDN);
	mpfr_set_d(mpfr_origin, origin, MPFR_RNDN);
	double ulp = computeULPDiff(mpfr_origin, mpfr_oracle);
	mpfr_clears(mpfr_origin, mpfr_oracle, mp1, mp2, mp3, (mpfr_ptr) 0);
	mpfr_free_cache();
	return ulp;
}
double getRelativeError(double x, double origin) {
	mpfr_t mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3;
	mpfr_inits2(128, mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3, (mpfr_ptr) 0);
	mpfr_set_d(mp1, x, MPFR_RNDN);
	mpfr_set_d(mp2, x, MPFR_RNDN);
	mpfr_mul(mp3, mp1, mp2, MPFR_RNDN);

	mpfr_set(mpfr_oracle, mp3, MPFR_RNDN);
	mpfr_set_d(mpfr_origin, origin, MPFR_RNDN);
	mpfr_sub(mpfr_absolute, mpfr_oracle, mpfr_origin, MPFR_RNDN);
	mpfr_div(mpfr_relative, mpfr_absolute, mpfr_oracle, MPFR_RNDN);
	double relative = mpfr_get_d(mpfr_relative, MPFR_RNDN);
	relative = fabs(relative);
	mpfr_clears(mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3, (mpfr_ptr) 0);
	mpfr_free_cache();
	return relative;
}
double getDoubleOfOrigin(double inputx) {
	double x = inputx;
	return x * x;
}
