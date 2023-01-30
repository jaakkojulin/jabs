/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2023 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */
#include <string.h>
#include <assert.h>
#include "defaults.h"
#include "generic.h"
#include "message.h"
#include "simulation_workspace.h"
#include "spectrum.h"


void sim_workspace_init_reactions(sim_workspace *ws) {
    const simulation *sim = ws->sim;
    ws->reactions = calloc(sim->n_reactions, sizeof (sim_reaction *));
    for(size_t i_reaction = 0; i_reaction < sim->n_reactions; i_reaction++) {
        ws->reactions[i_reaction] = sim_reaction_init(&ws->ion, ws->isotopes, sim->sample, ws->det, sim->reactions[i_reaction], ws->n_channels, ws->n_bricks);
        ws->n_reactions++;
    }
}

void sim_workspace_calculate_number_of_bricks(sim_workspace *ws) {
    size_t n_bricks = 0;
    const detector *det = ws->det;
    const simulation *sim = ws->sim;
    if(ws->params->n_bricks_max) {
        n_bricks = ws->params->n_bricks_max;
    } else {
        if(det->type == DETECTOR_ENERGY) {
            if(ws->params->incident_stop_params.step == 0.0) { /* Automatic incident step size */
                n_bricks = (int) ceil(sim->beam_E / (ws->params->brick_width_sigmas * sqrt(detector_resolution(ws->det, sim->beam_isotope, sim->beam_E))) + ws->sample->n_ranges);
            } else {
                n_bricks = (int) ceil(sim->beam_E / ws->params->incident_stop_params.step + ws->sample->n_ranges); /* This is conservative */
            }
        } else { /* TODO: maybe something more clever is needed here */
            n_bricks = BRICKS_DEFAULT;
        }
        if(n_bricks > BRICKS_MAX) {
            n_bricks = BRICKS_MAX;
            jabs_message(MSG_WARNING, stderr,  "Number of bricks limited to %zu.\n", n_bricks);
        }
    }
    ws->n_bricks = n_bricks;
#ifdef DEBUG
    fprintf(stderr, "Number of bricks: %zu\n", n_bricks);
#endif
}

sim_workspace *sim_workspace_init(const jibal *jibal, const simulation *sim, const detector *det) {
    if(!jibal || !sim || !det) {
        jabs_message(MSG_ERROR, stderr,  "No JIBAL, sim or det. Guru thinks: %p, %p %p.\n", jibal, sim, det);
        return NULL;
    }
    if(!sim->sample) {
        jabs_message(MSG_ERROR, stderr,  "No sample has been set. Will not initialize workspace.\n");
        return NULL;
    }
    sim_workspace *ws = malloc(sizeof(sim_workspace));
    ws->sim = sim;
    ws->emin = sim->emin;
    ws->fluence = sim->fluence;
    ws->det = det;
    ws->sample = sim->sample;
    ws->params = sim_calc_params_defaults(NULL);
    sim_calc_params_copy(sim->params,  ws->params);
    sim_calc_params_update(ws->params);
    ws->gsto = jibal->gsto;
    ws->isotopes = jibal->isotopes;
    ws->n_reactions = 0; /* Will be incremented later */

#if 0
    if(sim->n_reactions == 0) {
        jabs_message(MSG_ERROR, stderr,  "No reactions! Will not initialize workspace if there is nothing to simulate.\n");
        free(ws);
        return NULL;
    }
#endif

    if(sim->params->beta_manual && sim->params->ds) {
        jabs_message(MSG_WARNING, stderr,  "Manual exit angle is enabled in addition to dual scattering. This is an unsupported combination. Manual exit angle calculation will be disabled.\n");
        ws->params->beta_manual  = FALSE;
    }

    if(sim->params->beta_manual && sim->params->geostragg) {
        jabs_message(MSG_WARNING, stderr,  "Manual exit angle is enabled in addition to geometric scattering. This is an unsupported combination. Geometric straggling calculation will be disabled.\n");
        ws->params->geostragg = FALSE;
    }
    ws->ion = sim->ion; /* Shallow copy, but that is ok */

    sim_workspace_recalculate_n_channels(ws, sim);

    if(ws->n_channels == 0) {
        free(ws);
        jabs_message(MSG_ERROR, stderr,  "Number of channels in workspace is zero. Aborting initialization.\n");
        return NULL;
    }
    ws->histo_sum = gsl_histogram_alloc(ws->n_channels);
    if(!ws->histo_sum) {
        return NULL;
    }
    spectrum_set_calibration(ws->histo_sum, ws->det, JIBAL_ANY_Z); /* Calibration (assuming default calibration) can be set now. */
    gsl_histogram_reset(ws->histo_sum); /* This is not necessary, since contents should be set after simulation is over (successfully). */

    sim_workspace_calculate_number_of_bricks(ws);
    sim_workspace_init_reactions(ws);

    if(ws->params->cs_adaptive) { /* Actually integrate, allocate workspace for this */
#ifdef DEBUG
        fprintf(stderr, "cs_n_steps = 0, allocating integration workspace w_int_cs with %zu max intervals.\n", ws->params->int_cs_max_intervals);
#endif
        ws->w_int_cs = gsl_integration_workspace_alloc(ws->params->int_cs_max_intervals);
    } else {
        ws->w_int_cs = NULL;
    }
    if(ws->params->cs_n_stragg_steps == 0) {
#ifdef DEBUG
        fprintf(stderr, "cs_n_stragg_steps = 0, allocating integration workspace w_int_cs_stragg with %zu max intervals.\n", ws->params->int_cs_stragg_max_intervals);
#endif
        ws->w_int_cs_stragg = gsl_integration_workspace_alloc(ws->params->int_cs_stragg_max_intervals);
    } else {
        ws->w_int_cs_stragg = NULL;
    }
    ws->stop.type = GSTO_STO_TOT;
    ws->stop.gsto = jibal->gsto;
    ws->stop.rk4 = ws->params->rk4;
    ws->stop.nuclear_stopping_accurate = ws->params->nuclear_stopping_accurate;
    ws->stop.emin = ws->emin;
    ws->stragg = ws->stop; /* Copy */
    ws->stragg.type = GSTO_STO_STRAGG;
    return ws;
}

