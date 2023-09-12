/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2023 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

 */

#ifndef JABS_SIMULATION2IDF_H
#define JABS_SIMULATION2IDF_H
#include "simulation.h"
#include "fit.h"
#include <libxml/parser.h>

int simulation2idf(struct fit_data *fit, const char *filename);
xmlNodePtr simulation2idf_notes();
xmlNodePtr simulation2idf_elementsandmolecules(const sample_model *sm);
#endif
