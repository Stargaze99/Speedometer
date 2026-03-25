/*
 * sys_time.h
 *
 * Created: 2026-03-25 16:12:10
 *  Author: Admin
 */ 


#ifndef SYS_TIME_H_
#define SYS_TIME_H_

#include <stdint.h>
#include <stdbool.h>

/* Global millisecond counter */
extern volatile uint32_t sys_ms;

/* Initialize Timer0 to generate 1 ms tick */
void sys_time_init(void);

/* Check if duration_ms has passed since *last_time */
bool elapsed_ms(uint32_t *last_time, uint32_t duration_ms);



#endif /* SYS_TIME_H_ */