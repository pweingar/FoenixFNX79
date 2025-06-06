/*
 * ringbuffer.h
 *
 * Implement a FIFO ring buffer for bytes.
 *
 *  Created on: Jun 4, 2025
 *      Author: pjw
 */

#ifndef INC_RINGBUFFER_H_
#define INC_RINGBUFFER_H_

#include <stdbool.h>
#include <stdint.h>

#define MAX_RING_BUFFER_SIZE	8

typedef struct ring_buffer_s {
	uint8_t buffer[MAX_RING_BUFFER_SIZE];
	bool is_full;
	bool is_empty;
	short read_idx;
	short write_idx;
} ring_buffer_t, *ring_buffer_p;

/**
 * Initialize the ring buffer
 */
extern void rb_init(ring_buffer_p rb);

/**
 * Enqueue a byte to the ring buffer (fails silently)
 *
 * @param rb pointer to the ring buffer
 * @param data the byte to enqueue
 */
extern void rb_enqueue(ring_buffer_p rb, uint8_t data);

/**
 * Read the next byte from the ring buffer
 *
 * NOTE: callers should call rb_can_dequeue to be sure there is data before calling this
 *
 * @param rb pointer to the ring buffer
 * @retval the byte read (0 if none)
 */
extern uint8_t rb_dequeue(ring_buffer_p rb);

/**
 * Check to see if the ring buffer has data
 *
 * @param rb pointer to the ring buffer to check
 * @retval true if there is data than can be dequeued, false otherwise
 */
extern bool rb_can_dequeue(ring_buffer_p rb);

/**
 * Check to see if the ring buffer is not full
 *
 * @param rb pointer to the ring buffer to check
 * @retval true if there is room in the ring buffer for at least one more byte, false otherwise
 */
extern bool rb_can_enqueue(ring_buffer_p rb);

#endif /* INC_RINGBUFFER_H_ */