void sim_workspace_free(sim_workspace *ws) {
    if(!ws)
        return;
    sim_calc_params_free(ws->params);
    for(size_t i = 0; i < ws->n_reactions; i++) {
        sim_reaction_free(ws->reactions[i]);
    }
    free(ws->reactions);
    gsl_integration_workspace_free(ws->w_int_cs);
    gsl_integration_workspace_free(ws->w_int_cs_stragg);
    gsl_histogram_free(ws->histo_sum);
    free(ws);
}

void sim_workspace_recalculate_n_channels(sim_workspace *ws, const simulation *sim) { /* TODO: assumes calibration function is increasing */
    size_t n_max = CHANNELS_ABSOLUTE_MIN; /* Always simulate at least CHANNELS_ABSOLUTE_MIN channels */
    for(size_t i_reaction = 0; i_reaction < sim->n_reactions; i_reaction++) {
        const reaction *r = sim->reactions[i_reaction];
        if(!reaction_is_possible(r, ws->params, ws->det->theta)) {
#ifdef DEBUG
            fprintf(stderr, "Reaction %zu (target %s, type %s) is not possible when theta = %g deg. Skipping. \n", i_reaction + 1, r->target->name,
                    reaction_type_to_string(r->type), ws->det->theta / C_DEG);
#endif
            continue;
        }
        double E = reaction_product_energy(r, ws->det->theta, sim->beam_E);
        double E_safer = E + 3.0*sqrt(detector_resolution(ws->det, r->product, E)); /* Add 3x resolution sigma to max energy */
        E_safer *= 1.1; /* and 10% for good measure! */
        while(detector_calibrated(ws->det, JIBAL_ANY_Z, n_max) < E_safer && n_max <= CHANNELS_ABSOLUTE_MAX) {n_max++;} /* Increase number of channels until we hit this energy. TODO: this requires changes for ToF spectra. */
#ifdef DEBUG
        fprintf(stderr, "Reaction %zu (target %s, type %s), max E = %g keV -> %g keV after resolution and safety factor have been added, current n_max %zu (channels)\n", i_reaction + 1, r->target->name,
                reaction_type_to_string(r->type), E/C_KEV, E_safer/C_KEV, n_max);
#endif
    }
    if(n_max == CHANNELS_ABSOLUTE_MAX)
        n_max=0;
    ws->n_channels = n_max;
}

void sim_workspace_calculate_sum_spectra(sim_workspace *ws) {
    double sum;
    for(size_t i = 0; i < ws->histo_sum->n; i++) {
        sum = 0.0;
        for(size_t j = 0; j < ws->n_reactions; j++) {
            if(ws->reactions[j]->histo && i < ws->reactions[j]->histo->n)
                sum += ws->reactions[j]->histo->bin[i];
        }
        ws->histo_sum->bin[i] = sum;
    }
}

void sim_workspace_histograms_reset(sim_workspace *ws) {
    for(size_t i = 0; i < ws->n_reactions; i++) {
        sim_reaction *r = ws->reactions[i];
        if(!r)
            continue;
#ifdef DEBUG_VERBOSE
        fprintf(stderr, "Reaction %i:\n", i);
#endif
        gsl_histogram_reset(r->histo);
    }
}

