#ifndef JABS_BRICK_H
#define JABS_BRICK_H

#include <gsl/gsl_histogram.h>

typedef struct {
    double d; /* Depth */
    double E; /* Energy */
    double S; /* Straggling (variance) */
    double Q; /* Counts */
} brick;

void brick_int(double sigma_low, double  sigma_high, double E_low, double E_high, gsl_histogram *h, double Q);
void brick_int2(gsl_histogram *h, const brick *bricks, size_t n_bricks, const double S);

#endif // JABS_BRICK_H
