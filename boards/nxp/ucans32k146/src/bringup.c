/****************************************************************************
 * boards/arm/s32k1xx/ucans32k146/src/s32k1xx_bringup.c
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <sys/mount.h>
#include <syslog.h>

#ifdef CONFIG_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

#ifdef CONFIG_I2C_DRIVER
#  include "s32k1xx_pin.h"
#  include <nuttx/i2c/i2c_master.h>
#  include "s32k1xx_lpi2c.h"
#endif

#ifdef CONFIG_S32K1XX_EEEPROM
#  include "s32k1xx_eeeprom.h"
#endif

#include "board_config.h"

#include <stdio.h>
#include <nuttx/spi/spi.h>
#include <nuttx/leds/apa102.h>

int board_apa102_initialize(int devno, int spino);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: s32k1xx_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=n && CONFIG_BOARDCTL=y :
 *     Called from the NSH library
 *
 ****************************************************************************/


int board_apa102_initialize(int devno, int spino)
{
  struct spi_dev_s *spi;
  char devpath[13];
  int ret;

  spi = px4_spibus_initialize(spino);
  if (spi == NULL)
    {
      return -ENODEV;
    }

  /* Register the APA102 Driver at the specified location. */

  snprintf(devpath, 13, "/dev/leddrv%d", devno);
  ret = apa102_register(devpath, spi);
  if (ret < 0)
    {
      lederr("ERROR: apa102_register(%s) failed: %d\n",
             devpath, ret);
      return ret;
    }

  return OK;
}

int s32k1xx_bringup(void)
{
	int ret = OK;

#ifdef CONFIG_LEDS_APA102
	/* Configure and initialize the APA102 LED Strip. */

	ret = board_apa102_initialize(0, 1);
	if (ret < 0) {
		syslog(LOG_ERR, "ERROR: board_apa102_initialize() failed: %d\n", ret);
	}
#endif

#ifdef CONFIG_BUTTONS
	/* Register the BUTTON driver */

	ret = btn_lower_initialize("/dev/buttons");

	if (ret < 0) {
		syslog(LOG_ERR, "ERROR: btn_lower_initialize() failed: %d\n", ret);
	}

#endif

#ifdef CONFIG_USERLED
	/* Register the LED driver */

	ret = userled_lower_initialize("/dev/userleds");

	if (ret < 0) {
		syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
	}

#endif

#ifdef CONFIG_FS_PROCFS
	/* Mount the procfs file system */

	ret = mount(NULL, "/proc", "procfs", 0, NULL);

	if (ret < 0) {
		syslog(LOG_ERR, "ERROR: Failed to mount procfs at /proc: %d\n", ret);
	}

#endif

#ifdef CONFIG_S32K1XX_EEEPROM
	/* Register EEEPROM block device */

	s32k1xx_eeeprom_register(0, 4096);
#endif

#ifdef CONFIG_S32K1XX_LPSPI0
	/* Configure SPI chip selects if 1) SPI is not disabled, and 2) the weak
	 * function s32k1xx_spidev_initialize() has been brought into the link.
	 */

	s32k1xx_spidev_initialize();
	s32k1xx_spi_bus_initialize();
#endif

#if defined(CONFIG_S32K1XX_LPI2C0)
#if defined(CONFIG_I2C_DRIVER)
	FAR struct i2c_master_s *i2c;
	i2c = s32k1xx_i2cbus_initialize(0);

	if (i2c == NULL) {
		serr("ERROR: Failed to get I2C0 interface\n");

	} else {
		ret = i2c_register(i2c, 0);

		if (ret < 0) {
			serr("ERROR: Failed to register I2C0 driver: %d\n", ret);
			s32k1xx_i2cbus_uninitialize(i2c);
		}
	}

#endif
#endif

#ifdef CONFIG_S32K1XX_FLEXCAN
	s32k1xx_pinconfig(BOARD_REVISION_DETECT_PIN);

	if (s32k1xx_gpioread(BOARD_REVISION_DETECT_PIN)) {
		/* STB high -> active CAN phy */
		s32k1xx_pinconfig(PIN_CAN0_STB  | GPIO_OUTPUT_ONE);
		s32k1xx_pinconfig(PIN_CAN1_STB  | GPIO_OUTPUT_ONE);

	} else {
		/* STB low -> active CAN phy */
		s32k1xx_pinconfig(PIN_CAN0_STB  | GPIO_OUTPUT_ZERO);
		s32k1xx_pinconfig(PIN_CAN1_STB  | GPIO_OUTPUT_ZERO);
	}

#endif

	return ret;
}
