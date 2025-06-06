/*
 * ringbuffer.c
 *
 * Implement a FIFO ring buffer for bytes.
 *
 *  Created on: Jun 4, 2025
 *      Author: pjw
 */

#include "ringbuffer.h"

/**
 * Initialize the ring buffer
 */
void rb_init(ring_buffer_p rb) {
	rb->read_idx = 0;
	rb->write_idx = 0;
	rb->is_full = false;
	rb->is_empty = true;
}

/**
 * Enqueue a byte to the ring buffer (fails silently)
 *
 * @param rb pointer to the ring buffer
 * @param data the byte to enqueue
 */
void rb_enqueue(ring_buffer_p rb, uint8_t data) {
	if (rb_can_enqueue(rb)) {
		rb->buffer[rb->write_idx++] = data;
		if (rb->write_idx >= MAX_RING_BUFFER_SIZE) {
			rb->write_idx = 0;
		}

		// Set the data status flags
		rb->is_empty = false;
		if ((rb->write_idx + 1 == rb->read_idx) ||
			((rb->read_idx == 0) && (rb->write_idx + 1 == MAX_RING_BUFFER_SIZE))) {
			rb->is_full = true;
		} else {
			rb->is_full = false;
		}
	}
}

/**
 * Read the next byte from the ring buffer
 *
 * NOTE: callers should call rb_can_dequeue to be sure there is data before calling this
 *
 * @param rb pointer to the ring buffer
 * @retval the byte read (0 if none)
 */
uint8_t rb_dequeue(ring_buffer_p rb) {
	if (rb_can_dequeue(rb)) {
		uint32_t data = rb->buffer[rb->read_idx];
		rb->read_idx++;
		if (rb->read_idx >= MAX_RING_BUFFER_SIZE) {
			rb->read_idx = 0;
		}

		// Set the data status flags
		rb->is_full = false;
		rb->is_empty = (rb->write_idx == rb->read_idx);

		return data;
	} else {
		return 0;
	}
}

/**
 * Check to see if the ring buffer has data
 *
 * @param rb pointer to the ring buffer to check
 * @retval true if there is data than can be dequeued, false otherwise
 */
bool rb_can_dequeue(ring_buffer_p rb) {
	return !(rb->is_empty);
}

/**
 * Check to see if the ring buffer is not full
 *
 * @param rb pointer to the ring buffer to check
 * @retval true if there is room in the ring buffer for at least one more byte, false otherwise
 */
bool rb_can_enqueue(ring_buffer_p rb) {
	return !(rb->is_full);
}
