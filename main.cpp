
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <vector>

#define PRINT_ERROR(fmt, ...) fprintf(stderr, "%s:%d:%s " fmt, __FILE__, __LINE__, __func__, ## __VA_ARGS__);

int read_process_memory(
	pid_t pid,
	off_t offset,
	size_t size,
	void* buff)
{
	bool success = false;
	int ret;
	
	char mem_file_name[256];
	sprintf(mem_file_name, "/proc/%d/mem", pid);
	int mem_fd = open(mem_file_name, O_RDONLY);
	if (mem_fd == -1) {
		PRINT_ERROR("open err %s\n", strerror(mem_fd));
		return -1;
	}
	ret = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
	if (ret == -1) {
		PRINT_ERROR("ptrace attach err %s\n", strerror(errno));
		return -1;
	}
	ret = waitpid(pid, NULL, 0);
	if (ret == -1) {
		PRINT_ERROR("waitpid err %s\n", strerror(errno));
		goto Label_Detach;
	}
	ret = lseek(mem_fd, offset, SEEK_SET);
	if (ret == -1) {
		PRINT_ERROR("lseek err %s\n", strerror(errno));
		goto Label_Detach;
	}
	ret = read(mem_fd, buff, size);
	if (ret == -1) {
		PRINT_ERROR("read err %s\n", strerror(errno));
		goto Label_Detach;
	}
	
	success = true;
	
Label_Detach:
	ret = ptrace(PTRACE_DETACH, pid, NULL, NULL);
	if (ret == -1) {
		PRINT_ERROR("ptrace detach err %s\n", strerror(errno));
		return -1;
	}
	return success ? 0 : -1;
}

int pgrep(
	const char* pattern,
	std::vector<unsigned int>& pids)
{
	char buff[128];
	FILE *iopipe;
	sprintf(buff, "pgrep %s", pattern);
	if ((iopipe = popen(buff, "r")) == NULL) {
		PRINT_ERROR("popen err %s\n", strerror(errno));
		return -1;
	}
	int ret;
	while (!feof(iopipe)) {
		if (fgets(buff, 128, iopipe)) {
			//fputs(buff, stdout);
			//int pid = atoi(buff); // or use atoi(psBuffer) to get numerical value
			//printf("pid %d\n", pid);
			pids.push_back(strtoul(buff, 0, 10));
		}else {
			ret = ferror(iopipe);
			if (ret != 0) {
				return -1;
			}
		}
	}
	ret = pclose(iopipe);
	if (ret == -1) {
		PRINT_ERROR("pclose err %s\n", strerror(errno));
		return -1;
	}
	if (WIFEXITED(ret)) {
		return WEXITSTATUS(ret);
	}
	if (WIFSIGNALED(ret)) {
		PRINT_ERROR("WTERMSIG : %d\n", WTERMSIG(ret));
	}
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 3) {
		PRINT_ERROR("specify pid/name addr size\n");
		return 1;
	}
	
	pid_t pid = atoi(argv[1]);
	if (pid == 0) {
		std::vector<unsigned int> pids;
		const char* pattern = argv[1];
		pgrep(pattern, pids);
		if (pids.empty()) {
			PRINT_ERROR("specified name process not found\n");
			return 1;
		}
		pid = pids[0];
	}
	off_t offset = strtoul(argv[2], NULL, 16);
	size_t size = strtoul(argv[3], NULL, 10);
	std::vector<char> buff(size);
	
	int ret = read_process_memory(pid, offset, size, &buff[0]);
	if (ret == -1) {
		PRINT_ERROR("read_process_memory err\n");
		return 1;
	}
	
	fwrite(&buff[0], 1, size, stdout);
	return 0;
}

