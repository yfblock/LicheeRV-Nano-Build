/*
 * 用户态最小测试：检查 /dev/cvi-rtos-cmdqu 是否可用，并发送一条单向命令到 RTOS。
 * 与 osdrv/interdrv/v2/rtos_cmdqu/rtos_cmdqu.h 定义保持一致。
 *
 * 编译（宿主机 x86 仅做语法检查）:
 *   gcc -o rtos_cmdqu_test rtos_cmdqu_test.c
 * 交叉编译（RISC-V，推送到板子运行）:
 *   riscv64-linux-gnu-gcc -static -o rtos_cmdqu_test rtos_cmdqu_test.c
 *
 * 设备上运行（需 root）:
 *   ./rtos_cmdqu_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define RTOS_CMDQU_DEV "/dev/cvi-rtos-cmdqu"

/* 与内核 rtos_cmdqu.h 对齐 */
#define NR_SYSTEM_CMD  20
enum {
	CMDQU_SEND = 1,
	CMDQU_SEND_WAIT,
};
#define RTOS_CMDQU_SEND      _IOW('r', CMDQU_SEND, unsigned long)
#define RTOS_CMDQU_SEND_WAIT _IOW('r', CMDQU_SEND_WAIT, unsigned long)

enum IP_TYPE { IP_SYSTEM = 6, IP_LIMIT };
enum SYS_CMD_ID {
	SYS_CMD_INFO_TRANS = 0x50,
	SYS_CMD_INFO_LINUX_INIT_DONE,
	SYS_CMD_INFO_RTOS_INIT_DONE,
	SYS_CMD_INFO_STOP_ISR,
	SYS_CMD_INFO_STOP_ISR_DONE,
	SYS_CMD_INFO_LINUX,   /* 0x55, 单向通知，适合做存活探测 */
	SYS_CMD_INFO_RTOS,
	SYS_CMD_INFO_LIMIT,
};

typedef struct __attribute__((packed)) __attribute__((aligned(8))) {
	unsigned char ip_id;
	unsigned char cmd_id : 7;
	unsigned char block : 1;
	unsigned short resv;
	unsigned int  param_ptr;
} cmdqu_t;

int main(void)
{
	int fd;
	cmdqu_t cmdq;
	int ret;

	printf("RTOS cmdqu test: open %s ...\n", RTOS_CMDQU_DEV);
	fd = open(RTOS_CMDQU_DEV, O_RDWR);
	if (fd < 0) {
		printf("FAIL: open: %s\n", strerror(errno));
		return 1;
	}
	printf("OK: device opened.\n");

	memset(&cmdq, 0, sizeof(cmdq));
	cmdq.ip_id  = IP_SYSTEM;
	cmdq.cmd_id = SYS_CMD_INFO_LINUX;
	cmdq.block  = 0;
	cmdq.param_ptr = 0;

	ret = ioctl(fd, RTOS_CMDQU_SEND, &cmdq);
	close(fd);
	if (ret != 0) {
		printf("FAIL: ioctl(RTOS_CMDQU_SEND): %d (%s)\n", ret, strerror(errno));
		return 1;
	}
	printf("OK: ioctl RTOS_CMDQU_SEND succeeded (RTOS path is up).\n");
	return 0;
}
