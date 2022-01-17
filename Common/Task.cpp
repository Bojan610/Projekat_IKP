#include "TaskHeader.h"

struct Task* createTasks(unsigned capacity) {
	struct Task* task = (struct Task*)malloc(sizeof(struct Task));
	
	task->capacity = capacity;
	task->front = 0;
	task->size = 0;
	task->rear = capacity - 1;
	task->tasks = (int*)malloc(task->capacity * sizeof(int));

	return task;
}

bool addTask(struct Task* task, int data) {
	if (task->size == task->capacity)
		return false;

	task->rear = (task->rear + 1) % task->capacity;
	task->tasks[task->rear] = data;
	task->size = task->size + 1;

	return true;
}

int getTask(struct Task* task) {
	if (task->size == 0)
		return INT_MIN;

	int data = task->tasks[task->front];
	task->front = (task->front + 1) % task->capacity;
	task->size = task->size - 1;

	return data;
}