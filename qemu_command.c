#include <sys/statfs.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mount.h>
#include <string.h>
#include <stdlib.h>
#include "qemu_command.h"

int debugfs_found;
static int debugfs_premounted;
const char *debugfs_known_mountpoints[] = {
        "/sys/kernel/debug/",
        "/debug/",
        0,
};

char debugfs_mountpoint[PATH_MAX + 1] = "/sys/kernel/debug";

int debugfs_valid_mountpoint(const char *debugfs)
{
        struct statfs st_fs;

        if (statfs(debugfs, &st_fs) < 0)
                return -ENOENT;
        else if (st_fs.f_type != (long) DEBUGFS_MAGIC)
                return -ENOENT;

        return 0;
}

/* find the path to the mounted debugfs */
const char *debugfs_find_mountpoint(void)
{
        const char **ptr;
        char type[100];
        FILE *fp;

        if (debugfs_found)
                return (const char *) debugfs_mountpoint;

        ptr = debugfs_known_mountpoints;
        while (*ptr) {
                if (debugfs_valid_mountpoint(*ptr) == 0) {
                        debugfs_found = 1;
                        strcpy(debugfs_mountpoint, *ptr);
                        return debugfs_mountpoint;
                }
                ptr++;
        }

        /* give up and parse /proc/mounts */
        fp = fopen("/proc/mounts", "r");
        if (fp == NULL)
                return NULL;

        while (fscanf(fp, "%*s %" STR(PATH_MAX) "s %99s %*s %*d %*d\n",
                      debugfs_mountpoint, type) == 2) {
                if (strcmp(type, "debugfs") == 0)
                        break;
        }
        fclose(fp);

        if (strcmp(type, "debugfs") != 0)
                return NULL;

        debugfs_found = 1;

        return debugfs_mountpoint;
}

/* mount the debugfs somewhere if it's not mounted */

char *debugfs_mount(const char *mountpoint)
{
        /* see if it's already mounted */
        if (debugfs_find_mountpoint()) {
                debugfs_premounted = 1;
                goto out;
        }

        /* if not mounted and no argument */
        if (mountpoint == NULL) {
                /* see if environment variable set */
                mountpoint = getenv(PERF_DEBUGFS_ENVIRONMENT);
                /* if no environment variable, use default */
                if (mountpoint == NULL)
                        mountpoint = "/sys/kernel/debug";
        }

        if (mount(NULL, mountpoint, "debugfs", 0, NULL) < 0)
                return NULL;

        /* save the mountpoint */
        debugfs_found = 1;
        strncpy(debugfs_mountpoint, mountpoint, sizeof(debugfs_mountpoint));
out:
        return debugfs_mountpoint;
}

const char *find_debugfs(void)
{
        const char *path = debugfs_mount(NULL);

        if (!path)
	{	
		printf("Debugfs Not supported\n");
        	return NULL; 
	}

        return path;
}
