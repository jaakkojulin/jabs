/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021-2022 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */

#include <assert.h>
#include "geostragg.h"
#include "jabs.h"

double scattering_angle_exit_deriv(const ion *incident, const sim_workspace *ws) { /* Calculates the dtheta/dbeta derivative for geometrical straggling */
    double theta, phi;
    double scatter_theta, scatter_phi;
    double scatter_theta2, scatter_phi2;
    double theta_product, phi_product;
    double theta_product2, phi_product2;
    double epsilon = 0.001*C_DEG;
    rotate( -1.0*ws->sim->sample_theta, ws->sim->sample_phi, incident->theta, incident->phi, &theta, &phi);
    rotate( ws->det->theta, ws->det->phi, theta, phi, &scatter_theta, &scatter_phi);
    rotate(ws->det->theta+epsilon, ws->det->phi, theta, phi, &scatter_theta2, &scatter_phi2);
    rotate(ws->det->theta, ws->det->phi, ws->sim->sample_theta, ws->sim->sample_phi,  &theta_product, &phi_product);
    rotate(ws->det->theta+epsilon, ws->det->phi, ws->sim->sample_theta, ws->sim->sample_phi,  &theta_product2, &phi_product2);
    double deriv = (scatter_theta2-scatter_theta)/(theta_product2-theta_product) * -1.0; /* Since exit angle is pi - theta_product, the derivative dtheta/dbeta is -1.0 times this derivative calculated here */
#ifdef DEBUG
fprintf(stderr, "Scat: %g deg, eps: %g deg, product %g deg, eps: %g deg. Deriv %.8lf (-1.0 for IBM)\n", scatter_theta/C_DEG,scatter_theta2/C_DEG, theta_product/C_DEG, theta_product2/C_DEG, deriv);
#endif
return deriv;
}

double exit_angle_delta(const sim_workspace *ws, const char direction) {
    if(!ws->params.geostragg)
        return 0.0;
    if(ws->det->distance < 0.001 * C_MM)
        return 0.0;
    double sample_tilt = angle_tilt(ws->sim->sample_theta, ws->sim->sample_phi, direction);
    double det_tilt = C_PI - detector_angle(ws->det, direction);
    double exit = C_PI - det_tilt - sample_tilt;
    double geo = cos(exit)/cos(sample_tilt);
    double w = aperture_width_shape_product(&ws->sim->beam_aperture, direction);
    double delta_beam = w *  geo / ws->det->distance;
    double delta_detector = aperture_width_shape_product(&ws->det->aperture, direction) / ws->det->distance;
    double result = sqrt(pow2(delta_beam) + pow2(delta_detector));
#ifdef DEBUG
    fprintf(stderr, "Direction %c: detector angles %g deg, sample angle %g deg, exit angle %g deg\n", direction, det_tilt/C_DEG, sample_tilt/C_DEG, exit/C_DEG);
    fprintf(stderr, "cos(alpha) = %g, cos(beta) = %g, cos(beta)/cos(alpha) = %g\n", cos(sample_tilt), cos(exit), geo);
    fprintf(stderr, "Spread of exit angle in direction '%c' due to beam %g deg, due to detector %g deg. Combined %g deg FWHM.\n", direction, delta_beam/C_DEG, delta_detector/C_DEG, result/C_DEG);
#endif
    return result / C_FWHM;
}

geostragg_vars geostragg_vars_calculate(const sim_workspace *ws) {
    geostragg_vars g;
    g.x.direction = 'x';
    g.y.direction = 'y';
    geostragg_vars_dir *gds[] = {&g.x, &g.y, NULL};
    for(geostragg_vars_dir **gdp = gds; *gdp; gdp++) {
        geostragg_vars_dir *gd = *gdp;
        gd->delta_beta = exit_angle_delta(ws, gd->direction);
#ifdef UNNECESSARY_NUMERICAL_THINGS
        double theta_deriv_y = theta_deriv_beta(ws->det, gd->direction);
#else
        if(gd->direction == 'x') {
            gd->theta_deriv = -cos(ws->det->phi); /* TODO: check IBM with phi 180 deg and */
        } else if (gd->direction == 'y') {
            gd->theta_deriv = -sin(ws->det->phi); /* TODO: check Cornell with phi 270 deg */
        }
#endif
        gd->delta_beta = exit_angle_delta(ws, gd->direction);
        gd->beta_deriv = fabs(beta_deriv(ws->det, ws->sim, 'x'));
#ifdef DEBUG
        fprintf(stderr, "Spread in exit angle ('%c') %g deg FWHM\n", gd->direction, gd->delta_beta / C_DEG);
        fprintf(stderr, "dBeta/dBeta_%c: %g\n", gd->direction, gd->beta_deriv); /* TODO: verify, check sign */
        fprintf(stderr, "dTheta/dBeta_%c = %g\n", gd->direction, gd->theta_deriv); /* TODO: this should also be valid when sample_phi is not zero? */
#endif
    }
    return g;
}

