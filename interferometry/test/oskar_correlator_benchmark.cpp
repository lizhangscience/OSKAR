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

#include <oskar_correlate.h>
#include <oskar_telescope.h>
#include <oskar_sky.h>
#include <oskar_jones.h>
#include <oskar_mem.h>
#include <oskar_get_error_string.h>
#include <oskar_timer.h>

#include <apps/lib/oskar_OptionParser.h>

#ifndef _WIN32
#   include <sys/time.h>
#endif /* _WIN32 */
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>

int benchmark(int num_stations, int num_sources, int type,
        int jones_type, int loc, int use_extended, int use_time_ave, int niter,
        std::vector<double>& times);

int main(int argc, char** argv)
{
    oskar_OptionParser opt("oskar_correlator_benchmark");
    opt.addFlag("-nst", "Number of stations.", 1, "", true);
    opt.addFlag("-nsrc", "Number of sources.", 1, "", true);
    opt.addFlag("-sp", "Use single precision (default: double precision)");
    opt.addFlag("-s", "Use scalar Jones terms (default: matrix/polarised).");
    opt.addFlag("-g", "Run on the GPU");
    opt.addFlag("-c", "Run on the CPU");
    opt.addFlag("-e", "Use Gaussian sources (default: point sources).");
    opt.addFlag("-t", "Use analytical time averaging (default: no time "
            "averaging).");
    opt.addFlag("-r", "Dump raw iteration data to this file.", 1);
    opt.addFlag("-std", "Discard values greater than this number of standard "
            "deviations from the mean.", 1);
    opt.addFlag("-n", "Number of iterations", 1, "1", false);
    opt.addFlag("-v", "Display verbose output.", false);
    if (!opt.check_options(argc, argv))
        return EXIT_FAILURE;

    int num_stations, num_sources, niter;
    double max_std_dev = 0.0;
    opt.get("-nst")->getInt(num_stations);
    opt.get("-nsrc")->getInt(num_sources);
    int type = opt.isSet("-sp") ? OSKAR_SINGLE : OSKAR_DOUBLE;
    int jones_type = type | OSKAR_COMPLEX;
    if (!opt.isSet("-s"))
        jones_type |= OSKAR_MATRIX;
    opt.get("-n")->getInt(niter);
    int use_extended = opt.isSet("-e") ? OSKAR_TRUE : OSKAR_FALSE;
    int use_time_ave = opt.isSet("-t") ? OSKAR_TRUE : OSKAR_FALSE;
    std::string raw_file;
    if (opt.isSet("-r"))
        opt.get("-r")->getString(raw_file);
    if (opt.isSet("-std"))
        opt.get("-std")->getDouble(max_std_dev);

    int loc;
    if (opt.isSet("-g"))
        loc = OSKAR_GPU;
    if (opt.isSet("-c"))
        loc = OSKAR_CPU;
    if (!(opt.isSet("-c") ^ opt.isSet("-g")))
    {
        opt.error("Please select one of -g or -c");
        return EXIT_FAILURE;
    }

    if (opt.isSet("-v"))
    {
        printf("\n");
        printf("- Number of stations: %i\n", num_stations);
        printf("- Number of sources: %i\n", num_sources);
        printf("- Precision: %s\n", (type == OSKAR_SINGLE) ? "single" : "double");
        printf("- Jones type: %s\n", (opt.isSet("-s")) ? "scalar" : "matrix");
        printf("- Extended sources: %s\n", (use_extended) ? "true" : "false");
        printf("- Analytical time smearing: %s\n", (use_time_ave) ? "true" : "false");
        printf("- Number of iterations: %i\n", niter);
        if (max_std_dev > 0.0)
            printf("- Max standard deviations: %f\n", max_std_dev);
        if (!raw_file.empty())
            printf("- Writing iteration data to: %s\n", raw_file.c_str());
        printf("\n");
    }

    // Run benchmarks.
    double time_taken_sec = 0.0, average_time_sec = 0.0;
    std::vector<double> times;
    int status = benchmark(num_stations, num_sources, type, jones_type,
            loc, use_extended, use_time_ave, niter, times);

    // Compute total time taken.
    for (int i = 0; i < niter; ++i)
    {
        time_taken_sec += times[i];
    }

    // Dump raw data if requested.
    if (!raw_file.empty())
    {
        FILE* raw_stream = 0;
        raw_stream = fopen(raw_file.c_str(), "w");
        if (raw_stream)
        {
            for (int i = 0; i < niter; ++i)
            {
                fprintf(raw_stream, "%.6f\n", times[i]);
            }
            fclose(raw_stream);
        }
    }

    // Check for errors.
    if (status)
    {
        fprintf(stderr, "ERROR: correlate failed with code %i: %s\n", status,
                oskar_get_error_string(status));
        return EXIT_FAILURE;
    }

    // Compute average.
    if (max_std_dev > 0.0)
    {
        double std_dev_sec = 0.0, old_time_average_sec;

        // Compute standard deviation.
        old_time_average_sec = time_taken_sec / niter;
        for (int i = 0; i < niter; ++i)
        {
            std_dev_sec += pow(times[i] - old_time_average_sec, 2.0);
        }
        std_dev_sec /= niter;
        std_dev_sec = sqrt(std_dev_sec);

        // Compute new mean.
        average_time_sec = 0.0;
        int counter = 0;
        for (int i = 0; i < niter; ++i)
        {
            if (fabs(times[i] - old_time_average_sec) <
                    max_std_dev * std_dev_sec)
            {
                average_time_sec += times[i];
                counter++;
            }
        }
        if (counter)
            average_time_sec /= counter;
    }
    else
    {
        average_time_sec = time_taken_sec / niter;
    }

    // Print average.
    if (opt.isSet("-v"))
    {
        printf("==> Total time taken: %f seconds.\n", time_taken_sec);
        printf("==> Time taken per iteration: %f seconds.\n", average_time_sec);
        printf("==> Iteration values:\n");
        for (int i = 0; i < niter; ++i)
        {
            printf("%.6f\n", times[i]);
        }
        printf("\n");
    }
    else
    {
        printf("%f\n", average_time_sec);
    }

    return EXIT_SUCCESS;
}

