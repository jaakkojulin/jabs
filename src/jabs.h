/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2024 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */
#ifndef JABS_JABS_H
#define JABS_JABS_H

#include "simulation.h"
#include "ion.h"
#include "fit.h"
#include "options.h"
#include "sample.h"
#include "reaction.h"
#include "geostragg.h"
#include "stop.h"
#include "des.h"

#ifdef __cplusplus
extern "C" {
#endif

int simulate(const ion *incident, depth depth_start, sim_workspace *ws, const sample *sample);
int simulate_reaction(const ion *incident, depth depth_start, sim_workspace *ws, const sample *sample, const des_table *dt, const geostragg_vars *g, sim_reaction *sim_r);
int simulate_init_reaction(sim_reaction *sim_r, const sample *sample, const sim_calc_params *params, const geostragg_vars *g, double emin, double emin_incident, double emax_incident);
int assign_stopping(jibal_gsto *gsto, const simulation *sim);
int assign_stopping_Z2(jibal_gsto *gsto, const simulation *sim, int Z2); /* Assigns stopping and straggling (GSTO) for given Z2. Goes through all possible Z1s (beam and reaction products). */
int assign_stopping_Z1_Z2(jibal_gsto *gsto, int Z1, int Z2);
int simulate_with_ds(sim_workspace *ws);
double cross_section_concentration_product(const sim_workspace *ws, const sample *sample, const sim_reaction *sim_r, double E_front, double E_back, const depth *d_before, const depth *d_after, double S_front, double S_back);
double cross_section_concentration_product_adaptive(const sim_workspace *ws, const sample *sample, const sim_reaction *sim_r, double E_front, double E_back, const depth *d_before, const depth *d_after, double S_front, double S_back);
double cross_section_straggling(const sim_reaction *sim_r, gsl_integration_workspace *w, double accuracy, const prob_dist *pd, double E, double S);
double cross_section_straggling_fixed(const sim_reaction *sim_r, const prob_dist *pd, double E, double S);
double cross_section_straggling_adaptive(const sim_reaction *sim_r, gsl_integration_workspace *w, double accuracy, double E, double S);
#ifdef __cplusplus
}
#endif
#endif // JABS_JABS_H
