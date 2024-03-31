#ifndef STARTUP_BLQUEUE_H_
#define STARTUP_BLQUEUE_H_

#define BL_QUEUE_LEN 100

typedef struct {
	uint8_t start; //array index of first element in queue
	uint8_t next; //array index of the next element to write
	uint8_t buf[BL_QUEUE_LEN];
	uint8_t len;
} BLQueue;

void BLQueueInit(BLQueue *q) {
	q->start = 0;
	q->next = 0;
	q->len = 0;
}

inline void BLQueueFixBounds(BLQueue *q) {
	if(q->start >= BL_QUEUE_LEN)
		q->start -= BL_QUEUE_LEN;
	if(q->next >= BL_QUEUE_LEN)
		q->next -= BL_QUEUE_LEN;
	if(q->len > BL_QUEUE_LEN)
		q->len = BL_QUEUE_LEN;
}

void BLQueueAdd(BLQueue *q, uint8_t element) {
	q->buf[q->next] = element;
	q->next++;
	q->len++;

	if(q->len == BL_QUEUE_LEN)
		q->start++;

	BLQueueFixBounds(q);
}

void BLQueueAddArray(BLQueue *q, uint8_t *inbuf, uint8_t len) {
	for(uint8_t i = 0; i < len; i++)
		BLQueueAdd(q, inbuf[i]);
}

uint8_t BLQueueGetIdx(BLQueue *q, uint8_t idx) {
	uint8_t retIdx = idx + q->start;
	while(retIdx > BL_QUEUE_LEN)
		retIdx -= BL_QUEUE_LEN;

	return q->buf[retIdx];
}

#endif /* STARTUP_BLQUEUE_H_ */
