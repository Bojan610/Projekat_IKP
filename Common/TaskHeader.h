#pragma once
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

struct Task {
	unsigned capacity;
	int size, rear, front;
	int* tasks;
};

struct Task* createTasks(unsigned capacity);
bool addTask(struct Task* task, int data);
int getTask(struct Task* task);