double geostragg(const sim_workspace *ws, const sample *sample, const sim_reaction *r, const geostragg_vars_dir *gd, const depth d, const double E_0) {
    if(!ws->params.geostragg) {
        return 0.0;
    }
    ion ion;
    ion = r->p;
    //ion_set_angle(&ion, r->p.theta - delta_beta, ws->det->phi);
    ion_rotate(&ion, -1.0 * gd->delta_beta * gd->beta_deriv, gd->phi); /* -1.0 again because of difference between theta and pi - theta */
    ion.E = reaction_product_energy(r->r, r->theta + gd->delta_beta * gd->theta_deriv, E_0);
    /* TODO: make (debug) code that checks the sanity of the delta beta and delta theta angles.  */
#ifdef DEBUG
    fprintf(stderr, "Reaction product (+, direction '%c') angles %g deg, %g deg (delta beta %g deg, delta theta %g deg), E = %g keV, original %g keV\n",
            gd->direction, ion.theta/C_DEG, ion.phi/C_DEG, -1.0 * gd->delta_beta * gd->beta_deriv / C_DEG, gd->delta_beta * gd->theta_deriv / C_DEG, ion.E/C_KEV,
            reaction_product_energy(r->r, r->theta, E_0)/C_KEV);
#endif
    ion.S = 0.0; /* We don't need straggling for anything, might as well reset it */
    post_scatter_exit(&ion, d, ws, sample);
    double Eplus = ion.E;
    ion = r->p;
    //ion_set_angle(&ion, r->p.theta + delta_beta, ws->det->phi);
    ion_rotate(&ion, 1.0 * gd->delta_beta * gd->beta_deriv, gd->phi);
    ion.E = reaction_product_energy(r->r, r->theta - gd->delta_beta * gd->theta_deriv, E_0);
    ion.S = 0.0; /* We don't need straggling for anything, might as well reset it */
    post_scatter_exit(&ion, d, ws, sample);
    double Eminus = ion.E;
#ifdef DEBUG
    fprintf(stderr, "Direction (%c), Eplus %g keV, Eminus %g keV\n", gd->direction, Eplus/C_KEV, Eminus/C_KEV);
#endif
    return pow2((Eplus - Eminus)/2.0);
}

double theta_deriv_beta(const detector *det, const char direction) { /* TODO: possibly wrong sign */
    double x = detector_angle(det, 'x');
    double y = detector_angle(det, 'y');
    static const double delta = 0.001*C_DEG;
    double theta_from_rot = C_PI - theta_tilt(x, y); /* We shouldn't need to calculate this, but we do. Numerical issues? */
    if(direction == 'x') {
        x += delta;
    } else if(direction == 'y') {
        y += delta;
    } else {
        return 0.0;
    }
    double theta_eps = C_PI - theta_tilt(x, y);
    double deriv = (theta_eps - theta_from_rot) / delta;
#ifdef DEBUG
    fprintf(stderr, "Theta %.5lf deg, from rotations %.5lf deg. Delta = %g deg, Theta_eps = %.5lf deg\n", det->theta/C_DEG, theta_from_rot/C_DEG, delta/C_DEG, theta_eps/C_DEG);
    //assert(fabs(theta_from_rot - det->theta) < 1e-3); /* TODO: there is some numerical issue here */ */
    fprintf(stderr, "Derivative %.5lf (dir '%c').\n", deriv, direction);
#endif
    return deriv;
}

double beta_deriv(const detector *det, const simulation *sim, const char direction) { /* TODO: replace by analytical formula */
    double theta, phi;
    double theta_product, phi_product;
    static const double delta = 0.001*C_DEG;
    rotate(det->theta, det->phi, sim->sample_theta, sim->sample_phi, &theta_product, &phi_product); /* Detector in sample coordinate system */
    theta = delta;
    if(direction == 'x') {
        phi = 0.0;
    } else if(direction == 'y') {
        phi = C_PI_2;
    } else {
        return 0.0;
    }
    rotate(theta, phi, det->theta, det->phi, &theta, &phi);
    rotate(theta, phi, sim->sample_theta, sim->sample_phi, &theta, &phi); /* "new" Detector in sample coordinate system */
    double result = -1.0*(theta - theta_product)/delta; /* -1.0, since beta = pi - theta */
#ifdef DEBUG
    fprintf(stderr, "Beta deriv ('%c') got theta = %g deg (orig %g deg), diff in beta %g deg, result %g\n", direction, theta/C_DEG, theta_product/C_DEG,  (theta_product - theta)/C_DEG, result);
#endif
    return result;
}
