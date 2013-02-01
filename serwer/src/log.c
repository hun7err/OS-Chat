#include "../include/log.h"

int logfile(const char *message, int *sem_key) {
	int semid = 0;
	semid = semget(*sem_key, 3, 0777);
	if(semid == -1) {
		perror("Blad pobierania id semaforow");
		return -1;
	}
	p(semid, LOG);
	int fd = open(LOGFILE, O_RDWR | O_CREAT, 0777);
	if(fd == -1) return -1;
	if(lseek(fd,0,SEEK_END) == -1) return -1;
	if(write(fd, message, strlen(message)) == -1) return -1;
	if(close(fd) == -1) return -1;
	v(semid, LOG);
	return 0;
}
