/*
 * Configure file of SSD-SMR cache system.
 * All the switch is turn-off by default.
 */
/** configure of system structure **/
#undef NO_REAL_DISK_IO

#define NO_CACHE

#undef CACHE_PROPORTIOIN_STATIC
#undef NO_READ_CACHE
/**< Statistic information requirments defination */

#define  LOG_ALLOW // Log allowed EXCLUSIVELY for 1. Print the pcb by CM. 2. Print the WA by Emulator.
#undef  LOG_SINGLE_REQ  // Print detail time information of each single request.

/** Emulator Related **/
#undef SIMULATION
#undef SIMULATOR_AIO
#undef SIMU_NO_DISK_IO

/** Daemon Thread **/
#undef DAEMON_PROC
#undef DAEMON_BANDWIDHT
#undef DAEMON_CACHE_RUNTIME
#undef DAEON_SMR_RUNTIME

#define WRITE_IN_BATCH

/** T-Switcher **/
#undef T_SWITCHER_ON

/* Future Features */
#undef HRC_PROCS_N 10
#undef CG_THROTTLE     // CGroup throttle.
#undef MULTIUSER
