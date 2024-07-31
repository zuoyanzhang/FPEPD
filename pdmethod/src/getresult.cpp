#include "common.h"
double getULP(double x, double origin) {
	return 1.0;
}
double getULP3(double x1, double x2, double x3, double origin) {
	return 1.0;
}
double getRelativeError(double x, double origin) {
	return 1.0;
}
double getRelativeError3(double x1, double x2, double x3, double origin) {
	return 1.0;
}
double getDoubleOfOrigin(double inputx) {
	return 1.0;
}
double getDoubleOfOrigin3(double inputx1, double inputx2, double inputx3) {
	return 1.0;
}
double getULP2(double x1, double x2, double origin) {
	mpfr_t mpfr_origin, mpfr_oracle, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11;
	mpfr_inits2(128, mpfr_origin, mpfr_oracle, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11, (mpfr_ptr) 0);
	mpfr_set_d(mp1, x1, MPFR_RNDN);
	mpfr_set_d(mp2, x2, MPFR_RNDN);
	mpfr_mul(mp3, mp1, mp2, MPFR_RNDN);
	mpfr_set_d(mp4, x2, MPFR_RNDN);
	mpfr_mul(mp5, mp3, mp4, MPFR_RNDN);
	mpfr_set_d(mp6, 3.1299999999999999, MPFR_RNDN);
	mpfr_sub(mp7, mp5, mp6, MPFR_RNDN);
	mpfr_set_d(mp8, x1, MPFR_RNDN);
	mpfr_set_d(mp9, 1, MPFR_RNDN);
	mpfr_sub(mp10, mp8, mp9, MPFR_RNDN);
	mpfr_div(mp11, mp7, mp10, MPFR_RNDN);

	mpfr_set(mpfr_oracle, mp11, MPFR_RNDN);
	mpfr_set_d(mpfr_origin, origin, MPFR_RNDN);
	double ulp = computeULPDiff(mpfr_origin, mpfr_oracle);
	mpfr_clears(mpfr_origin, mpfr_oracle, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11, (mpfr_ptr) 0);
	mpfr_free_cache();
	return ulp;
}
double getRelativeError2(double x1, double x2, double origin) {
	mpfr_t mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11;
	mpfr_inits2(128, mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11, (mpfr_ptr) 0);
	mpfr_set_d(mp1, x1, MPFR_RNDN);
	mpfr_set_d(mp2, x2, MPFR_RNDN);
	mpfr_mul(mp3, mp1, mp2, MPFR_RNDN);
	mpfr_set_d(mp4, x2, MPFR_RNDN);
	mpfr_mul(mp5, mp3, mp4, MPFR_RNDN);
	mpfr_set_d(mp6, 3.1299999999999999, MPFR_RNDN);
	mpfr_sub(mp7, mp5, mp6, MPFR_RNDN);
	mpfr_set_d(mp8, x1, MPFR_RNDN);
	mpfr_set_d(mp9, 1, MPFR_RNDN);
	mpfr_sub(mp10, mp8, mp9, MPFR_RNDN);
	mpfr_div(mp11, mp7, mp10, MPFR_RNDN);

	mpfr_set(mpfr_oracle, mp11, MPFR_RNDN);
	mpfr_set_d(mpfr_origin, origin, MPFR_RNDN);
	mpfr_sub(mpfr_absolute, mpfr_oracle, mpfr_origin, MPFR_RNDN);
	mpfr_div(mpfr_relative, mpfr_absolute, mpfr_oracle, MPFR_RNDN);
	double relative = mpfr_get_d(mpfr_relative, MPFR_RNDN);
	relative = fabs(relative);
	mpfr_clears(mpfr_origin, mpfr_oracle, mpfr_relative, mpfr_absolute, mp1, mp2, mp3, mp4, mp5, mp6, mp7, mp8, mp9, mp10, mp11, (mpfr_ptr) 0);
	mpfr_free_cache();
	return relative;
}
double getDoubleOfOrigin2(double inputx1, double inputx2) {
	double x1 = inputx1;
	double x2 = inputx2;
	return (x1 * x2 * x2 - 3.13) / (x1 - 1);
}
