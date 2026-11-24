#ifndef ZZT_FRAME_SCHEDULER_H
#define ZZT_FRAME_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

#include "zoo.h"

typedef struct {
	double pit_remainder_ms;
	int64_t previous_frame_nanos;
	double next_game_ms;
} zzt_frame_scheduler;

static inline void zzt_frame_scheduler_reset(zzt_frame_scheduler *scheduler) {
	scheduler->pit_remainder_ms = 0;
	scheduler->previous_frame_nanos = 0;
	scheduler->next_game_ms = 0;
}

/* Apply the Android Choreographer timestamp to the same PIT accumulator and
   game deadline used by the JNI frontend. Return each PIT tick to be delivered
   to libzoo by the caller. Gaps over one second deliberately rebase the frame
   deadline without adding the suspended interval to game time. */
static inline int zzt_frame_scheduler_begin_frame(zzt_frame_scheduler *scheduler,
	int64_t frame_time_nanos, double *now_ms_out) {
	double now_ms = (double) frame_time_nanos / 1000000.0;
	int pit_ticks = 0;
	*now_ms_out = now_ms;
	if (scheduler->previous_frame_nanos == 0
		|| frame_time_nanos < scheduler->previous_frame_nanos
		|| frame_time_nanos - scheduler->previous_frame_nanos > 1000000000LL) {
		scheduler->previous_frame_nanos = frame_time_nanos;
		scheduler->next_game_ms = now_ms;
	} else {
		scheduler->pit_remainder_ms +=
			(double) (frame_time_nanos - scheduler->previous_frame_nanos) / 1000000.0;
		scheduler->previous_frame_nanos = frame_time_nanos;
	}

	while (scheduler->pit_remainder_ms >= ZOO_PIT_TICK_MS) {
		scheduler->pit_remainder_ms -= ZOO_PIT_TICK_MS;
		pit_ticks++;
	}
	return pit_ticks;
}

static inline bool zzt_frame_scheduler_game_due(const zzt_frame_scheduler *scheduler,
	double now_ms) {
	return now_ms >= scheduler->next_game_ms;
}

static inline void zzt_frame_scheduler_modal_prompt(zzt_frame_scheduler *scheduler,
	double now_ms) {
	scheduler->next_game_ms = now_ms;
}

static inline void zzt_frame_scheduler_tick_finished(zzt_frame_scheduler *scheduler,
	double now_ms, zoo_tick_retval result) {
	if (result == ERROR) return;
	if (result == RETURN_NEXT_FRAME)
		scheduler->next_game_ms = now_ms + 16;
	else if (result == RETURN_NEXT_CYCLE)
		scheduler->next_game_ms += ZOO_PIT_TICK_MS;
	else
		scheduler->next_game_ms = now_ms + 16;
}

#endif
