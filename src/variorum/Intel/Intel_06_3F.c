// Copyright 2019-2023 Lawrence Livermore National Security, LLC and other
// Variorum Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>

#include <Intel_06_3F.h>
#include <clocks_features.h>
#include <config_architecture.h>
#include <counters_features.h>
#include <misc_features.h>
#include <intel_power_features.h>
#include <thermal_features.h>

#ifdef CPRINTF_FOUND
#include <cprintf.h>
#endif

static struct haswell_3f_offsets msrs =
{
    .msr_platform_info            = 0xCE,
    .ia32_time_stamp_counter      = 0x10,
    .ia32_perf_ctl                = 0x199,
    .ia32_perf_status             = 0x198,
    .ia32_therm_interrupt         = 0x19B,
    .ia32_therm_status            = 0x19C,
    .ia32_misc_enable             = 0x1A0,
    .msr_temperature_target       = 0x1A2,
    .msr_turbo_ratio_limit        = 0x1AD,
    .msr_turbo_ratio_limit1       = 0x1AE,
    .msr_config_tdp_level1        = 0x649,
    .msr_config_tdp_level2        = 0x64A,
    .msr_config_tdp_nominal       = 0x648,
    .ia32_package_therm_status    = 0x1B1,
    .ia32_package_therm_interrupt = 0x1B2,
    .ia32_fixed_counters[0]       = 0x309,
    .ia32_fixed_counters[1]       = 0x30A,
    .ia32_fixed_counters[2]       = 0x30B,
    .ia32_fixed_ctr_ctrl          = 0x38D,
    .ia32_perf_global_status      = 0x38E,
    .ia32_perf_global_ctrl        = 0x38F,
    .ia32_perf_global_ovf_ctrl    = 0x390,
    .msr_rapl_power_unit          = 0x606,
    .msr_pkg_power_limit          = 0x610,
    .msr_pkg_energy_status        = 0x611,
    .msr_pkg_perf_status          = 0x613,
    .msr_pkg_power_info           = 0x614,
    .msr_dram_power_limit         = 0x618,
    .msr_dram_energy_status       = 0x619,
    .msr_dram_perf_status         = 0x61B,
    .msr_dram_power_info          = 0x61C,
    .msr_turbo_activation_ratio   = 0x64C,
    .ia32_mperf                   = 0xE7,
    .ia32_aperf                   = 0xE8,
    .ia32_perfmon_counters[0]     = 0xC1,
    .ia32_perfmon_counters[1]     = 0xC2,
    .ia32_perfmon_counters[2]     = 0xC3,
    .ia32_perfmon_counters[3]     = 0xC4,
    .ia32_perfmon_counters[4]     = 0xC5,
    .ia32_perfmon_counters[5]     = 0xC6,
    .ia32_perfmon_counters[6]     = 0xC7,
    .ia32_perfmon_counters[7]     = 0xC8,
    .ia32_perfevtsel_counters[0]  = 0x186,
    .ia32_perfevtsel_counters[1]  = 0x187,
    .ia32_perfevtsel_counters[2]  = 0x188,
    .ia32_perfevtsel_counters[3]  = 0x189,
    .ia32_perfevtsel_counters[4]  = 0x18A,
    .ia32_perfevtsel_counters[5]  = 0x18B,
    .ia32_perfevtsel_counters[6]  = 0x18C,
    .ia32_perfevtsel_counters[7]  = 0x18D,
};

