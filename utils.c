#include "triangle.h"
#define INPUTLINESIZE 1024
#include <time.h>
// data structures used for the queue 
// in order to record all triangles in the
// .ele output file

void enqueue(node_t **head, Edge *val) 
{
	node_t *new_node = malloc(sizeof(node_t));
	if (!new_node) return;
	new_node->val = val;
	if (*head == NULL) {
		new_node->next = new_node;
		new_node->prev = new_node;
		*head = new_node;
	} else {
		node_t *tail = (*head)->prev;
		new_node->next = *head;
		(*head)->prev = new_node;
		new_node->prev = tail;
		tail->next = new_node;
		*head = new_node;
	}
}

Edge *dequeue(node_t **head) {
	if((*head) == NULL) return NULL;
	Edge *retval;
	if ((*head)->prev != (*head)) {
		node_t *tail = (*head)->prev;
		node_t *new_tail = tail->prev;
		retval = tail->val;
		free(tail);
		new_tail->next = *head;
		(*head)->prev = new_tail;
	} else {
		retval = (*head)->val;
		free(*head);
		*head = NULL;
	}
		
	return retval;
}

void swap_ints(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}
int get_random(int max)
{
	return rand() % max;
}
void shuffle(int *array, int length) 
{
	int i,j;
	srand(time(0));
	for (i = length-1; i > 0; i--) {
		j = get_random(i+1);
		swap_ints(&array[i], &array[j]);
	}
}
int min(int i, int j)
{
	if (i <= j) return i; 
	else return j;
}
// read line from input file--pretty much copied from
// Shewchuk's Triangle file reading in Triangle/triangle_io.c
char *readline(char *string, FILE *infile)
{
	char *result;
	do {
		result = fgets(string, INPUTLINESIZE, infile);
		if (result == (char *) NULL) {
			return result;
		}
		while ((*result != '\0') && (*result != '#')
			&& (*result != '.') && (*result != '+') && (*result != '-')
			&& ((*result < '0') || (*result > '9'))) {
				result ++;
		} 
		
	} while ((*result == '#') || (*result == '\0'));
	return result;
}
// find the next field-copied from Triangle/triangle_io.c
char *findfield(char *string)
{
	char *result;
	result = string;
	while ((*result != '\0') && (*result != '#') && (*result != ' ') && (*result != '\t')) {
		result++;
	}
	while ((*result != '\0') && (*result != '#')
		&& (*result != '.') && (*result != '+') && (*result != '-')
		&& ((*result < '0') || (*result > '9'))) {
			result++;
	}
	if (*result == '#') {
		*result = '\0';
	}
	return result;
}
int file_writenodes(FILE *file, Edge *e, read_io *io, int first_node, int max_index)
{
	// header
	// this is wrong but we'll overwrite it later
	fprintf(file, "%d 3 0\n", io->num_points);
	node_t *q = NULL;
	// initialize queue for finding all triangles 
	enqueue(&q, e);
	Point *p1; Point *p2; Point *p3;
	int i = 1;
	while (q) {
		e = dequeue(&q);
		if (e->trav == 2) continue; // left face already processed
		else {	
			e->trav = 2; // make sure each face only processed once
			lnext(e)->trav = 2; lprev(e)->trav = 2;
			// add edges of neighboring faces if they haven't yet been added to the queue
			if (!(sym(e)->trav)) {
				enqueue(&q, sym(e));
				sym(e)->trav = 1; // make sure each edge only added once
			} 
			if (!(sym(lnext(e))->trav)) {
				enqueue(&q, sym(lnext(e)));
				sym(lnext(e))->trav = 1;
			}
			if (!(sym(lprev(e))->trav)) {
				enqueue(&q, sym(lprev(e)));
				sym(lprev(e))->trav = 1;
			}
			p1 = e->Org; p2 = e->Dest; p3 = lnext(e)->Dest;
			// if any point is symbolic, skip the face
			if (symbolic(p1) || symbolic(p2) || symbolic(p3)) {	
				continue;
			} else {
				// write left face of current edge
				// change id of lexico. max vertex
				// points indexed at 1 in the data structure, so 
				// update if were indexed at 0 in input file
				// first_node is label of first node in input file (0 or 1)
				if (p1->id == 0) p1->id = max_index + first_node;
				if (p2->id == 0) p2->id = max_index + first_node;
				if (p3->id == 0) p3->id = max_index + first_node;
				fprintf(file, "%d %d %d %d\n", i - 1 + first_node, p1->id, p2->id, p3->id); 
				i++;	
			}			
		}
		// free memory if both faces of an edge have been processed					
		if (sym(e)->trav == 2) delete_edge(e);
	}
	return i - 1;
}
// read vertices from a .node file. Inspiration from Triangle/triangle_io.c
int file_readnodes(FILE *file, int *firstnode, read_io *io)
{
	char inputline[INPUTLINESIZE];
	char *stringptr;
	int mesh_dim;
	int invertices;
	int i;
	double x, y;
	stringptr = readline(inputline, file);
	if (stringptr == (char *) NULL) {
		return 1;
	}
	invertices = (int) strtol(stringptr, &stringptr, 0);
	stringptr = findfield(stringptr);	
	if (*stringptr == '\0') {
		mesh_dim = 2;
	} else {
		mesh_dim = (int) strtol(stringptr, &stringptr, 0);
	}
	// assume no attributes or boundary markers
	if (invertices < 3) {
		return 1;
	}
	if (mesh_dim != 2) {
		return 1;
	}
	double *pointlist = malloc(2*invertices*sizeof(double));
	// read the vertices
	for (i = 0; i < invertices; i++) {
		stringptr = readline(inputline, file);
		if (stringptr == (char *) NULL) {
			free(pointlist);
			return 1;
		} 
		if (i == 0) {
			*firstnode = (int) strtol(stringptr, &stringptr, 0);
		}
		stringptr = findfield(stringptr);
		if (*stringptr == '\0') {
			free(pointlist);	
			return 1;
		}
		x = (double) strtod(stringptr, &stringptr);
		stringptr = findfield(stringptr);
		if (*stringptr == '\0') {
			free(pointlist);
			return 1;
		}
		y = (double) strtod(stringptr, &stringptr);
		pointlist[2*i+0] = x;
		pointlist[2*i+1] = y;
	}
	io->num_points = invertices;
	io->point_list = pointlist;
	return 0;
}


