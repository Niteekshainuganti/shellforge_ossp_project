/* signals.h — Week 6: Signals & Async Control */
#ifndef SHELLFORGE_SIGNALS_H
#define SHELLFORGE_SIGNALS_H

void shell_install_signal_handlers(void);

void child_reset_signal_handlers(void);

void sigchld_block(void);
void sigchld_unblock(void);

#endif /* SHELLFORGE_SIGNALS_H */
