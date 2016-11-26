Intel
instr dest,src


USING STANDARD x86-64 calling convention

rbp+24	arg 3
rbp+16	arg 2	
rbp+8	arg 1
rbp+0	previous IP


REG ARGS

RDI	RSI	RDX	R8	R9	XMM0...XMM7


calloc RDI-Number of elements	RSI-size of elements


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



/**
 * Simple node for a singly linked list
 */
struct Node {								256	bit		32	byte
	void *data;								64	bit		8	byte
	struct Node *next;						64	bit		8	byte
	int type; //either object or primitive 	64	bit		8	byte
	int color; //white grey or black 		64	bit		8	byte
	//@TODO locations where this memory is referenced
};

/**
 * Hash map uses chaining and
 * pointers to sentinel nodes to do that
 * keeps track of num_elems and
 * table_size for load factor calculations
 */
typedef struct hash_map {				192	bit		24	byte
	size_t table_size;					64	bit		8	byte
	size_t num_elems;					64	bit		8	byte
	struct sent **table;				64	bit		8	byte
} Map;

/**
 * Sentinel node, it keeps track of the
 * head of the linked list and number
 * of elements in it
 */
struct sent {							128 bit		16	byte
	struct Node *head;					64 	bit		8 	byte
	size_t num_elems;					64 	bit		8 	byte(UNSIGNED)
};

/**
 * Simple queue implemented as
 * a linked list
 */
typedef struct queue_t {				192	bit		24	byte
	struct Node* head;					64	bit		8	byte
	struct Node* tail;					64 	bit		8	byte
	size_t num_elems;					64	bit		8	byte(UNSIGNED)
} Queue;



#--------------------------------------------------------------------------------------------------------------------
#size_t table_size = rbp+8
initGC:							#label of initGC function
	mov		rbp,rsp				#move the stack pointer into the base pointer
	mov		rdi,1				#prepare to calloc 1 element
	mov 	rsi,24				#prepare to calloc element of size 24 bytes
	push 	rbp					#push the base pointer
	call 	calloc				#calloc with return pointer in rax this is Map m
	pop		rbp					#pop the base pointer
	mov		[rax],[rbp+8]		#move table_size into m->table_size
	push	rax					#save Map m for later
	mov 	rdi,16				#prepare to calloc sizeof(sent *) elements
	mov 	rsi,[rbp+8]			#prepare to calloc table_size number of elements
	push 	rbp					#push the base pointer
	call 	calloc				#calloc with return pointer in rax this is for m->table
	pop		rbp					#pop the base pointer
	pop		rbx					#we put Map m into rbx
	mov 	[rbx+16],rax		#we put the recently calloc'd memory into m->table
	mov 	rax, rbx			#move Map m into rax for the return
	mov 	rsp,rbp				#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------
#size_t 	table_size 	= rbp+16
#void* 	key			= rbp+8
hash:							#label of hash function
	mov 	rbp,rsp				#move the stack pointer into the base pointer
	xor		rax,rax				#zero out rax with xor
	mov 	rax,[rbp+8]			#move key into rax
	shr 	rax,5				#shift rax to the right by 5 bits
	xor		rdx,rdx 			#zero out the rdx register with xor
	mov 	rcx,[rbp+16] 		#move table_size into rcx
	div		rcx 				#do 64 bit unsigned division on rax by rcx, rax = rax/rcx
	mov 	rsp,rbp 			#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------