// read command line args

int read_input(int argc, char *argv[], command_line *input_args)
{
	int opt;
	opterr = 0;
	int index;
	while((opt = getopt(argc, argv, "rfi:o:")) != -1)
	{
		switch(opt)
		{
			case 'r':
				input_args->randomized = 1;
				break;
			case 'f':
				input_args->fast = 1;
				break;
			case 'i':
				input_args->in_filename = optarg;
				break;
			case 'o':
				input_args->out_filename = optarg;
				break;
			case '?':
				if (optopt == 'i') {
					printf("Please specify input filename\n");
					return 1;
				} else if (optopt == 'o') {
					input_args->out_filename = "output.ele";
					break;	
				} else {
					printf("Unknown option\n");
					return 1;
				}
			default:
				abort();
		}
	}
	if (!input_args->in_filename) {
		printf("Please specify input filename\n");
		return 1;
	}
	if (!input_args->out_filename) {
		input_args->out_filename = "output.ele";
	}
	if (input_args->randomized) printf("Randomized insertion.\n");
	else printf("NOT randomized insertion.\n");
	if (input_args->fast) printf("Fast point location!\n");
	else printf("Slow point location :(\n");
	printf("Input filename is: %s\n", input_args->in_filename);
	printf("Output filename is: %s\n", input_args->out_filename);
	for(index = optind; index < argc; index ++) {
		printf("Extra argument not used: %s\n", argv[index]);
	}
	return 0;
}
