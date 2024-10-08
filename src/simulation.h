/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2024 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */
#ifndef JABS_SIMULATION_H
#define JABS_SIMULATION_H

#include <jibal.h>
#include <time.h>

#include "ion.h"
#include "reaction.h"
#include "brick.h"
#include "detector.h"
#include "sample.h"
#include "prob_dist.h"
#include "stop.h"
#include "sim_calc_params.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct simulation {
    reaction **reactions;
    size_t n_reactions;
    detector **det; /* Array of n_det detector pointers */
    size_t n_det;
    sample *sample;
    double fluence; /* Number of incident particles */
    double sample_theta; /* Polar angle. Kind of. Zero is sample perpendicular to beam. */
    double sample_phi; /* Typically one uses a zero here, unless doing channeling stuff. Note that this is an azimuthal angle. */
    const jibal_isotope *beam_isotope;
    aperture *beam_aperture;
    double beam_E;
    double beam_E_broad; /* Variance */
    double emin;
    sim_calc_params *params;
    jabs_stop *stop;
    int erd; /* Add ERD reactions */
    int rbs; /* Add RBS reactions */
    jabs_reaction_cs cs_rbs;
    jabs_reaction_cs cs_erd;
    ion ion; /* This ion is not to be used in calculations, it is simply copied to ws->ion. We do store the nuclear stopping (ion->nucl_stop) here too. */
} simulation;

simulation *sim_init(jibal *jibal);
void sim_free(simulation *sim);
jabs_reaction_cs sim_cs(const simulation *sim, reaction_type type);
reaction *sim_reaction_make_from_argv(const jibal *jibal, const simulation *sim, int *argc, char * const **argv); /* Note confusing naming, this does not make a sim_reaction */
int sim_reactions_add_reaction(simulation *sim, reaction *r, int silent);
int sim_reactions_remove_reaction(simulation *sim, size_t i);
int sim_reactions_add_auto(simulation *sim, const sample_model *sm, reaction_type type, jabs_reaction_cs cs, int silent); /* Add RBS or ERD reactions automagically */
void sim_reactions_free(simulation *sim); /* Free reactions and reset the number of reactions to zero */
int sim_sanity_check(const simulation *sim);
detector *sim_det(const simulation *sim, size_t i_det);
detector *sim_det_from_string(const simulation *sim, const char *s);
int sim_det_add(simulation *sim, detector *det);
int sim_det_set(simulation *sim, detector *det, size_t i_det); /* Will free existing detector (can be NULL too) */
void sim_print(const simulation *sim, jabs_msg_level msg_level);
void sim_sort_reactions(const simulation *sim);
double sim_alpha_angle(const simulation *sim);
double sim_exit_angle(const simulation *sim, const detector *det);
int sim_do_we_need_erd(const simulation *sim);
int sim_prepare_ion(ion *ion, const simulation *sim, const jibal_isotope *isotopes, const jibal_gsto *gsto);
int sim_prepare_reactions(const simulation *sim, const jibal_isotope *isotopes, const jibal_gsto *gsto);
#ifdef __cplusplus
}
#endif
#endif // JABS_SIMULATION_H
