#ifndef TEXT_PROCESSING_H
#define TEXT_PROCESSING_H

#include <stdlib.h>

#include "communication.h"
#include "window.h"
#include "defines.h"

int stringtoint(const char *c);
void key_callback(gui_t *g, core_t *c, char *buf, int outfd);
void out_callback(int fd);
void in_callback(gui_t *g, core_t *c, int fd);
void parse_cmd(gui_t *g, core_t *c, char *cmd, int outfd);

#endif // TEXT_PROCESSING_H
