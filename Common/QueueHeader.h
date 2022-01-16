#pragma once
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

//struktura koja predstavlja red tj kolekciju podataka
struct Queue {
	int front, rear, size;
	unsigned capacity;
	int* array;
};

struct Queue* createQueue(unsigned capacity);
int isFull(struct Queue* queue);
int isEmpty(struct Queue* queue);
void enqueue(struct Queue* queue, int item);
int dequeue(struct Queue* queue);
