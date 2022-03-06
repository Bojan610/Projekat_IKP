#include "../Common/QueueHeader.h"

/*
	Funkcija: createQueue
	---------------------------------------------------
	Funkcionalnost: Kreiranje kolekcije podataka - red

	Parametri: 'capacity' - predstavlja kapacitet reda, tj velicinu reda

	Povratna vrednost: kreirani red
*/
struct Queue* createQueue(unsigned capacity)
{
	struct Queue* queue = (struct Queue*)malloc(
		sizeof(struct Queue));
	queue->capacity = capacity;
	queue->front = queue->size = 0;

	queue->rear = capacity - 1;
	queue->array = (int*)malloc(
		queue->capacity * sizeof(int));
	return queue;
}

/*
	Funkcija: isFull
	---------------------------------------------------
	Funkcionalnost: Proverava da li je prosledjen red popunjen

	Parametri: red koji proveravamo da li je pun

	Povratna vrednost: 0 ako red nije pun, 1 ako je red pun
*/
int isFull(struct Queue* queue)
{
	return (queue->size == queue->capacity);
}

/*
	Funkcija: isEmpty
	---------------------------------------------------
	Funkcionalnost: Proverava da li je prosledjen red prazan

	Parametri: red koji proveravamo da li je prazan

	Povratna vrednost: 0 ako red nije prazan, 1 ako je red prazan
*/
int isEmpty(struct Queue* queue)
{
	return (queue->size == 0);
}

/*
	Funkcija: enqueue
	---------------------------------------------------
	Funkcionalnost: Funkcija koja dodaje element u red

	Parametri: red u koji dodajemo podatak, i sam podatak koji se dodaje u red

	Povratna vrednost: /
*/
bool enqueue(struct Queue* queue, int item)
{
	if (isFull(queue))
		return false;
	queue->rear = (queue->rear + 1)
		% queue->capacity;
	queue->array[queue->rear] = item;
	queue->size = queue->size + 1;
	printf("%d enqueued to queue\n", item);

	return true;
}

/*
	Funkcija: dequeue
	---------------------------------------------------
	Funkcionalnost: Funkcija koja brise prvi dodat element u red

	Parametri: red iz kog se brise podatak

	Povratna vrednost: obrisan element
*/
int dequeue(struct Queue* queue)
{
	if (isEmpty(queue))
		return -1;
	int item = queue->array[queue->front];
	queue->front = (queue->front + 1)
		% queue->capacity;
	queue->size = queue->size - 1;
	return item;
}