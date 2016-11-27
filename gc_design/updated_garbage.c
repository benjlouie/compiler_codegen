#include <stdio.h>
#include <stdlib.h>

#define LF_MAX 75

#define DEFAULT_TABLE_SIZE 128

#define OBJECT_TYPE 0
#define PRIMITIVE_TYPE 1

#define WHITE 0
#define GREY 1
#define BLACK 2

#define INT_TAG 1
#define STRING_TAG 2
#define BOOL_TAG 3

#define TAG_IND 0
#define SIZE_IND 1

struct QNode {
	struct Node *data;
	struct QNode *next;
};

struct Node {
	void *data;
	struct Node *next;
	int type; //either object or primitive
	int color; //white grey or black
	//@TODO locations where this memory is referenced
};

typedef struct hash_map {
	size_t table_size;
	size_t num_elems;
	struct sent **table;
} Map;

struct sent {
	struct Node *head;
	size_t num_elems;
};

typedef struct queue_t {
	struct QNode* head;
	struct QNode* tail;
	size_t num_elems;
} Queue;

Map *initGC(size_t table_size);
Map *addRef(Map *m, void *ref, int type);
Map *collectAndResize(Map *m);
void clearAll(Map *m);
void transfer(Map *src, Map *dest);
void insert(Map *m, struct Node *n);
struct Node *get(Map *m, void *ref);
void setGreys(Map *m);

struct Node *removeNonWhite(struct sent *s);
void addSent(struct sent *s, struct Node *n);
void freeSent(struct sent *s);

struct Node *makeNode(void *ref, int type);
Queue *makeQueue();
struct QNode *makeQNode(struct Node *n);
struct Node *pop(Queue *q);
void push(Queue *q, struct Node *n);
Queue *getGreySet(Map *m);

void print_all(Map *m);

/* registers and stack for testing purposes */
void *rbx, *r12, *r13, *r14, *r15, *rdi, *rsi;
long stack[32];
long top_of_stack = 31;

/* test functions */
void *make_new(long type, size_t, void *ref);

int main(int argc, char** argv) {
	Map *m = initGC(DEFAULT_TABLE_SIZE);
	rbx = malloc(sizeof(int));
	*(int *) rbx = 5;
	m = addRef(m, rbx, PRIMITIVE_TYPE);
	r12 = malloc(32);
	m = addRef(m, r12, PRIMITIVE_TYPE);
	r13 = malloc(64);
	m = addRef(m, r13, PRIMITIVE_TYPE);
	r14 = make_new(STRING_TAG, 4, (void *) "hello");
	m = addRef(m, r14, PRIMITIVE_TYPE);
	r15 = make_new(4, 4, r14);
	m = addRef(m, r15, OBJECT_TYPE);
	rsi = (void *) 0x60;
	rdi = NULL;
	int i;
	for (i = 0; i < 1000000; i++) {
		m = addRef(m, malloc(64), PRIMITIVE_TYPE);
	}
	printf("%p: %d\n", rbx, *(int *)rbx);
	printf("%p: %s\n", r14, (char *)(((long *) r14)[3]));
	printf("end table size: %ld\n", m->table_size);
	clearAll(m);
	return 0;
}

void *make_new(long type, size_t size, void *ref) {
	long *object = calloc(size*8, 1);
	object[TAG_IND] = type;
	object[SIZE_IND] = size;
	object[3] = (long) ref;
	return (void *) object;
}

Map *initGC(size_t table_size) {
	Map *m = calloc(sizeof(Map), 1);
	m->table_size = table_size;
	m->table = calloc(sizeof(struct sent *), table_size);
	return m;
}

long hash(void *key, size_t table_size) {
	return ((long) key >> 5) % table_size; //malloc is 32 bit aligned
}

Map *addRef(Map *m, void *ref, int type) {
	struct Node *n = makeNode(ref, type);
	insert(m, n);
	int LF = (100*m->num_elems) / m->table_size;
	if (LF > LF_MAX)
		m = collectAndResize(m);
	return m;
}

Map *collectAndResize(Map *m) {
	setGreys(m);
	Queue *gs = getGreySet(m);
	struct Node *ref, *nested_ref;
	long *object;
	while (gs->num_elems > 0) {
		ref = pop(gs);
		ref->color = BLACK;
		if (ref->type == PRIMITIVE_TYPE)
			continue; //primitive types cannot have further references
		object = (long *) ref->data;
		if (object[TAG_IND] == INT_TAG || object[TAG_IND] == BOOL_TAG)
			continue; //ints and bools don't malloc memory
		int i;
		for (i = 3; i < object[SIZE_IND]; i++) {
			nested_ref = get(m, (void *) &object[i]);
			if (nested_ref == NULL || nested_ref->color != WHITE)
				continue;
			if (nested_ref->type == OBJECT_TYPE) {
				nested_ref->color = GREY;
				push(gs, nested_ref);
			} else {
				nested_ref->color = BLACK;
			}
		}
	}
	free(gs);
	/* grey set is empty, anything white is unreachable */
	int table_size = m->table_size * 2;
	Map *new = initGC(table_size);
	transfer(m, new);
	clearAll(m);
	return new;
}

