/* signals.c — Week 6: Signals & Async Control implementation */
#include "signals.h"

#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t sigchld_pending = 0;

static void sigchld_handler(int signo) {
    (void)signo;
    sigchld_pending = 1;
}

static void sigint_sigtstp_handler(int signo) {
    (void)signo;

    const char nl[] = "\n";
    write(STDOUT_FILENO, nl, sizeof(nl) - 1);
}

void shell_install_signal_handlers(void) {
    struct sigaction sa_ignore;

    sa_ignore.sa_handler = sigint_sigtstp_handler;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa_ignore, NULL);
    sigaction(SIGTSTP, &sa_ignore, NULL);

    struct sigaction sa_chld;

    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    sigaction(SIGCHLD, &sa_chld, NULL);
}

void child_reset_signal_handlers(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
}

void sigchld_block(void) {
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void sigchld_unblock(void) {
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}
