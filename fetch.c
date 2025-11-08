
/*
 * fetch.c - Public domain.
 * A non-portable pfetch clone written in C for OpenBSD.
 * 
 * Run `make obj` before running `make`. This has only been tested on OpenBSD,
 * and will likely not run on anything else.
 *
 * The ASCII art was stolen from pfetch.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include <pwd.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/utsname.h>
#include <time.h>
#include <stdckdint.h>
#include <math.h>

void
getUserData(char **userDataString)
{
	struct passwd *pw = nullptr;
	char hostname[HOST_NAME_MAX + 1] = { 0 };
	short userDataStringLength = 0;
	char *u = nullptr;

	if ((pw = getpwuid(getuid())) == nullptr)
		err(1, "getpwuid");

	if (gethostname(hostname, HOST_NAME_MAX + 1) == -1)
		err(1, "gethostname");

	userDataStringLength = strnlen(pw->pw_name, _PW_NAME_LEN + 1) +
				strnlen(hostname, HOST_NAME_MAX + 1) +
				2; /* '@', and '\0' */

	if ((u = calloc(1, userDataStringLength)) == nullptr)
		err(1, "calloc");

	snprintf(u, userDataStringLength, "%s@%s", pw->pw_name, hostname);
	*userDataString = u;
}

void
getSystemData(char **systemDataString)
{
	struct utsname u;
	int systemDataStringLength = 0;
	const char systemTag[] = "System:\t";
	char *s = nullptr;

	if (uname(&u) == -1)
		err(1, "uname");

	systemDataStringLength = strlen(systemTag) +
				strnlen(u.sysname, SYS_NMLN) +
				strnlen(u.release, SYS_NMLN) +
				strnlen(u.machine, SYS_NMLN) +
				3;

	if ((s = calloc(1, systemDataStringLength)) == nullptr)
		err(1, "calloc");

	snprintf(s, systemDataStringLength, "%s%s %s %s", systemTag, u.sysname,
							u.release, u.machine);
	*systemDataString = s;
}

void
getHostData(char **hostDataString)
{
	int mib[2] = { 0 };
	long hwVendorLength = 0, hwProductLength = 0;
	long hostDataStringLength = 0;
	long insertPoint = 0;
	const char hostTag[] = "Host:\t";
	char *h = nullptr;

	mib[0] = CTL_HW;
	mib[1] = HW_VENDOR;
	if (sysctl(mib, 2, nullptr, &hwVendorLength, nullptr, 0) == -1)
		err(1, "sysctl");

	mib[1] = HW_PRODUCT;
	if (sysctl(mib, 2, nullptr, &hwProductLength, nullptr, 0) == -1)
		err(1, "sysctl");

	/*
	 * sysctl length includes the '\0'; providing us room for one ' '
	 * and one '\0' by default.
	 */
	if (ckd_add(&hostDataStringLength,
				hwVendorLength,
				hwProductLength) ||
	    ckd_add(&hostDataStringLength,
				hostDataStringLength,
				strlen(hostTag)))
		errx(1, "ckd_add: integer overflow");

	if ((h = calloc(1, hostDataStringLength)) == nullptr)
		err(1, "calloc");

	snprintf(h, strlen(hostTag) + 1, "%s", hostTag);
	insertPoint += strlen(hostTag);

	mib[1] = HW_VENDOR;
	if (sysctl(mib, 2, h + insertPoint, &hwVendorLength, nullptr, 0) == -1)
		err(1, "sysctl");
	insertPoint += hwVendorLength;

	h[insertPoint - 1] = ' ';

	mib[1] = HW_PRODUCT;
	if (sysctl(mib, 2, h + insertPoint, &hwProductLength, nullptr, 0) == -1)
		err(1, "sysctl");

	*hostDataString = h;
}

/* Approximate number of characters required to store an integer. */
#define NUM_LENGTH(x) ((floor(log10(x))) + 1)

