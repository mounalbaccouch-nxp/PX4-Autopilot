/**
 * @file latch_relay_control.c
 * Minimal application example for PX4 autopilot
 *
 * @author Example User <mail@example.com>
 */

#include <px4_platform_common/log.h>
#include <drivers/drv_hrt.h>
#include <nuttx/arch.h>
#include "s32k1xx_pin.h"
#include "board_config.h"
#define TEST_TIME_US            10000000 /* usec */
#define DEFAULT_PULSE_WIDTH     100      /* msec */
#define DEFAULT_PULSE_STEP      25       /* msec */
#define MAX_PULSE_WIDTH         300      /* msec */
#define MIN_PULSE_WIDTH         1        /* msec */
#define MIN_RETRY_TIME          100      /* msec */
#define SEC_TO_USEC             1000000

__EXPORT int latch_relay_control_main(int argc, char *argv[]);

void toggle_latch(int pulse_width, int max_pulse_width)
{
        s32k1xx_gpiowrite(RELAY_CONTROL_PIN, 1);
        up_mdelay(pulse_width);
        s32k1xx_gpiowrite(RELAY_CONTROL_PIN, 0);
	up_mdelay(max_pulse_width);
}

int latch_relay_control_main(int argc, char *argv[])
{
	hrt_abstime stime;
        long long unsigned int test_time = TEST_TIME_US;
        int pulse_width = DEFAULT_PULSE_WIDTH;
        int max_pulse_width = MAX_PULSE_WIDTH;
        int pulse_step = DEFAULT_PULSE_STEP;
        int retry_time = MIN_RETRY_TIME;

        /* PIN Initialization */
	s32k1xx_pinconfig(LATCH_STATUS_PIN);
	s32k1xx_pinconfig(RELAY_CONTROL_PIN);

        PX4_INFO("Latch Control Test!\n");
        /* Commandline Input parsing */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-tt") == 0 || strcmp(argv[i], "--test_time") == 0) {
                        if((i+1) < argc) {
                                test_time = (atoi(argv[i+1]) * SEC_TO_USEC);
                        }
		}

		if (strcmp(argv[i], "-pw") == 0 || strcmp(argv[i], "--pulse_width") == 0) {
                        if((i+1) < argc) {
                                if(atoi(argv[i+1]) < max_pulse_width)
                                        pulse_width = atoi(argv[i+1]);
                        }
                }
		
                if (strcmp(argv[i], "-mpw") == 0 || strcmp(argv[i], "--max_pulse_width") == 0) {
                        if((i+1) < argc) {
                                if(atoi(argv[i+1]) < MAX_PULSE_WIDTH)
                                        max_pulse_width = atoi(argv[i+1]);
                        }
                }

                if (strcmp(argv[i], "-ps") == 0 || strcmp(argv[i], "--pulse_step") == 0) {
                        if((i+1) < argc) {
                                if((atoi(argv[i+1]) < (MAX_PULSE_WIDTH - pulse_width)) && (atoi(argv[i+1]) > MIN_PULSE_WIDTH))
                                        pulse_step = atoi(argv[i+1]);
                        }
                }
                
                if (strcmp(argv[i], "-ri") == 0 || strcmp(argv[i], "--retry_interval") == 0) {
                        if((i+1) < argc) {
                                if(atoi(argv[i+1]) > MIN_RETRY_TIME)
                                        retry_time = atoi(argv[i+1]);
                        }
                }
	}

        /* Latch control logic */
	stime = hrt_absolute_time();

	while (hrt_absolute_time() - stime < test_time) {

                if (s32k1xx_gpioread(LATCH_STATUS_PIN) == 0)
                {
                        toggle_latch(pulse_width, max_pulse_width);

                        while ((s32k1xx_gpioread(LATCH_STATUS_PIN) == 0) && (pulse_width < max_pulse_width))
                        {
                                up_mdelay(retry_time);
                                pulse_width += pulse_step;
                                toggle_latch(pulse_width, max_pulse_width);
                        }
                }
        }

        return OK;
}
