#include "../include/communication.h"
#include <mqueue.h>
#include <sys/msg.h>
#include <signal.h>
#include <stdio.h>

void quit(resources* res) {
	endwin();
	int ret = -1, c1 = res->child_id1, c2 = res->child_id2;
	printf("[%d] Caught SIGINT: quitting...\n", getpid());
	ret = kill(res->child_id1, SIGINT);
	printf("[%d] %d killed, code: %d\n", getpid(), c1, ret);
	ret = kill(res->child_id2, SIGINT);
	printf("[%d] %d killed, code: %d\n", getpid(), c2, ret);
	ret = mq_close(res->int_in_fd);
	printf("[%d] Internal queue (in) closed, code: %d\n", getpid(), ret);
	ret = mq_close(res->int_out_fd);
	printf("[%d] Internal queue (out) closed, code: %d\n", getpid(), ret);
	ret = mq_unlink("/internal_in");
	printf("[%d] Internal (in) unlinked, code: %d\n", getpid(), ret);
	ret = mq_unlink("/internal_out");
	printf("[%d] Internal (out) unlinked, code: %d\n", getpid(), ret);
	int mid = msgget(res->queue_key, 0777);
	if(mid != -1) {
		ret = msgctl(mid, IPC_RMID, NULL);
		printf("[%d] External queue removed, code: %d\n", getpid(), ret);
	}
	exit(0);
}
