/*
 * Copyright (c) 2013-2014, The University of Oxford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <private_station.h>
#include <oskar_station_accessors.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Data common to all station types. */

int oskar_station_precision(const oskar_Station* model)
{
    return model->precision;
}

int oskar_station_mem_location(const oskar_Station* model)
{
    return model->mem_location;
}

int oskar_station_type(const oskar_Station* model)
{
    return model->station_type;
}

int oskar_station_normalise_final_beam(const oskar_Station* model)
{
    return model->normalise_final_beam;
}

double oskar_station_lon_rad(const oskar_Station* model)
{
    return model->lon_rad;
}

double oskar_station_lat_rad(const oskar_Station* model)
{
    return model->lat_rad;
}

double oskar_station_alt_metres(const oskar_Station* model)
{
    return model->alt_metres;
}

double oskar_station_beam_lon_rad(const oskar_Station* model)
{
    return model->beam_lon_rad;
}

double oskar_station_beam_lat_rad(const oskar_Station* model)
{
    return model->beam_lat_rad;
}

int oskar_station_beam_coord_type(const oskar_Station* model)
{
    return model->beam_coord_type;
}

oskar_Mem* oskar_station_noise_freq_hz(oskar_Station* model)
{
    return model->noise_freq_hz;
}

const oskar_Mem* oskar_station_noise_freq_hz_const(const oskar_Station* model)
{
    return model->noise_freq_hz;
}

oskar_Mem* oskar_station_noise_rms_jy(oskar_Station* model)
{
    return model->noise_rms_jy;
}

const oskar_Mem* oskar_station_noise_rms_jy_const(const oskar_Station* model)
{
    return model->noise_rms_jy;
}

/* Data used only for Gaussian beam stations. */

double oskar_station_gaussian_beam_fwhm_rad(const oskar_Station* model)
{
    return model->gaussian_beam_fwhm_rad;
}

double oskar_station_gaussian_beam_reference_freq_hz(const oskar_Station* model)
{
    return model->gaussian_beam_reference_freq_hz;
}

/* Data used only for aperture array stations. */

int oskar_station_identical_children(const oskar_Station* model)
{
    return model->identical_children;
}

int oskar_station_num_elements(const oskar_Station* model)
{
    return model->num_elements;
}

int oskar_station_num_element_types(const oskar_Station* model)
{
    return model->num_element_types;
}

int oskar_station_use_polarised_elements(const oskar_Station* model)
{
    return model->use_polarised_elements;
}

int oskar_station_normalise_array_pattern(const oskar_Station* model)
{
    return model->normalise_array_pattern;
}

int oskar_station_enable_array_pattern(const oskar_Station* model)
{
    return model->enable_array_pattern;
}

int oskar_station_common_element_orientation(const oskar_Station* model)
{
    return model->common_element_orientation;
}

int oskar_station_array_is_3d(const oskar_Station* model)
{
    return model->array_is_3d;
}

int oskar_station_apply_element_errors(const oskar_Station* model)
{
    return model->apply_element_errors;
}

int oskar_station_apply_element_weight(const oskar_Station* model)
{
    return model->apply_element_weight;
}

double oskar_station_nominal_element_orientation_x_rad(
        const oskar_Station* model)
{
    return model->nominal_orientation_x_rad;
}

double oskar_station_nominal_element_orientation_y_rad(
        const oskar_Station* model)
{
    return model->nominal_orientation_y_rad;
}

double oskar_station_element_orientation_x_rad(const oskar_Station* model,
        int index)
{
    const double *p;
    p = (const double*)
            oskar_mem_void_const(model->element_orientation_x_rad_cpu);
    return p[index];
}

double oskar_station_element_orientation_y_rad(const oskar_Station* model,
        int index)
{
    const double *p;
    p = (const double*)
            oskar_mem_void_const(model->element_orientation_y_rad_cpu);
    return p[index];
}

oskar_Mem* oskar_station_element_true_x_enu_metres(oskar_Station* model)
{
    return model->element_true_x_enu_metres;
}

const oskar_Mem* oskar_station_element_true_x_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_true_x_enu_metres;
}

oskar_Mem* oskar_station_element_true_y_enu_metres(oskar_Station* model)
{
    return model->element_true_y_enu_metres;
}

const oskar_Mem* oskar_station_element_true_y_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_true_y_enu_metres;
}

oskar_Mem* oskar_station_element_true_z_enu_metres(oskar_Station* model)
{
    return model->element_true_z_enu_metres;
}

const oskar_Mem* oskar_station_element_true_z_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_true_z_enu_metres;
}

oskar_Mem* oskar_station_element_measured_x_enu_metres(oskar_Station* model)
{
    return model->element_measured_x_enu_metres;
}

const oskar_Mem* oskar_station_element_measured_x_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_measured_x_enu_metres;
}

oskar_Mem* oskar_station_element_measured_y_enu_metres(oskar_Station* model)
{
    return model->element_measured_y_enu_metres;
}

const oskar_Mem* oskar_station_element_measured_y_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_measured_y_enu_metres;
}

oskar_Mem* oskar_station_element_measured_z_enu_metres(oskar_Station* model)
{
    return model->element_measured_z_enu_metres;
}