void transfer(Map *src, Map *dest) {
	struct Node* tmp;
	long i;
	for (i = 0; i < src->table_size; i++) {
		if (src->table[i] == NULL)
			continue;
		while ((tmp = removeNonWhite(src->table[i])) != NULL) {//racist AF I know
			insert(dest, tmp);
		}
	}
}

struct Node* removeNonWhite(struct sent *s) {
	if (s->num_elems == 0)
		return NULL;
	struct Node *tmp, *tmp2;
	if (s->head->color != WHITE) {
		tmp = s->head;
		s->head = s->head->next;
		s->num_elems--;
		return tmp;
	} else {
		tmp = s->head;
		while (tmp->next != NULL) {
			if (tmp->next->color != WHITE) {
				tmp2 = tmp->next;
				tmp->next = tmp2->next;
				s->num_elems--;
				return tmp2;
			}
			tmp = tmp->next;
		}
	}
	return NULL;
}

void insert(Map *m, struct Node *n) {
	long index = hash(n->data, m->table_size);
	if (m->table[index] == NULL)
		m->table[index] = calloc(sizeof(struct sent), 1);
	n->next = m->table[index]->head;
	m->table[index]->head = n;
	m->table[index]->num_elems++;
	m->num_elems++;
}

void clearAll(Map *m) {
	long i;
	for (i = 0; i < m->table_size; i++) {
		if(m->table[i] != NULL)
			freeSent(m->table[i]);
	}
	free(m->table);
	free(m);
}

void freeSent(struct sent *s) {
	struct Node *n = s->head;
	struct Node *tmp;
	while (n != NULL) {
		tmp = n;
		n = n->next;
		free(tmp->data);
		free(tmp);
	}
	free(s);
}

struct Node *get(Map *m, void *ref) {
	long index = hash(ref, m->table_size);
	if (m->table[index] == NULL || m->table[index]->head == NULL)
		return NULL;
	struct Node *tmp = m->table[index]->head;
	while (tmp != NULL) {
		if (tmp->data == ref)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

void push(Queue *q, struct Node *n) {
	struct QNode * qn = makeQNode(n);
	if (q->tail == NULL) {
		q->head = qn;
		q->tail = qn;
	} else {
		q->tail->next = qn;
		q->tail = qn;
	}
	q->num_elems++;
}

struct Node *pop(Queue *q) {
	struct Node *rv;
	struct QNode *save;
	save = q->head;
	rv = save->data;
	if (q->head == q->tail) {
		q->head = NULL;
		q->tail = NULL;
	} else {
		q->head = q->head->next;
	}
	q->num_elems--;
	free(save);
	return rv;
	
}

Queue *getGreySet(Map *m) {
	Queue *gs = calloc(sizeof(Queue), 1);
	struct Node* tmp;
	long i;
	for (i = 0; i < m->table_size; i++) {
		if (m->table[i] == NULL || m->table[i]->head == NULL)
			continue;
		tmp = m->table[i]->head;
		while (tmp != NULL) {
			if (tmp->color == GREY)
				push(gs, tmp);
			tmp = tmp->next;
		}
	}
	return gs;
}

struct Node *makeNode(void *ref, int type) {
	struct Node* n = malloc(sizeof(struct Node));
	n->data = ref;
	n->type = type;
	n->color = WHITE;
	n->next = NULL;
	return n;
}

void setGreys(Map *m) {
	struct Node *tmp;
	if ((tmp = get(m, rbx)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, r12)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, r13)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, r14)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, r15)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, rdi)) != NULL)
		tmp->color = GREY;
	if ((tmp = get(m, rsi)) != NULL)
		tmp->color = GREY;
	/* This will be replaced with actually searching
	 * from $rsp to the top of our stack which will
	 * be stored upon entry to our program */
	long stk_ptr = 0;
	while (stk_ptr < top_of_stack) {
		if ((tmp = get(m, (void *) stack[stk_ptr])) != NULL)
			tmp->color = GREY;
		stk_ptr++;
	}
}

void print_all(Map *m) {
	long i;
	for (i = 0; i < m->table_size; i++) {
		printf("table[%5ld]: ", i);
		if (m->table[i] == NULL)
			printf("NULL\n");
		else {
			struct Node *tmp = m->table[i]->head;
			long j;
			for (j = 0; j < m->table[i]->num_elems; j++) {
				printf("%p, ", tmp);
				tmp = tmp->next;
			}
			printf("%p\n", tmp);
		}
	}
}

struct QNode *makeQNode(struct Node *n) {
	struct QNode *qn = malloc(sizeof(struct QNode));
	qn->data = n;
	qn->next = NULL;
	return qn;
}
