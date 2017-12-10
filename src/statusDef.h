/*
 * Configure file of SSD-SMR cache system.
 * All the switch is turn-off by default.
 */
/** configure of system structure **/
#define NO_REAL_DISK_IO

#undef NO_CACHE

#undef CG_THROTTLE     // CGroup throttle.
#undef MULTIUSER

#define CACHE_PROPORTIOIN_STATIC

/**< Statistic information requirments defination */
#undef  LOG_ALLOW
#undef  LOG_SINGLE_REQ  // Print detail time information of each single request.

/** Simulator Related **/
#define SIMULATION
#undef SIMULATOR_AIO
#define SIMU_NO_DISK_IO

/** Daemon process **/
#undef DAEMON_PROC
#undef DAEMON_BANDWIDHT
#undef DAEMON_CACHE_RUNTIME
#undef DAEON_SMR_RUNTIME

#undef PORE_BATCH
