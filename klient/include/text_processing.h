#ifndef TEXT_PROCESSING_H
#define TEXT_PROCESSING_H

#include <stdlib.h>

#include "../include/window.h"
#include "../include/defines.h"

void key_callback(gui_t *g, core_t *c, char *buf);
void parse_cmd(gui_t *g, core_t *c, char *cmd);

#endif // TEXT_PROCESSING_H
