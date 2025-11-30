/* Author: Peter Sovietov */
/* Modifications by Megus */

#ifndef AYUMI_FILTERS_H
#define AYUMI_FILTERS_H

typedef float (*ayumi_filter_func)(float* x);

// Filter functions
float ayumi_filter_low(float* x);
float ayumi_filter_medium(float* x);
float ayumi_filter_high(float* x);
float ayumi_filter_best(float* x);



#endif