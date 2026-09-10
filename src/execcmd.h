#include "readcmd.h"
#include "csapp.h"
#include "jobs.h"

extern pid_t fg_pids[10];
extern int num_fg_procs;
extern char* cmd_precedent;

void exec_cmd_int(struct cmdline *l);

void exec_cmd(struct cmdline *l, int num_cmds);