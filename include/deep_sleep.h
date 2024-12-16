#ifndef deep_sleep_h
#define deep_sleep_h

void enable_timer_wake(int wakeup_time_sec);
void enable_rtc_io_wake(int GPIO_PORT, int level);
void deep_sleep_task(void *pvParameters);

#endif // !deep_sleep_h
