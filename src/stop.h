/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2023 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */
#ifndef JABS_STOP_H
#define JABS_STOP_H
#include "jibal_gsto.h"
#include "ion.h"
#include "sample.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct jabs_stop {
    jibal_gsto *gsto;
    gsto_stopping_type type;
    int nuclear_stopping_accurate;
    int rk4;
    double emin;
} jabs_stop;

typedef struct jabs_stop_step_params {
    double step; /* This is energy. Zero is automatic. */
    double min;
    double max;
    double sigmas;
} jabs_stop_step_params;

depth stop_next_crossing(const ion *incident, const sample *sample, const depth *d_from);
depth stop_step(const jabs_stop *stop, const jabs_stop *stragg, ion *incident, const sample *sample, depth depth_before, double step);
double stop_sample(const jabs_stop *stop, const ion *incident, const sample *sample, depth depth, double E);
double stop_step_calc(const jabs_stop_step_params *params, const ion *ion);
int stop_sample_exit(const jabs_stop *stop, const jabs_stop *stragg, const jabs_stop_step_params *params_exiting, ion *p, depth depth_start, const sample *sample);
#ifdef __cplusplus
}
#endif
#endif // JABS_STOP_H