int benchmark(int num_stations, int num_sources, int type,
        int jones_type, int loc, int use_extended, int use_time_ave, int niter,
        std::vector<double>& times)
{
    int status = 0;
    int num_vis = num_stations * (num_stations-1) / 2;

    oskar_Timer* timer;
    timer = oskar_timer_create(loc == OSKAR_GPU ?
            OSKAR_TIMER_CUDA : OSKAR_TIMER_OMP);

    // Set up a test sky model, telescope model and Jones matrices.
    oskar_Telescope* tel = oskar_telescope_create(type, loc,
            num_stations, &status);
    oskar_Sky* sky = oskar_sky_create(type, loc, num_sources, &status);
    oskar_Jones* J = oskar_jones_create(jones_type, loc, num_stations,
            num_sources, &status);

    oskar_telescope_set_smearing_values(tel, 1e6, (double) use_time_ave);
    oskar_sky_set_use_extended(sky, use_extended);

    // Memory for visibility coordinates and output visibility slice.
    oskar_Mem *vis, *u, *v;
    vis = oskar_mem_create(jones_type, loc, num_vis, &status);
    u = oskar_mem_create(type, loc, num_stations, &status);
    v = oskar_mem_create(type, loc, num_stations, &status);

    // Run benchmark.
    times.resize(niter);
    for (int i = 0; i < niter; ++i)
    {
        oskar_timer_start(timer);
        oskar_correlate(vis, oskar_sky_num_sources(sky), J, sky, tel, u, v,
                0.0, 100e6, &status);
        times[i] = oskar_timer_elapsed(timer);
    }

    // Free memory.
    oskar_mem_free(u, &status);
    oskar_mem_free(v, &status);
    oskar_mem_free(vis, &status);
    oskar_jones_free(J, &status);
    oskar_telescope_free(tel, &status);
    oskar_sky_free(sky, &status);
    oskar_timer_free(timer);
    return status;
}
