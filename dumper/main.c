#include <stdio.h>
#include <stdlib.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <hw/csr.h>
#include <hw/flags.h>
#include <hw/mem.h>

#include <net/microudp.h>
#include <net/tftp.h>

static int d0;

#define LOCALIP1 192
#define LOCALIP2 168
#define LOCALIP3 0
#define LOCALIP4 42
#define REMOTEIP1 192
#define REMOTEIP2 168
#define REMOTEIP3 0
#define REMOTEIP4 14

static void print_status(void)
{
	printf("Ph: %4d\n", d0);
}

static void capture_raw(void)
{
	unsigned char *macadr = (unsigned char *)FLASH_OFFSET_MAC_ADDRESS;
	static char capture_buf[2*1024*1024] __attribute__((aligned(16)));
	unsigned int ip;
	int i;

	for(i=0;i<sizeof(capture_buf);i++)
		capture_buf[i] = 0xde;

	dvisampler0_dma_base_write((unsigned int)capture_buf);
	dvisampler0_dma_length_write(sizeof(capture_buf));
	dvisampler0_dma_shoot_write(1);

	printf("waiting for DMA...");
	while(dvisampler0_dma_busy_read());
	printf("done\n");
	
	printf("TFTP transfer...\n");
	ip = IPTOINT(REMOTEIP1, REMOTEIP2, REMOTEIP3, REMOTEIP4);
	microudp_start(macadr, IPTOINT(LOCALIP1, LOCALIP2, LOCALIP3, LOCALIP4));
	tftp_put(ip, "raw_dvi", capture_buf, sizeof(capture_buf));
	printf("done\n");
}

static void calibrate_delays(void)
{
	dvisampler0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_CAL);
	while(dvisampler0_data0_cap_dly_busy_read());
	dvisampler0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_RST);
	dvisampler0_data0_cap_phase_reset_write(1);
	d0 = 0;
	printf("Delays calibrated\n");
}

static void adjust_phase(void)
{
	switch(dvisampler0_data0_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			dvisampler0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_DEC);
			d0--;
			dvisampler0_data0_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			dvisampler0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_INC);
			d0++;
			dvisampler0_data0_cap_phase_reset_write(1);
			break;
	}
}

static int init_phase(void)
{
	int od0; 
	int i, j;

	for(i=0;i<100;i++) {
		od0 = d0;
		for(j=0;j<1000;j++)
			adjust_phase();
		if(abs(d0 - od0) < 4)
			return 1;
	}
	return 0;
}

static void dvidump(void)
{
	unsigned int counter;

	while(1) {
		printf("waiting for PLL lock...\n");
		while(!dvisampler0_clocking_locked_read());
		printf("PLL locked\n");
		calibrate_delays();
		if(init_phase())
			printf("Phase init OK\n");
		else
			printf("Phase did not settle\n");
		print_status();

		counter = 0;
		while(dvisampler0_clocking_locked_read()) {
			counter++;
			if(counter == 2000000) {
				print_status();
				adjust_phase();
				counter = 0;
			}
			if(readchar_nonblock() && (readchar() == 'c'))
				capture_raw();
		}
		printf("PLL unlocked\n");
	}
}

int main(void)
{
	irq_setmask(0);
	irq_setie(1);
	uart_init();

	puts("Raw DVI dump software built "__DATE__" "__TIME__"\n");
	
	dvidump();
	
	return 0;
}
