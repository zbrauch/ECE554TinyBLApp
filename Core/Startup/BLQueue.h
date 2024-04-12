#ifndef STARTUP_BLQUEUE_H_
#define STARTUP_BLQUEUE_H_

#define BL_QUEUE_LEN 100

#pragma GCC push_options
#pragma GCC optimize ("-Ofast")

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

void BLQueueFixBounds(BLQueue *q) {
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

//pulls a 10 byte command out of the buffer if one exists
	//Commands start with sopChar and end with 'E'
	//Also moves the start of the buffer to the end of the extracted command
	//returns 1 if command extracted, 0 if none found
uint8_t BLQueueExtractInit(BLQueue *q, uint8_t *outBuf, uint8_t sopChar) {
	for(uint8_t i = 0; i < q->len - 9; i++) {
		if(BLQueueGetIdx(q, i) == sopChar) {
			if(i+9 < q->len && BLQueueGetIdx(q, i+9) == 'E') {
				for(uint8_t j = 0; j < 10; j++)
					outBuf[j] = BLQueueGetIdx(q, i+j);

				q->start += i+10;
				q->len -= (i+10);
				BLQueueFixBounds(q);
				return 1;
			}
		}
	}
	return 0;
}

//pulls a 20 byte command out of the buffer if one exists
	//Commands start with sopChar and end with 'E'
	//Also moves the start of the buffer to the end of the extracted command
	//returns 1 if command extracted, 0 if none found
uint8_t BLQueueExtractData(BLQueue *q, uint8_t *outBuf, uint8_t sopChar) {
	for(uint8_t i = 0; i < q->len - 19; i++) {
		if(BLQueueGetIdx(q, i) == sopChar) {
			if(i+19 < q->len && BLQueueGetIdx(q, i+19) == 'E') {
				for(uint8_t j = 0; j < 20; j++)
					outBuf[j] = BLQueueGetIdx(q, i+j);

				q->start += i+20;
				q->len -= (i+20);
				BLQueueFixBounds(q);
				return 1;
			}
		}
	}
	return 0;
}
#pragma GCC pop_options

#endif /* STARTUP_BLQUEUE_H_ */
