#include "../include/core.h"
#include <string.h>
#include <stdio.h>

void add_user_to_list(core_t *c, const char *name) {
	snprintf(c->userlist[c->cur_user_pos], MAX_USER_NAME_LENGTH, "%s", name);
	c->userlist[c->cur_user_pos][strlen(name)+1] = '\0';
	c->cur_user_pos++;
}
