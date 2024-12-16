#include "sdkconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"
  .bass

  .global io_number
io_number:
  .long 7

  .text
  .global entry
entry:
  move r3, io_number
  ld r3, r3, 0

#if CONFIG_IDF_TARGET_ESP32S2
    /* ESP32S2 powers down RTC periph when entering deep sleep and thus by association SENS_SAR_IO_MUX_CONF_REG */
	WRITE_RTC_FIELD(SENS_SAR_IO_MUX_CONF_REG, SENS_IOMUX_CLK_GATE_EN, 1)
#elif CONFIG_IDF_TARGET_ESP32S3
    /* ESP32S3 powers down RTC periph when entering deep sleep and thus by association SENS_SAR_PERI_CLK_GATE_CONF_REG */
    WRITE_RTC_FIELD(SENS_SAR_PERI_CLK_GATE_CONF_REG, SENS_IOMUX_CLK_EN, 1);
#endif

  /* jump to read_io_high if r0 is greater than or equal to 16 */
  move r0, r3
  jumpr read_io_high, 16, ge

  READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
  rsh r0, r0, r3
  jump read_done

read_io_high:
  READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + 16, 2)
  sub r3, r3, 16
  rsh r0, r0, r3

read_done:
  and r1, r0, r3
  jumpr wake_up, 0, EQ
  halt