int intel_cpu_fm_06_3f_get_power_limits(int long_ver)
{
    unsigned socket;
    unsigned nsockets, ncores, nthreads;
#ifdef VARIORUM_WITH_INTEL_CPU
    variorum_get_topology(&nsockets, &ncores, &nthreads, P_INTEL_CPU_IDX);
#endif

    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    for (socket = 0; socket < nsockets; socket++)
    {
        if (long_ver == 0)
        {
            print_package_power_limit(stdout, msrs.msr_pkg_power_limit,
                                      msrs.msr_rapl_power_unit, socket);
        }
        else if (long_ver == 1)
        {
            print_verbose_package_power_limit(stdout, msrs.msr_pkg_power_limit,
                                              msrs.msr_rapl_power_unit, socket);
        }
    }

    for (socket = 0; socket < nsockets; socket++)
    {
        if (long_ver == 0)
        {
            print_dram_power_limit(stdout, msrs.msr_dram_power_limit,
                                   msrs.msr_rapl_power_unit, socket);
        }
        else if (long_ver == 1)
        {
            print_verbose_dram_power_limit(stdout, msrs.msr_dram_power_limit,
                                           msrs.msr_rapl_power_unit, socket);
        }
    }

    for (socket = 0; socket < nsockets; socket++)
    {
        if (long_ver == 0)
        {
            print_package_power_info(stdout, msrs.msr_pkg_power_info, socket);
        }
        else if (long_ver == 1)
        {
            print_verbose_package_power_info(stdout, msrs.msr_pkg_power_info, socket);
        }
    }

    for (socket = 0; socket < nsockets; socket++)
    {
        if (long_ver == 0)
        {
            print_dram_power_info(stdout, msrs.msr_dram_power_info, socket);
        }
        else if (long_ver == 1)
        {
            print_verbose_dram_power_info(stdout, msrs.msr_dram_power_info, socket);
        }
    }

    if (long_ver == 0)
    {
        print_rapl_power_unit(stdout, msrs.msr_rapl_power_unit);
    }
    else if (long_ver == 1)
    {
        print_verbose_rapl_power_unit(stdout, msrs.msr_rapl_power_unit);
    }

    return 0;
}

int intel_cpu_fm_06_3f_cap_power_limits(int package_power_limit)
{
    unsigned socket;
    unsigned nsockets, ncores, nthreads;
#ifdef VARIORUM_WITH_INTEL_CPU
    variorum_get_topology(&nsockets, &ncores, &nthreads, P_INTEL_CPU_IDX);
#endif

    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    for (socket = 0; socket < nsockets; socket++)
    {
        cap_package_power_limit(socket, package_power_limit, msrs.msr_pkg_power_limit,
                                msrs.msr_rapl_power_unit);
    }
    return 0;
}

int intel_cpu_fm_06_3f_get_features(void)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

//Really not sure if this makes sense to use libjust here but the spaced
// out format strings used normally is a bit weird.
#ifdef CPRINTF_FOUND
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_platform_info",
             msrs.msr_platform_info);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_time_stamp_counter",
                msrs.ia32_time_stamp_counter);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_perf_ctl", msrs.ia32_perf_ctl);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_perf_status",
                msrs.ia32_perf_status);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_therm_interrupt",
                msrs.ia32_therm_interrupt);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_therm_status", 
                msrs.ia32_therm_status);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_misc_enable",
                msrs.ia32_misc_enable);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_temperature_target",
                msrs.msr_temperature_target);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_turbo_ratio_limit",
                msrs.msr_turbo_ratio_limit);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_turbo_ratio_limit1",
                msrs.msr_turbo_ratio_limit1);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_package_therm_status",
                msrs.ia32_package_therm_status);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_package_therm_interrupt",
                msrs.ia32_package_therm_interrupt);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_fixed_counters[0]",
                msrs.ia32_fixed_counters[0]);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_fixed_counters[1]",
                msrs.ia32_fixed_counters[1]);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_fixed_counters[2]",
                msrs.ia32_fixed_counters[2]);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_fixed_ctr_ctrl",
                msrs.ia32_fixed_ctr_ctrl);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_perf_global_status",
                msrs.ia32_perf_global_status);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_perf_global_ctrl",
                msrs.ia32_perf_global_ctrl);
    cfprintf(stdout, "%s = 0x%-lx\n", "ia32_perf_global_ovf_ctrl",
                msrs.ia32_perf_global_ovf_ctrl);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_rapl_power_unit",
                msrs.msr_rapl_power_unit);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_pkg_power_limit",
                msrs.msr_pkg_power_limit);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_pkg_energy_status",
                msrs.msr_pkg_energy_status);
    cfprintf(stdout, "%s = 0x%-lx\n", "msr_pkg_perf_status",
                msrs.msr_pkg_perf_status);
    cfprintf(stdout, "%s = 0x%-lx", "msr_pkg_power_info",
            msrs.msr_pkg_power_info);
    cfprintf(stdout, "%s = 0x%-lx", "msr_dram_power_limit",
            msrs.msr_dram_power_limit);
    cfprintf(stdout, "%s = 0x%-lx", "msr_dram_energy_status",
            msrs.msr_dram_energy_status);
    cfprintf(stdout, "%s = 0x%-lx", "msr_dram_perf_status",
            msrs.msr_dram_perf_status);
    cfprintf(stdout, "%s = 0x%-lx", "msr_dram_power_info",
            msrs.msr_dram_power_info);
    cfprintf(stdout, "%s = 0x%-lx", "msr_turbo_activation_rat",
            msrs.msr_turbo_activation_ratio);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_mperf", msrs.ia32_mperf);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_aperf", msrs.ia32_aperf);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[0]",
            msrs.ia32_perfmon_counters[0]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[1]",
            msrs.ia32_perfmon_counters[1]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[2]",
            msrs.ia32_perfmon_counters[2]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[3]",
            msrs.ia32_perfmon_counters[3]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[4]",
            msrs.ia32_perfmon_counters[4]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[5]",
            msrs.ia32_perfmon_counters[5]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[6]",
            msrs.ia32_perfmon_counters[6]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfmon_counters[7]",
            msrs.ia32_perfmon_counters[7]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[0]",
            msrs.ia32_perfevtsel_counters[0]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[1]",
            msrs.ia32_perfevtsel_counters[1]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[2]",
            msrs.ia32_perfevtsel_counters[2]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[3]",
            msrs.ia32_perfevtsel_counters[3]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[4]",
            msrs.ia32_perfevtsel_counters[4]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[5]",
            msrs.ia32_perfevtsel_counters[5]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[6]",
            msrs.ia32_perfevtsel_counters[6]);
    cfprintf(stdout, "%s = 0x%-lx", "ia32_perfevtsel_counters[7]",
            msrs.ia32_perfevtsel_counters[7]);

    cflush();
