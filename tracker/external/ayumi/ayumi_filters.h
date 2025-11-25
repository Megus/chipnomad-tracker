/* Author: Peter Sovietov */
/* Modifications by Megus */

#ifndef AYUMI_FILTERS_H
#define AYUMI_FILTERS_H

typedef float (*ayumi_filter_func)(float* x);

// Fast filter for real-time playback (reduced quality)
float ayumi_filter_fast(float* x);

// Full quality filter for export (original quality)
float ayumi_filter_full(float* x);

#endif