const oskar_Mem* oskar_station_element_measured_z_enu_metres_const(
        const oskar_Station* model)
{
    return model->element_measured_z_enu_metres;
}

oskar_Mem* oskar_station_element_gain(oskar_Station* model)
{
    return model->element_gain;
}

const oskar_Mem* oskar_station_element_gain_const(const oskar_Station* model)
{
    return model->element_gain;
}

oskar_Mem* oskar_station_element_gain_error(oskar_Station* model)
{
    return model->element_gain_error;
}

const oskar_Mem* oskar_station_element_gain_error_const(
        const oskar_Station* model)
{
    return model->element_gain_error;
}

oskar_Mem* oskar_station_element_phase_offset_rad(oskar_Station* model)
{
    return model->element_phase_offset_rad;
}

const oskar_Mem* oskar_station_element_phase_offset_rad_const(
        const oskar_Station* model)
{
    return model->element_phase_offset_rad;
}

oskar_Mem* oskar_station_element_phase_error_rad(oskar_Station* model)
{
    return model->element_phase_error_rad;
}

const oskar_Mem* oskar_station_element_phase_error_rad_const(
        const oskar_Station* model)
{
    return model->element_phase_error_rad;
}

oskar_Mem* oskar_station_element_weight(oskar_Station* model)
{
    return model->element_weight;
}

const oskar_Mem* oskar_station_element_weight_const(const oskar_Station* model)
{
    return model->element_weight;
}

oskar_Mem* oskar_station_element_orientation_x_rad_cpu(oskar_Station* model)
{
    return model->element_orientation_x_rad_cpu;
}

const oskar_Mem* oskar_station_element_orientation_x_rad_cpu_const(
        const oskar_Station* model)
{
    return model->element_orientation_x_rad_cpu;
}

oskar_Mem* oskar_station_element_orientation_y_cpu(oskar_Station* model)
{
    return model->element_orientation_y_rad_cpu;
}

const oskar_Mem* oskar_station_element_orientation_y_rad_cpu_const(
        const oskar_Station* model)
{
    return model->element_orientation_y_rad_cpu;
}

oskar_Mem* oskar_station_element_types(oskar_Station* model)
{
    return model->element_types;
}

const oskar_Mem* oskar_station_element_types_const(const oskar_Station* model)
{
    return model->element_types;
}

const int* oskar_station_element_types_cpu_const(const oskar_Station* model)
{
    return (const int*) oskar_mem_void_const(model->element_types_cpu);
}

int oskar_station_has_child(const oskar_Station* model)
{
    return model->child ? 1 : 0;
}

oskar_Station* oskar_station_child(oskar_Station* model, int i)
{
    return model->child[i];
}

const oskar_Station* oskar_station_child_const(const oskar_Station* model,
        int i)
{
    return model->child[i];
}

int oskar_station_has_element(const oskar_Station* model)
{
    return model->element ? 1 : 0;
}

oskar_Element* oskar_station_element(oskar_Station* model,
        int element_type_index)
{
    return model->element[element_type_index];
}

const oskar_Element* oskar_station_element_const(const oskar_Station* model,
        int element_type_index)
{
    return model->element[element_type_index];
}

int oskar_station_num_permitted_beams(const oskar_Station* model)
{
    return model->num_permitted_beams;
}

const oskar_Mem* oskar_station_permitted_beam_az_rad_const(
        const oskar_Station* model)
{
    return model->permitted_beam_az_rad;
}

const oskar_Mem* oskar_station_permitted_beam_el_rad_const(
        const oskar_Station* model)
{
    return model->permitted_beam_el_rad;
}


/* Setters. */

void oskar_station_set_station_type(oskar_Station* model, int type)
{
    model->station_type = type;
}

void oskar_station_set_normalise_final_beam(oskar_Station* model, int value)
{
    model->normalise_final_beam = value;
}

void oskar_station_set_position(oskar_Station* model,
        double longitude_rad, double latitude_rad, double altitude_m)
{
    model->lon_rad = longitude_rad;
    model->lat_rad = latitude_rad;
    model->alt_metres = altitude_m;
}

void oskar_station_set_phase_centre(oskar_Station* model,
        int beam_coord_type, double beam_longitude_rad,
        double beam_latitude_rad)
{
    model->beam_coord_type = beam_coord_type;
    model->beam_lon_rad = beam_longitude_rad;
    model->beam_lat_rad = beam_latitude_rad;
}

void oskar_station_set_gaussian_beam(oskar_Station* model,
        double fwhm_rad, double ref_freq_hz)
{
    model->gaussian_beam_fwhm_rad = fwhm_rad;
    model->gaussian_beam_reference_freq_hz = ref_freq_hz;
}

void oskar_station_set_use_polarised_elements(oskar_Station* model, int value)
{
    model->use_polarised_elements = value;
}

void oskar_station_set_normalise_array_pattern(oskar_Station* model, int value)
{
    model->normalise_array_pattern = value;
}

void oskar_station_set_enable_array_pattern(oskar_Station* model, int value)
{
    model->enable_array_pattern = value;
}

#ifdef __cplusplus
}
#endif
