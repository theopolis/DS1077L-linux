#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

/* i2c-dev.h from lm-sensors */
#include "i2c-dev.h"

/* address of DS1077LZ */
#define I2C_ADDR 	0x58

#define DIV 		0x01
#define MUX 		0x02
#define BUS 		0x0D
#define E2 			0x3F

struct ds1077_data {
	__u8 addr;
	__u8 cmd;
	__u8 msb;
	__u8 lsb;
};

struct msg_data {
	__u16 data;
};

/* do not use */
int custom_write(int fd, __u32 p, __u32 div)
{
	struct ds1077_data msg;

	msg.addr = I2C_ADDR;

	/* swap half-nibbles */
	/* p = ((p & 0xC) >> 2) + ((p & 0x3) << 2); */
	/* need to lose one bit for MSB */
	msg.msb = p >> 1;
	/* turn sel0 on */
	msg.msb |= 0x10;
	/* first 6 bits of LSB are unused, 7th is div1 enable */
	msg.lsb = (p & 0x1) << 7;

	msg.cmd = MUX;
	printf("Setting MUX=0x%x, 0x%x\n", msg.msb, msg.lsb); /*div=0x%x, %x; freq=B/(p*div)*/
	if ( ioctl(fd, I2C_SMBUS, &msg) < 0 ) {
		printf("Error: Failed to write MUX\n");
		return 2;
	}

	msg.msb = div >> 2;
	msg.lsb = (div & 0x3) << 6;

	msg.cmd = DIV;
	printf("Setting DIV=0x%x, 0x%x\n", msg.msb, msg.lsb); /*div=0x%x, %x; freq=B/(p*div)*/
	if ( ioctl(fd, I2C_SMBUS, &msg) < 0 ) {
		printf("Error: Failed to write MUX\n");
		return 2;
	}

	return 0;
}

int smbus_write(int fd, __u32 p, __u32 div)
{
	struct msg_data msg;

	/* turn sel0, en0 on */
	msg.data = p;
	msg.data |= 0x18;
	/* first 6 bits of LSB are unused, 7th is div1 enable */
	msg.data = msg.data << 7;

	/* swap */
	msg.data = ((msg.data & 0xff) << 8) + (msg.data >> 8);

	printf("Setting MUX=0x%x, 0x%x\n", msg.data >> 8, msg.data & 0xff);
	/*div=0x%x, %x; freq=B/(p*div)*/
	if ( i2c_smbus_write_word_data(fd, MUX, msg.data) < 0 ) {
		printf("Error: Failed to write MUX\n");
		return 2;
	}

	sleep(1);

	msg.data = div << 6;

	/* swap */
	msg.data = ((msg.data & 0xff) << 8) + (msg.data >> 8);

	printf("Setting DIV=0x%x, 0x%x\n", msg.data >> 8, msg.data & 0xff);
	/*div=0x%x, %x; freq=B/(p*div)*/
	if ( i2c_smbus_write_word_data(fd, DIV, msg.data) < 0 ) {
		printf("Error: Failed to write DIV\n");
		return 2;
	}

	return 0;
}

void smbus_read(int fd)
{
	sleep(1);
	printf("Reading MUX=0x%x\n", i2c_smbus_read_word_data(fd, MUX));
	sleep(1);
	printf("Reading DIV=0x%x\n", i2c_smbus_read_word_data(fd, DIV));
}

int main(int argc, char *argv[])
{
	int fd;

	__u32 p;
	__u32 div;

	if (argc != 3) {
		goto error;
	}

	p = atoi(argv[1]);
	if (p != 0 && p != 1 && p != 2 && p != 4 && p != 8) {
		goto error;
	}

	div = atoi(argv[2]);
	if (div < 2 || div > 1025) {
		goto error;
	}
	div -= 2;

	fd = open("/dev/i2c-1", O_RDWR);
	if (ioctl( fd, I2C_SLAVE, I2C_ADDR) < 0) {
		printf("Error: Failed to set slave address\n");
		return 2;
	}

	if (p == 0) {
		smbus_read(fd);
		return 0;
	}

	return smbus_write(fd, p, div);

error:
	printf("Usage: %s p [div]\n Where p=[0,1,2,4,8]; div=[1-1025]\n Set p=0 to read\n", argv[0]);
	return 1;
}