void sim_workspace_histograms_calculate(sim_workspace *ws) {
    for(size_t i = 0; i < ws->n_reactions; i++) {
        sim_reaction *r = ws->reactions[i];
        if(!r)
            continue;
#ifdef DEBUG_VERBOSE
        fprintf(stderr, "Reaction %i:\n", i);
#endif
        assert(r->last_brick < r->n_bricks);
        if(r->last_brick == 0) {
            continue;
        }
        bricks_calculate_sigma(ws->det, r->p.isotope, r->bricks, r->last_brick);
        bricks_convolute(r->histo, r->bricks, r->last_brick, ws->fluence * ws->det->solid, ws->params->sigmas_cutoff, ws->params->gaussian_accurate);
    }
}

void sim_workspace_histograms_scale(sim_workspace *ws, double scale) {
    for(size_t i = 0; i < ws->n_reactions; i++) {
        sim_reaction *r = ws->reactions[i];
        if(!r)
            continue;
#ifdef DEBUG_VERBOSE
        fprintf(stderr, "Reaction %i:\n", i);
#endif
        gsl_histogram_scale(r->histo, scale);
    }
}


int sim_workspace_print_spectra(const sim_workspace *ws, const char *filename, const gsl_histogram *histo_iter, const gsl_histogram *exp) {
    char sep = ' ';
    if(!ws) {
        return EXIT_FAILURE;
    }
    const gsl_histogram *h;
    if(histo_iter) {
        h = histo_iter;
    } else {
#ifdef DEBUG
        fprintf(stderr, "No stored sum spectrum in fit_data, falling back to sum spectrum in workspace.\n");
#endif
        h = ws->histo_sum;
    }
    if(!h) {
        return EXIT_FAILURE;
    }
    FILE *f = fopen_file_or_stream(filename, "w");
    if(!f) {
        return EXIT_FAILURE;
    }
    if(filename) {
        size_t l = strlen(filename);
        if(l > 4 && strncmp(filename + l - 4, ".csv", 4) == 0) { /* For CSV: print header line */
            sep = ','; /* and set the separator! */
            fprintf(f, "\"Channel\",\"Energy (keV)\",\"Simulated\"");
            if(exp) {
                fprintf(f, ",\"Experimental\"");
            }
            for(size_t j = 0; j < ws->n_reactions; j++) {
                const reaction *r = ws->reactions[j]->r;
                fprintf(f, ",\"%s (%s)\"", r->target->name, reaction_name(r));
            }
            fprintf(f, "\n");
        }
    }
    for(size_t i = 0; i < ws->n_channels; i++) {
        fprintf(f, "%zu%c%.3lf%c", i, sep, detector_calibrated(ws->det, JIBAL_ANY_Z, i) / C_KEV,
                sep); /* Channel, energy. TODO: Z-specific calibration can have different energy (e.g. for a particular reaction). */
        if(i >= h->n || h->bin[i] == 0.0) {
            fprintf(f, "0"); /* Tidier output with a clean zero sum */
        } else {
            fprintf(f, "%e", h->bin[i]);
        }
        if(exp) {
            if(i < exp->n) {
                fprintf(f, "%c%g", sep, exp->bin[i]);
            } else {
                fprintf(f, "%c0", sep);
            }
        }
        for(size_t j = 0; j < ws->n_reactions; j++) {
            gsl_histogram *rh = ws->reactions[j]->histo; /* TODO: these don't correspond to stored sum spectrum in fit (but they should be very close!). */
            if(i >= rh->n || rh->bin[i] == 0.0) {
                fprintf(f, "%c0", sep);
            } else {
                fprintf(f, "%c%e", sep, rh->bin[i]);
            }
        }
        fprintf(f, "\n");
    }
    fclose_file_or_stream(f);
    return EXIT_SUCCESS;
}

int sim_workspace_print_bricks(const sim_workspace *ws, const char *filename) {
    double psr = ws->fluence * ws->det->solid;
    FILE *f = fopen_file_or_stream(filename, "w");
    if(!f) {
        return EXIT_FAILURE;
    }
    for(size_t i = 0; i < ws->n_reactions; i++) {
        fprintf(f, "#Reaction %zu\n", i + 1);
        const sim_reaction *r = ws->reactions[i];
        if(r->last_brick == 0) {
            continue;
        }
        sim_reaction_print_bricks(f, r, psr);
        fprintf(f, "\n\n");
    }
    fclose_file_or_stream(f);
    return EXIT_SUCCESS;
}
