/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 Jaakko Julin

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

double stop_sample(sim_workspace *ws, const ion *incident, const sample *sample, gsto_stopping_type type, double x, double E);
double stop_step(sim_workspace *ws, ion *incident, const sample *sample, double x, double h_max, double step);
void simulate(const ion *incident, double x_0, sim_workspace *ws, const sample *sample);
reaction *make_rbs_reactions(const sample *sample, const simulation *sim);/* Note that sim->ion needs to be set! */
int assign_stopping(jibal_gsto *gsto, simulation *sim, sample *sample);
void print_spectra(FILE *f, const global_options *global, const sim_workspace *ws, const sample *sample, const gsl_histogram *exp);
void add_fit_params(global_options *global, simulation *sim, jibal_layer **layers, int n_layers, fit_params *params);
void output_bricks(const char *filename, const sim_workspace *ws);
void no_ds(sim_workspace *ws, const sample *sample);
void ds(sim_workspace *ws, const sample *sample); /* TODO: the DS routine is more pseudocode at this stage... */

#endif // JABS_JABS_H
