#include <type.h>
#include <assert.h>

#include <kern/syscall.h>
#include <kern/time.h>

static size_t timecounter;

/*
 * 时间戳加一
 */
void
timecounter_inc(void)
{
	timecounter++;
}

/*
 * 获取内核当前的时间戳
 */
size_t
kern_get_ticks(void)
{
	return timecounter;
}
//添加kern_delay_ticks
size_t
kern_delay_ticks(u32 ticks)
{
	u32 t_ticks = ticks;
	//记录初始时间
	size_t start = timecounter;
	// kprintf("s%d ", start);

	assert(timecounter != 0);
	//timecounter可能等于0
	while(1)
	{
		if((timecounter - start) >= t_ticks )//当延迟了目标的时间片数，退�?
			break;
	}
	return 0;
}

ssize_t
do_get_ticks(void)
{
	return (ssize_t)kern_get_ticks();
}

//添加do_delay_ticks
ssize_t
do_delay_ticks(u32 ticks)
{
	return (ssize_t)kern_delay_ticks(ticks);
}