void
getUptimeData(char **uptimeDataString)
{
	struct timespec	boottime;
	time_t uptime;
	time_t timeVals[4] = { 0 };
	const char *timeNames[] = {"days", "hours", "minutes", "seconds"};
	short uptimeDataStringLength = 0;
	const char uptimeTag[] = "Uptime:\t";
	char *u = nullptr;

	if (clock_gettime(CLOCK_BOOTTIME, &boottime) == -1)
		err(1, "clock_gettime");
	else
		uptime = boottime.tv_sec;

	/* Day, hour, minute, second. */
	timeVals[0] = uptime / 86400;
        uptime -= timeVals[0] * 86400;
        timeVals[1] = uptime / 3600;
        uptime -= timeVals[1] * 3600;
        timeVals[2] = uptime / 60;
        uptime -= timeVals[2] * 60;
        timeVals[3] = uptime;

	uptimeDataStringLength = strlen(uptimeTag);
	for (short i = 0; i < 4; ++i)
		uptimeDataStringLength += NUM_LENGTH(timeVals[i]) + 1 +
					  strlen(timeNames[i]);
	uptimeDataStringLength += 7; /* spaces and commas */

	if ((u = calloc(1, uptimeDataStringLength)) == nullptr)
		err(1, "calloc");

	snprintf(u, uptimeDataStringLength,
			"%s%llu %s, %llu %s, %llu %s, %llu %s", uptimeTag,
			timeVals[0], timeNames[0],
			timeVals[1], timeNames[1],
			timeVals[2], timeNames[2],
			timeVals[3], timeNames[3]);
	*uptimeDataString = u;
}

void
getMemoryData(char **memoryDataString)
{
	int mib[2] = { 0 };
	struct uvmexp vm = { 0 };
	long pageSize = 0, physMem = 0;
	long usedMemoryMiB = 0, totalMemoryMiB = 0;
	short memoryDataLength = 0;
	const char memoryTag[] = "Memory:\t";
	char *m = nullptr;

	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	if (sysctl(mib, 2, &pageSize, (long[]){ 8 }, nullptr, 0) == -1)
		err(1, "sysctl");

	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP;
	if (sysctl(mib, 2, &vm, (long[]){sizeof(struct uvmexp)}, nullptr, 0) ==
	    -1)
		err(1, "sysctl");

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;
	if (sysctl(mib, 2, &physMem, (long[]){ 8 }, nullptr, 0) == -1)
		err(1, "sysctl");

	if (ckd_add(&usedMemoryMiB, vm.active, vm.swpginuse) ||
	    ckd_mul(&usedMemoryMiB, usedMemoryMiB, pageSize))
		errx(1, "ckd_add: integer overflow");

	usedMemoryMiB /= 1048576;
	totalMemoryMiB = physMem / 1048576;

	memoryDataLength = strlen(memoryTag) + NUM_LENGTH(usedMemoryMiB) +
				NUM_LENGTH(totalMemoryMiB) + 12;

	if ((m = calloc(1, memoryDataLength)) == nullptr)
		err(1, "calloc");

	snprintf(m, memoryDataLength, "%s%ld MiB / %ld MiB", memoryTag,
			usedMemoryMiB, totalMemoryMiB);
	*memoryDataString = m;
}

int
main(void)
{
	char *userData = nullptr, *systemData = nullptr, *hostData = nullptr,
		*uptimeData = nullptr, *memoryData = nullptr;

	getHostData(&hostData);

	if (pledge("stdio rpath vminfo", nullptr) == -1)
		err(1, "pledge");

	getMemoryData(&memoryData);

	if (pledge("stdio rpath", nullptr) == -1)
		err(1, "pledge");

	getUserData(&userData);

	if (pledge("stdio", nullptr) == -1)
		err(1, "pledge");

	getSystemData(&systemData);
	getUptimeData(&uptimeData);

	/* -----BEGIN FISH----- */

	  puts("      _____");
	printf("    \\-     -/\t\t%s\n", userData);
	printf(" \\_/         \\\t\t%s\n", systemData);
	printf(" |        O O |\t\t%s\n", hostData);
	printf(" |_  <   )  3 )\t\t%s\n", uptimeData);
	printf(" / \\         /\t\t%s\n", memoryData);
	  puts("    /-_____-\\\n");

	/* -----END FISH----- */

	/* There's no point in freeing anything. */

	return 0;
}