#else
    fprintf(stdout, "msr_platform_info            = 0x%lx\n",
            msrs.msr_platform_info);
    fprintf(stdout, "ia32_time_stamp_counter      = 0x%lx\n",
            msrs.ia32_time_stamp_counter);
    fprintf(stdout, "ia32_perf_ctl                = 0x%lx\n", msrs.ia32_perf_ctl);
    fprintf(stdout, "ia32_perf_status             = 0x%lx\n",
            msrs.ia32_perf_status);
    fprintf(stdout, "ia32_therm_interrupt         = 0x%lx\n",
            msrs.ia32_therm_interrupt);
    fprintf(stdout, "ia32_therm_status            = 0x%lx\n",
            msrs.ia32_therm_status);
    fprintf(stdout, "ia32_misc_enable             = 0x%lx\n",
            msrs.ia32_misc_enable);
    fprintf(stdout, "msr_temperature_target       = 0x%lx\n",
            msrs.msr_temperature_target);
    fprintf(stdout, "msr_turbo_ratio_limit        = 0x%lx\n",
            msrs.msr_turbo_ratio_limit);
    fprintf(stdout, "msr_turbo_ratio_limit1       = 0x%lx\n",
            msrs.msr_turbo_ratio_limit1);
    fprintf(stdout, "ia32_package_therm_status    = 0x%lx\n",
            msrs.ia32_package_therm_status);
    fprintf(stdout, "ia32_package_therm_interrupt = 0x%lx\n",
            msrs.ia32_package_therm_interrupt);
    fprintf(stdout, "ia32_fixed_counters[0]       = 0x%lx\n",
            msrs.ia32_fixed_counters[0]);
    fprintf(stdout, "ia32_fixed_counters[1]       = 0x%lx\n",
            msrs.ia32_fixed_counters[1]);
    fprintf(stdout, "ia32_fixed_counters[2]       = 0x%lx\n",
            msrs.ia32_fixed_counters[2]);
    fprintf(stdout, "ia32_fixed_ctr_ctrl          = 0x%lx\n",
            msrs.ia32_fixed_ctr_ctrl);
    fprintf(stdout, "ia32_perf_global_status      = 0x%lx\n",
            msrs.ia32_perf_global_status);
    fprintf(stdout, "ia32_perf_global_ctrl        = 0x%lx\n",
            msrs.ia32_perf_global_ctrl);
    fprintf(stdout, "ia32_perf_global_ovf_ctrl    = 0x%lx\n",
            msrs.ia32_perf_global_ovf_ctrl);
    fprintf(stdout, "msr_rapl_power_unit          = 0x%lx\n",
            msrs.msr_rapl_power_unit);
    fprintf(stdout, "msr_pkg_power_limit          = 0x%lx\n",
            msrs.msr_pkg_power_limit);
    fprintf(stdout, "msr_pkg_energy_status        = 0x%lx\n",
            msrs.msr_pkg_energy_status);
    fprintf(stdout, "msr_pkg_perf_status          = 0x%lx\n",
            msrs.msr_pkg_perf_status);
    fprintf(stdout, "msr_pkg_power_info           = 0x%lx\n",
            msrs.msr_pkg_power_info);
    fprintf(stdout, "msr_dram_power_limit         = 0x%lx\n",
            msrs.msr_dram_power_limit);
    fprintf(stdout, "msr_dram_energy_status       = 0x%lx\n",
            msrs.msr_dram_energy_status);
    fprintf(stdout, "msr_dram_perf_status         = 0x%lx\n",
            msrs.msr_dram_perf_status);
    fprintf(stdout, "msr_dram_power_info           = 0x%lx\n",
            msrs.msr_dram_power_info);
    fprintf(stdout, "msr_turbo_activation_ratio   = 0x%lx\n",
            msrs.msr_turbo_activation_ratio);
    fprintf(stdout, "ia32_mperf                   = 0x%lx\n", msrs.ia32_mperf);
    fprintf(stdout, "ia32_aperf                   = 0x%lx\n", msrs.ia32_aperf);
    fprintf(stdout, "ia32_perfmon_counters[0]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[0]);
    fprintf(stdout, "ia32_perfmon_counters[1]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[1]);
    fprintf(stdout, "ia32_perfmon_counters[2]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[2]);
    fprintf(stdout, "ia32_perfmon_counters[3]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[3]);
    fprintf(stdout, "ia32_perfmon_counters[4]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[4]);
    fprintf(stdout, "ia32_perfmon_counters[5]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[5]);
    fprintf(stdout, "ia32_perfmon_counters[6]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[6]);
    fprintf(stdout, "ia32_perfmon_counters[7]     = 0x%lx\n",
            msrs.ia32_perfmon_counters[7]);
    fprintf(stdout, "ia32_perfevtsel_counters[0]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[0]);
    fprintf(stdout, "ia32_perfevtsel_counters[1]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[1]);
    fprintf(stdout, "ia32_perfevtsel_counters[2]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[2]);
    fprintf(stdout, "ia32_perfevtsel_counters[3]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[3]);
    fprintf(stdout, "ia32_perfevtsel_counters[4]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[4]);
    fprintf(stdout, "ia32_perfevtsel_counters[5]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[5]);
    fprintf(stdout, "ia32_perfevtsel_counters[6]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[6]);
    fprintf(stdout, "ia32_perfevtsel_counters[7]  = 0x%lx\n",
            msrs.ia32_perfevtsel_counters[7]);

#endif

    return 0;
}

int intel_cpu_fm_06_3f_get_thermals(int long_ver)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    if (long_ver == 0)
    {
        print_therm_temp_reading(stdout, msrs.ia32_therm_status,
                                 msrs.ia32_package_therm_status, msrs.msr_temperature_target);
    }
    else if (long_ver == 1)
    {
        print_verbose_therm_temp_reading(stdout, msrs.ia32_therm_status,
                                         msrs.ia32_package_therm_status, msrs.msr_temperature_target);
    }
    return 0;
}

int intel_cpu_fm_06_3f_get_counters(int long_ver)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    if (long_ver == 0)
    {
        print_all_counter_data(stdout, msrs.ia32_fixed_counters,
                               msrs.ia32_perfevtsel_counters, msrs.ia32_perfmon_counters,
                               msrs.msrs_pcu_pmon_evtsel, msrs.ia32_perfevtsel_counters);
    }
    else if (long_ver == 1)
    {
        print_verbose_all_counter_data(stdout, msrs.ia32_fixed_counters,
                                       msrs.ia32_perfevtsel_counters, msrs.ia32_perfmon_counters,
                                       msrs.msrs_pcu_pmon_evtsel, msrs.ia32_perfevtsel_counters);
    }
    return 0;
}

int intel_cpu_fm_06_3f_get_clocks(int long_ver)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    if (long_ver == 0)
    {
        print_clocks_data(stdout, msrs.ia32_aperf, msrs.ia32_mperf,
                          msrs.ia32_time_stamp_counter, msrs.ia32_perf_status, msrs.msr_platform_info,
                          CORE);
    }
    else if (long_ver == 1)
    {
        print_verbose_clocks_data(stdout, msrs.ia32_aperf, msrs.ia32_mperf,
                                  msrs.ia32_time_stamp_counter, msrs.ia32_perf_status, msrs.msr_platform_info,
                                  CORE);
    }
    return 0;
}

int intel_cpu_fm_06_3f_get_power(int long_ver)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    if (long_ver == 0)
    {
        print_power_data(stdout, msrs.msr_rapl_power_unit, msrs.msr_pkg_energy_status,
                         msrs.msr_dram_energy_status);
    }
    else if (long_ver == 1)
    {
        print_verbose_power_data(stdout, msrs.msr_rapl_power_unit,
                                 msrs.msr_pkg_energy_status, msrs.msr_dram_energy_status);
    }
    return 0;
}

int intel_cpu_fm_06_3f_enable_turbo(void)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    unsigned int turbo_mode_disable_bit = 38;
    set_turbo_on(msrs.ia32_misc_enable, turbo_mode_disable_bit);

    return 0;
}

int intel_cpu_fm_06_3f_disable_turbo(void)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    unsigned int turbo_mode_disable_bit = 38;
    set_turbo_off(msrs.ia32_misc_enable, turbo_mode_disable_bit);

    return 0;
}
int intel_cpu_fm_06_3f_get_turbo_status(void)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    unsigned int turbo_mode_disable_bit = 38;
    print_turbo_status(stdout, msrs.ia32_misc_enable, turbo_mode_disable_bit);

    return 0;
}

int intel_cpu_fm_06_3f_poll_power(FILE *output)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    get_all_power_data(output, msrs.msr_pkg_power_limit, msrs.msr_dram_power_limit,
                       msrs.msr_rapl_power_unit, msrs.msr_pkg_energy_status,
                       msrs.msr_dram_energy_status);
    return 0;
}

int intel_cpu_fm_06_3f_monitoring(FILE *output)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    get_all_power_data_fixed(output, msrs.msr_pkg_power_limit,
                             msrs.msr_dram_power_limit, msrs.msr_rapl_power_unit, msrs.msr_pkg_energy_status,
                             msrs.msr_dram_energy_status, msrs.ia32_fixed_counters,
                             msrs.ia32_perf_global_ctrl, msrs.ia32_fixed_ctr_ctrl, msrs.ia32_aperf,
                             msrs.ia32_mperf, msrs.ia32_time_stamp_counter);
    return 0;
}

int intel_cpu_fm_06_3f_get_node_power_json(char **get_power_obj_str)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    json_t *get_power_obj = json_object();

    json_get_power_data(get_power_obj, msrs.msr_pkg_power_limit,
                        msrs.msr_rapl_power_unit, msrs.msr_pkg_energy_status,
                        msrs.msr_dram_energy_status);

    *get_power_obj_str = json_dumps(get_power_obj, 0);
    json_decref(get_power_obj);

    return 0;
}

int intel_cpu_fm_06_3f_get_node_power_domain_info_json(char
        **get_domain_obj_str)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    json_t *get_domain_obj = json_object();

    json_get_power_domain_info(get_domain_obj, msrs.msr_pkg_power_info,
                               msrs.msr_dram_power_info, msrs.msr_rapl_power_unit, msrs.msr_pkg_power_limit);

    *get_domain_obj_str = json_dumps(get_domain_obj, 0);
    json_decref(get_domain_obj);

    return 0;
}

int intel_cpu_fm_06_3f_cap_best_effort_node_power_limit(int node_limit)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    /* We make an assumption here to uniformly distribute the specified
     * power to both sockets as socket-level power caps. We are not accounting
     * for memory power or uncore power at the moment. We will develop a model
     * for this in the future.
     * When an odd number value is provided, we want this to result in
     * the floor of the value being taken. So while we will be off by 1W total,
     * we will guarantee that we stay under the specified cap. */

    unsigned nsockets, ncores, nthreads;
#ifdef VARIORUM_WITH_INTEL_CPU
    variorum_get_topology(&nsockets, &ncores, &nthreads, P_INTEL_CPU_IDX);
#endif

    // Adding this for portability and rounding down.
    // Ideally this should be okay as it is integer division and we have
    // two sockets only.

    int remainder = node_limit % nsockets;
    node_limit = (remainder == 0) ? node_limit : (node_limit - remainder);

    int pkg_limit = node_limit / nsockets;

    intel_cpu_fm_06_3f_cap_power_limits(pkg_limit);

    return 0;
}

int intel_cpu_fm_06_3f_get_frequencies(void)
{
    char *val = getenv("VARIORUM_LOG");
    if (val != NULL && atoi(val) == 1)
    {
        printf("Running %s\n", __FUNCTION__);
    }

    get_available_frequencies(stdout, &msrs.msr_platform_info,
                              &msrs.msr_turbo_ratio_limit, &msrs.msr_turbo_ratio_limit1,
                              &msrs.msr_config_tdp_level1, &msrs.msr_config_tdp_level2);
    return 0;
}
