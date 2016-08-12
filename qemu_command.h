#ifndef QEMU_CMD_H
#define QEMU_CMD_H

#define CMD_TRACE_OFF 0xAA
#define CMD_TRACE_ON  0xBB
#define CMD_TRACE_CLEAR 0xCC
#define CMD_SERVER_CLOSE 0xDEADDEAD

struct command
{
        int cmdNo;
};

typedef struct command tcmd;

#define MAX_PATH 256
#define _STR(x) #x
#define STR(x) _STR(x)
#define PATH_MAX 4096
#define PERF_DEBUGFS_ENVIRONMENT "PERF_DEBUGFS_DIR"
#define DEBUGFS_MAGIC          0x64626720

const char *find_debugfs(void);
void processCmd(int cmd);

#endif
