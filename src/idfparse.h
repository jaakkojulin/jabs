/*

    Jaakko's Backscattering Simulator (JaBS)
    Copyright (C) 2021 - 2024 Jaakko Julin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    See LICENSE.txt for the full license.

    Some parts of this source file under different license, see below!

 */
#ifndef IDFPARSER_H
#define IDFPARSER_H
#include <libxml/tree.h>
#include <jibal_units.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IDF_BUF_MSG_MAX (1024) /* Maximum length of input string to idf_output_printf. Longer strings will be truncated. */
#define IDF_BUF_SIZE_INITIAL (8*IDF_BUF_MSG_MAX)
#define JABS_FILE_SUFFIX ".jbs"
#define EXP_SPECTRUM_FILE_SUFFIX ".dat"
#define SAVE_SPECTRUM_FILE_SUFFIX ".csv"

typedef enum idf_error {
    IDF2JBS_SUCCESS = 0,
    IDF2JBS_FAILURE = -1,
    IDF2JBS_FAILURE_COULD_NOT_READ = -2,
    IDF2JBS_FAILURE_NOT_IDF_FILE = -3,
    IDF2JBS_FAILURE_WRONG_EXTENSION = -4,
    IDF2JBS_FAILURE_COULD_NOT_WRITE_OUTPUT = -5,
    IDF2JBS_FAILURE_NO_SAMPLES_DEFINED = -6
} idf_error;

typedef struct idf_unit {
    const char *unit;
    double factor;
} idf_unit;

#define IDF_MODE_FWHM "FWHM"

#define IDF_UNIT_FRACTION "fraction"
#define IDF_UNIT_DEGREE "degree"
#define IDF_UNIT_RADIAN "radian"
#define IDF_UNIT_TFU "1e15at/cm2"
#define IDF_UNIT_PARTICLES "#particles"
#define IDF_UNIT_AMU "amu"
#define IDF_UNIT_KEV "keV"
#define IDF_UNIT_KEVCH "keV/channel"
#define IDF_UNIT_KEVCH2 "keV/channel^2"
#define IDF_UNIT_MEV "MeV"
#define IDF_UNIT_M "m"
#define IDF_UNIT_CM "cm"
#define IDF_UNIT_MM "mm"
#define IDF_UNIT_US "us"
#define IDF_UNIT_NS "ns"
#define IDF_UNIT_PS "ps"
#define IDF_UNIT_MSR "msr"
#define IDF_UNIT_SR "sr"


static const idf_unit idf_units[] = {
        {IDF_UNIT_FRACTION, 1.0},
        {IDF_UNIT_DEGREE, C_DEG},
        {IDF_UNIT_RADIAN, 1.0},
        {IDF_UNIT_TFU, C_TFU},
        {IDF_UNIT_PARTICLES, 1.0},
        {IDF_UNIT_AMU, C_U},
        {IDF_UNIT_KEV, C_KEV},
        {IDF_UNIT_KEVCH, C_KEV},
        {IDF_UNIT_KEVCH2, C_KEV},
        {IDF_UNIT_MEV, C_MEV},
        {IDF_UNIT_M,  1.0},
        {IDF_UNIT_CM, C_CM},
        {IDF_UNIT_MM, C_MM},
        {IDF_UNIT_US, C_US},
        {IDF_UNIT_NS, C_NS},
        {IDF_UNIT_PS, C_PS},
        {IDF_UNIT_MSR, C_MSR},
        {IDF_UNIT_SR, 1.0},
        {0, 0}};

typedef struct idf_parser {
    char *filename;
    char *basename; /* Same as filename, but without the extension */
    xmlDoc *doc;
    xmlNode *root_element;
    char *buf;
    size_t buf_size;
    size_t pos_write;
    idf_error error;
    size_t n_samples;
    size_t i_sample; /* Keep track on how many samples are defined in the file */
    char *sample_basename;
    size_t n_spectra; /* Number of spectra in a sample (the one that is currently being parsed) */
    size_t i_spectrum;
} idf_parser;

xmlNode *idf_findnode_deeper(xmlNode *root, const char *path, const char **path_next); /* Called by idf_findnode(), goes one step deeper in path */
xmlNode *idf_findnode(xmlNode *root, const char *path); /* Finds the first node with given path. Path should look "like/this/thing" */
size_t idf_foreach(idf_parser *idf, xmlNode *node, const char *name, idf_error (*f)(idf_parser *idf, xmlNode *node)); /* Runs f(parser,child_node) for each child element of node element name "name" */
char *idf_node_content_to_str(const xmlNode *node); /* will always return a char * which should be freed by free() */
const xmlChar *idf_xmlstr(const char *s);
double idf_node_content_to_double(const xmlNode *node); /* performs unit conversion */
int idf_node_content_to_boolean(const xmlNode *node);
double idf_unit_string_to_SI(const xmlChar *unit);
double idf_unit_mode(const xmlChar *mode); /* FWHM etc */
int idf_stringeq(const void *a, const void *b);
int idf_stringneq(const void *a, const void *b, size_t n);
idf_error idf_write_simple_data_to_file(const char *filename, const char *x, const char *y);
idf_error idf_output_printf(idf_parser *idf, const char *format, ...);
idf_error idf_buffer_realloc(idf_parser *idf);
idf_parser *idf_file_read(const char *filename);
void idf_file_free(idf_parser *idf);
idf_error idf_write_buf_to_file(const idf_parser *idf, char **filename_out); /* Name is generated automatically and set to filename_out (if it is not NULL). */
idf_error idf_write_buf(const idf_parser *idf, FILE *f);
char *idf_jbs_name(const idf_parser *idf);
char *idf_exp_name(const idf_parser *idf, size_t i_spectrum);
char *idf_sim_name(const idf_parser *idf, size_t i_spectrum);
char *idf_spectrum_out_name(const idf_parser *idf, size_t i_spectrum);
const char *idf_boolean_to_str(int boolean); /* "true", "false", "unset" trinary */
const char *idf_error_code_to_str(idf_error idferr);
xmlNodePtr idf_new_node_printf(const xmlChar *name, const char *format, ...);
xmlNodePtr idf_new_node_units(const xmlChar *name, const xmlChar *unit, const xmlChar *mode, double value);
#ifdef __cplusplus
}
#endif
#endif // IDFPARSER_H
