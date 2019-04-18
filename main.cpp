#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <thread>
#include <memory>
#include "singleton.h"

using namespace sox;
#define HTTP_PORT 8082
#define LOG_FILE "yy_record_daemon.log"

void sigchild_handler(int signo) {
	int stat;
	pid_t pid;
	while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
		std::cout << "child:" << pid << " exit" << std::endl;
	}
}

int main(char argc, char *argv[]) {
	
	SingleTon<MySqlConnPool<CourseRecordDB>>::getInstance()->init(10, 50);
	while(1) {
		sleep(10);
	}
	
	return 0;
}
