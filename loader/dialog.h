#ifndef __DIALOG_H__
#define __DIALOG_H__

int init_msg_dialog(const char *msg);
int get_msg_dialog_result(void);

void fatal_error(const char *fmt, ...) __attribute__((noreturn));

#endif