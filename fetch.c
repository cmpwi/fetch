
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

#define NEWLINE putchar('\n')

void
printUserData(void)
{
	struct passwd *pw = nullptr;
	char hostname[HOST_NAME_MAX + 1] = { 0 };

	if ((pw = getpwuid(getuid())) == nullptr)
		errx(1, "getpwnam");

	if (gethostname(hostname, HOST_NAME_MAX + 1) == -1)
		err(1, nullptr);

	fputs(pw->pw_name, stdout); putchar('@');
	fputs(hostname, stdout);
}

void
printSystemData(void)
{
	struct utsname u;

	if (uname(&u) == -1)
		err(1, nullptr);

	fputs("System:\t", stdout);
	fputs(u.sysname, stdout); putchar(' ');
	fputs(u.release, stdout); putchar(' ');
	fputs(u.machine, stdout);
}

void
printHostData(void)
{
	int	mib[2] = { CTL_HW, HW_VENDOR };
	char	*hwVendor = nullptr, *hwProduct = nullptr;
	long	hwVendorLength = 0, hwProductLength = 0;

	if (sysctl(mib, 2, nullptr, &hwVendorLength, nullptr, 0) == -1)
		err(1, "sysctl");

	if ((hwVendor = malloc(sizeof(char) * hwVendorLength)) == nullptr)
		err(1, nullptr);

	if (sysctl(mib, 2, hwVendor, &hwVendorLength, nullptr, 0) == -1)
		err(1, "sysctl");

	mib[1] = HW_PRODUCT;
	
	if (sysctl(mib, 2, nullptr, &hwProductLength, nullptr, 0) == -1)
		err(1, nullptr);

	if ((hwProduct = malloc(sizeof(char) * hwProductLength)) == nullptr)
		err(1, nullptr);

	if (sysctl(mib, 2, hwProduct, &hwProductLength, nullptr, 0) == -1)
		err(1, nullptr);

	fputs("Host:\t", stdout);
	fputs(hwVendor, stdout); putchar(' ');
	fputs(hwProduct, stdout);

	if (hwVendor != nullptr)
		free(hwVendor);

	if (hwProduct != nullptr)
		free(hwProduct);
}

void
printUptimeData(void)
{
	struct timespec	boottime;
	time_t		uptime;
	int		timeVals[4] = { 0 };
        const char	*timeNames[] = {"days", "hours", "minutes", "seconds"};
	bool didPrint = false;

	if (clock_gettime(CLOCK_BOOTTIME, &boottime) == -1)
		err(1, nullptr);
	else
		uptime = boottime.tv_sec;

	/* Day, hour, minute, second. */
	timeVals[0] = uptime / 86400;
        uptime -= timeVals[0] * 86400;
        timeVals[1] = uptime / 3600;
        uptime -= timeVals[1] * 3600;
        timeVals[2] = uptime / 60;
        uptime -= timeVals[2] * 60;
        timeVals[3] = (int)uptime;

	fputs("Uptime:\t", stdout);

	for (int i = 0; i < 4; ++i)
		if (timeVals[i] != 0) {
			printf(didPrint ? ", %d %.*s" : "%d %.*s",
				timeVals[i],
					timeVals[i] == 1 ?
					(int)strnlen(timeNames[i], 7) - 1 :
					(int)strnlen(timeNames[i], 7),
				timeNames[i]);
			didPrint = true;
		}
}

void
printMemoryData(void)
{
	struct uvmexp	vm = { 0 };
	int		mib[2] = { CTL_HW, HW_PAGESIZE };	
	long		pageSize = 0,
			physMem = 0;
	long		vmSize = sizeof(struct uvmexp),
			pageSizeSize = sizeof(pageSize),
			physMemSize = sizeof(physMem);

	if (sysctl(mib, 2, &pageSize, &pageSizeSize, nullptr, 0) == -1)
		err(1, "sysctl");

	mib[0] = CTL_VM;
	mib[1] = VM_UVMEXP;

	if (sysctl(mib, 2, &vm, &vmSize, nullptr, 0) == -1)
		err(1, "sysctl");

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;

	if (sysctl(mib, 2, &physMem, &physMemSize, nullptr, 0) == -1)
		err(1, "sysctl");

	printf("Memory:\t%ld MiB / %ld MiB",
			(vm.active + vm.swpginuse) * pageSize / 1048576,
							physMem / 1048576);
}

int
main(void)
{
	/* -----BEGIN FISH----- */

	 puts("      _____");
	fputs("    \\-     -/\t\t", stdout);	printUserData();	NEWLINE;
	fputs(" \\_/         \\\t\t", stdout);	printSystemData();	NEWLINE;
	fputs(" |        O O |\t\t", stdout);	printHostData();	NEWLINE;
	fputs(" |_  <   )  3 )\t\t", stdout);	printUptimeData();	NEWLINE;
	fputs(" / \\         /\t\t", stdout);	printMemoryData();	NEWLINE;
	fputs("    /-_____-\\\n\n", stdout);

	/* -----END FISH----- */
	
	return 0;
}