#int 	type = rbp+24
#void* 	ref = rbp+16
#Map* 	mov = rbp+8
addRef:
	mov 	rbp,rsp				#move the stack pointer into the base pointer
	push 	rbp					#push rbp to prepare for call
	push 	[rbp+24]			#push int type to prepare for call
	push 	[rbp+16]			#push void* ref to prepare for call
	call 	makeNode 			#the call to the make node function
	sub 	rsp,16 				#subtract 16 from rsp
	pop 	rbp 				#pop the base pointer
	push 	rbp					#push the base pointer to prepare for call 
	push 	rax					#push Node* n to prepare for call
	push 	[rbp+8]				#push Map* m to prepare for call
	call 	insert				#the call to the insert function
	sub 	rsp,16
	pop		rbp
	xor		rax,rax				#zero out rax
	mov 	rax,[rbp+8]			#move Map* m into rax
	mov 	rax,[rax+8]			#move m->num_elems into rax
	xor 	rbx,rbx				#zero out rbx
	mov 	rbx,100				#move 100 into rbx
	mul		rbx					#do 64 bit unsigned multiply on rax by rbx,  rax=rax*rbx
	mov 	rcx,[rbp+8]			#move Map* m into rcx	
	mov 	rcx,[rcx]			#move m->table_size into rcx
	xor		rdx,rdx`			#zero out rdx using xor
	div 	rcx 				#do 64 bit unsigned division on rax by rcx, rax = rax/rcx, int LF is now in rax
	cmp 	rax,75				#compare int LF to LF_MAX, LF > LF_MAX
	jle		LFngtLF_MAX			#if LF <= LF_MAX then jump to ending
	push 	rbp					#push rbp
	push 	[rbp+8]				#push Map* m
	call 	collectAndResize	#call collectAndResize(m)
	subt 	rsp,8				#subtract 8 from rsp
	pop 	rbp					#return rbp
	mov 	[rbp+8],rax			#move the return from collectAndResize into Map* m on the stack
LFngtLF_MAX:
	mov  	rax,[rbp+8]			#move Map* m into rax for return
	mov 	rsp,rbp				#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------
#Map*  	m = rbp+8
#fuck this we do it live
collectAndResize:
	mov 	rbp,rsp 			#move the stack pointer into the base pointer

	mov 	rsp,rbp 			#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------
#Map* 	dest = rbp+16
#Map* 	src = rbp+8
transfer:
	move 	rbp,rsp				#move the stack pointer into the base pointer
	xor 	rax,rax				#zero out the rax register with xor, this will be long i 
	mov 	rbx,[rbp+8]			#move Map* src into rbx
	mov 	rbx,[rbx]			#move src->table_size into rbx
transfer_CMP:
	cmp 	rax,rbx 			#compare i with src->table_size, i< src->table_size
	jge 	transfer_END_FUNCT	#if i >= src->table_size then jump to end
	#rax and rbx taken
	mov 	rcx,[rbp+8]			#move Map* src into rcx
	mov 	rcx,[rcx+16]		#move src->table into rcx
	mov 	rcx,[rcx+rax]		#move src->table[i] into rcx
	cmp 	rcx,0				#check if src->table[i] == NULL
	jne 	transfer_NOTNULL	#if not NULL carry on
	inc 	rax					#increcment rax
	jmp 	transfer_CMP		#continue as in continue;
transfer_NOTNULL:
	#save rax and rbx, dont save rcx
	push 	rbp					#push rbp to save it
	push 	rax					#push rax to save it
	push 	rbx					#push rbx to save it
	push 	rcx					#push rcx for function call
	call 	removeNonWhite
	mov 	rax,rdx 			#move the return of removeNonWhite into rdx
	sub 	rsp,8				#substract 8 from rsp
	pop		rbx					#restore rbx
	pop 	rax					#restore rax
	pop 	rbp					#restore rbp
	cmp 	rdx,0				#compare the return of removeNonWhite to 0
	je 		transfer_END_WHILE	#tmp == NULL then jump to end of while function
	#save rax and rbx
	push 	rbp					#push rbp to save it
	push 	rax					#push rax to save it
	push 	rbx					#push rbx to save it
	push 	rdx 				#push rdx for call to insert, this is tmp
	push 	[rbp+16]			#push Map* dest
	call 	insert 				#call insert which is return type void
	sub 	rsp,16 				#subtract 16 from rsp
	pop 	rbx					#restore rbx
	pop		rax					#restore rax
	pop  	rbp					#restore rbp
transfer_END_WHILE:
	inc 	rax 				#increment rax, i++
	jmp 	transfer_CMP 		#jump back to beginning of for loop
transfer_END_FUNCT:
	mov 	rsp,rbp				#move the base pointer into the stack pointer
	xor		rax,rax				#zero out rax with xor
	return
#--------------------------------------------------------------------------------------------------------------------
#struct sent*	s = rbp+8
removeNonWhite:
	mov 	rbp,rsp 			#move the stack pointer into the base pointer

	mov 	rsp,rbp 			#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------
#struct Node* 	n = rbp +16
#Map* 			m = rbp+8
insert:
	mov 	rbp,rsp 			#move the stack pointer into the base pointer
	mov 	rax,[rbp+16]		#move n into rax
	mov 	rax,[rax] 			#move n->data into rax
	mov 	rbx,[rbp+8]			#move m into rbx
	mov 	rbx,[rbx] 			#move m->table_size into rbx
	push  	rbp					#push rbp to save it
	push 	rbx					#push m->table_size for function call
	push 	rax					#push n->data for function call
	call 	hash 				#call hash, should return the index into rax
	sub 	rsp,16				#subtact 16 from rsp
	pop 	rbp					#restore rbp
	xor 	rbx,rbx				#zero out rbx with xor
	mov 	rbx,[rbp+8]			#move Map* m into rbx
	mov 	rbx,[rbx+24]		#move m->table into rbx
	mov 	rbx,[rbx+rax]		#move m->table[index] into rbx
	cmp 	rbx,0 				#check if m->table[index] is NULL
	jne 	insert_NOTNULL		#if not null continue with function
	push 	rbp					#push rbp to save it
	push 	rbx					#push rbx to save it this is m->table[index]
	mov 	rdi,16				#prepare to calloc with element size of 16
	mov 	rsi,1				#prepare to calloc with 1 element
	call 	calloc				#call calloc
	pop 	rbx					#restore rbx, this is m->table[index]
	pop 	rbp					#restore rbp
	mov 	rbx,rax 			#move calloc(sizeof(struct sent), 1) into m->table[index]
insert_NOTNULL:
	xor 	rax,rax 			#zero out rax with xor
	mov 	rax,[rbp+16] 		#move n into rax
	mov 	rax,[rax+8] 		#move n->next into rax
	mov 	rcx,rbx 			#move m->table[index] into rcx
	mov 	rcx,[rcx] 			#move m->table[index]->head into rcx
	mov 	rax,rcx 			#move m->table[index]->head into n->next
	mov 	rcx,[rbp+16] 		#move n into m->table[index]->head
	mov 	rcx,rbx 			#move m->table[index] into rcx
	xor 	rax,rax 			#zero out rax with xor
	mov 	rax[rcx+8]			#move m->table[index]->num_elems into rax
	inc 	rax 				#increment rax by 1
	mov 	rcx+8,rax 			#move num_elems++ into m->table[index]->num_elems
	mov 	rsp,rbp 			#move the base pointer into the stack pointer
	return
#--------------------------------------------------------------------------------------------------------------------