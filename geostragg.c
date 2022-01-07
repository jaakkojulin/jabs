/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021-2022 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */

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
    double exit = det_tilt - sample_tilt;
    double geo = fabs(cos(exit)/cos(sample_tilt));
    double w = aperture_width_shape_product(&ws->sim->beam_aperture, direction);
    double delta_beam = w *  geo / ws->det->distance;
    double delta_detector = aperture_width_shape_product(&ws->det->aperture, direction) / ws->det->distance;
    double result = sqrt(pow2(delta_beam) + pow2(delta_detector));
#ifdef DEBUG
    fprintf(stderr, "Direction %c: detector angles %g deg, sample angle %g deg, exit angle %g deg\n", direction, det_tilt/C_DEG, sample_tilt/C_DEG, exit/C_DEG);
    fprintf(stderr, "cos(alpha) = %g, cos(beta) = %g, cos(beta)/cos(alpha) = %g\n", cos(sample_tilt), cos(exit), geo);
    fprintf(stderr, "Spread of exit angle in direction '%c' due to beam %g deg, due to detector %g deg. Combined %g deg FWHM.\n\n", direction, delta_beam/C_DEG, delta_detector/C_DEG, result/C_DEG);
#endif
    return result;
}

double geostragg(const sim_workspace *ws, const sample *sample, const sim_reaction *r, const depth d, const double E_0, const double delta_beta, const double theta_deriv) {
    if(!ws->params.geostragg) {
        return 0.0;
    }
    ion ion;
    ion = r->p;
    //ion_set_angle(&ion, r->p.theta - delta_beta, ws->det->phi);
    ion_rotate(&ion, -1.0 * delta_beta, ws->det->phi); /* TODO: why ws->det->phi and not something else? */
    ion.E = reaction_product_energy(r->r, r->theta + delta_beta * theta_deriv, E_0);
    /* TODO: make (debug) code that checks the sanity of the delta beta and delta theta angles.  */
    double foo, bar;
    rotate(ion.theta, ion.phi, 0.0, 0.0, &foo, &bar);
#ifdef DEBUG
    fprintf(stderr, "Reaction product (+) (beta %g deg), direction %g deg, %g deg. Manual calculation of theta says %g deg. Other angles: %g deg, %g deg.\n", (C_PI - ion.theta)/C_DEG, ion.theta/C_DEG, ion.phi/C_DEG, (r->theta + delta_beta * theta_deriv)/C_DEG, foo/C_DEG, bar/C_DEG);
#endif
    ion.S = 0.0; /* We don't need straggling for anything, might as well reset it */
    post_scatter_exit(&ion, d, ws, sample);
    double Eplus = ion.E;
    ion = r->p;
    //ion_set_angle(&ion, r->p.theta + delta_beta, ws->det->phi);
    ion_rotate(&ion, 1.0 * delta_beta, ws->det->phi);
    ion.E = reaction_product_energy(r->r, r->theta - delta_beta * theta_deriv, E_0);
    ion.S = 0.0; /* We don't need straggling for anything, might as well reset it */
    post_scatter_exit(&ion, d, ws, sample);
    double Eminus = ion.E;
    return pow2((Eplus - Eminus)/2.0/C_FWHM);
}
