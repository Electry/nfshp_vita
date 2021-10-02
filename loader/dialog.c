/*
 * dialog.c -- common dialog for error messages
 *
 * Copyright (C) 2021 fgsfds, Andy Nguyen
 *               2021 Electry <dev@electry.sk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <vitasdk.h>
#include <vita2d.h>

#include <stdio.h>
#include <stdarg.h>

int init_msg_dialog(const char *msg) {
  SceMsgDialogUserMessageParam msg_param;
  memset(&msg_param, 0, sizeof(msg_param));
  msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
  msg_param.msg = (SceChar8 *)msg;

  SceMsgDialogParam param;
  sceMsgDialogParamInit(&param);
  _sceCommonDialogSetMagicNumber(&param.commonParam);
  param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
  param.userMsgParam = &msg_param;

  return sceMsgDialogInit(&param);
}

int get_msg_dialog_result(void) {
  if (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
    return 0;
  sceMsgDialogTerm();
  return 1;
}

void fatal_error(const char *fmt, ...) {
  va_list list;
  char string[512];

  va_start(list, fmt);
  vsnprintf(string, sizeof(string), fmt, list);
  va_end(list);

  vita2d_init();
  init_msg_dialog(string);

  while (!get_msg_dialog_result()) {
    vita2d_common_dialog_update();
    vita2d_swap_buffers();
  }

  vita2d_fini();

  sceKernelExitProcess(0);
  while (1);
}
