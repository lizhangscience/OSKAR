/*
 * Copyright (c) 2011-2014, The University of Oxford
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

#include <private_vis.h>
#include <oskar_vis.h>

#ifdef __cplusplus
extern "C" {
#endif

oskar_Vis* oskar_vis_create(int amp_type, int location, int num_channels,
        int num_times, int num_stations, int* status)
{
    oskar_Vis* vis = 0;
    int type, num_amps, num_coords, num_baselines;

    /* Check all inputs. */
    if (!status)
    {
        oskar_set_invalid_argument(status);
        return 0;
    }

    /* Check type. */
    if (oskar_mem_type_is_double(amp_type))
        type = OSKAR_DOUBLE;
    else if (oskar_mem_type_is_single(amp_type))
        type = OSKAR_SINGLE;
    else
    {
        *status = OSKAR_ERR_BAD_DATA_TYPE;
        return 0;
    }
    if (!oskar_mem_type_is_complex(amp_type))
    {
        *status = OSKAR_ERR_BAD_DATA_TYPE;
        return 0;
    }

    /* Check location. */
    if (location != OSKAR_GPU && location != OSKAR_CPU)
    {
        *status = OSKAR_ERR_BAD_LOCATION;
        return 0;
    }

    /* Allocate the structure. */
    vis = (oskar_Vis*) malloc(sizeof(oskar_Vis));

    /* Set dimensions. */
    num_baselines = num_stations * (num_stations - 1) / 2;
    vis->num_stations  = num_stations;
    vis->num_channels  = num_channels;
    vis->num_times     = num_times;
    vis->num_baselines = num_baselines;
    num_amps   = num_channels * num_times * num_baselines;
    num_coords = num_times * num_baselines;

    /* Initialise meta-data. */
    vis->freq_start_hz = 0.0;
    vis->freq_inc_hz = 0.0;
    vis->channel_bandwidth_hz = 0.0;
    vis->time_start_mjd_utc = 0.0;
    vis->time_inc_sec = 0.0;
    vis->time_average_sec = 0.0;
    vis->phase_centre_ra_deg = 0.0;
    vis->phase_centre_dec_deg = 0.0;
    vis->telescope_lon_deg = 0.0;
    vis->telescope_lat_deg = 0.0;
    vis->telescope_alt_metres = 0.0;

    /* Initialise memory. */
    vis->settings_path = oskar_mem_create(OSKAR_CHAR, OSKAR_CPU, 0, status);
    vis->telescope_path = oskar_mem_create(OSKAR_CHAR, OSKAR_CPU, 0, status);
    vis->settings = oskar_mem_create(OSKAR_CHAR, OSKAR_CPU, 0, status);
    vis->station_x_offset_ecef_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_y_offset_ecef_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_z_offset_ecef_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_x_enu_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_y_enu_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_z_enu_metres = oskar_mem_create(type, location, num_stations, status);
    vis->station_lon_deg = oskar_mem_create(type, location, num_stations, status);
    vis->station_lat_deg = oskar_mem_create(type, location, num_stations, status);
    vis->station_orientation_x_deg = oskar_mem_create(type, location, num_stations, status);
    vis->station_orientation_y_deg = oskar_mem_create(type, location, num_stations, status);
    vis->baseline_uu_metres = oskar_mem_create(type, location, num_coords, status);
    vis->baseline_vv_metres = oskar_mem_create(type, location, num_coords, status);
    vis->baseline_ww_metres = oskar_mem_create(type, location, num_coords, status);
    vis->amplitude = oskar_mem_create(amp_type, location, num_amps, status);

    /* Return handle to structure. */
    return vis;
}

#ifdef __cplusplus
}
#endif
