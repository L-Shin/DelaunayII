#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "utils.h"
#define INPUTLINESIZE 1024


// boring util
int min(int i, int j)
{
	if (i <= j) return i; 
	else return j;
}


////////// RANDOM POINT INSERTION //////////

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
// store indices [0, length-1] in 'array' and if 'shuffled' randomly shuffle 
void generate_indices(int *array, int length, int shuffled)
{
	for (int i = 0; i < length; i++) array[i] = i;
	if (shuffled) shuffle(array, length);
}

////////// READ COMMAND LINE ARGS //////////

// read command line input and store in 'command_line' object
int read_input(int argc, char *argv[], command_line *input_args)
{
	int opt;
	opterr = 0;
	int index;
	while((opt = getopt(argc, argv, "rfi:o:")) != -1)
	{
		switch(opt)
		{
			case 'r': //randomized point insertion
				input_args->randomized = 1;
				break;
			case 'f': //fast point location
				input_args->fast = 1;
				break;
			case 'i': //input filename specified
				input_args->in_filename = optarg;
				break;
			case 'o': //output filename specified
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
	if (input_args->randomized) printf("Randomized insertion\n");
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


////////// READ .node INPUT FILE //////////

// Basically all stolen from Prof. Shewchuk's awesome Triangle program at
// https://github.com/wo80/Triangle/blob/master/src/Triangle/triangle_io.c


// read line from input file 
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

// find the next field
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

// Read vertices from a .node file. Write to read_io structure.
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



/////////// FIND MAX POINT IN INPUT //////////////

void find_max(read_io *io, int *max_index, Point *max)
{
	int i;
	double x, y;
	max->x = io->point_list[0];
	max->y = io->point_list[1];
	max->id = 0;
	int index = 0;
	for (i = 1; i < io->num_points; i++) {
		x = io->point_list[2*i];
		y = io->point_list[2*i+1];
		if ((x > max->x) || ((x == max->x) && (y > max->y))) {
			max->x = x;
			max->y = y;
			index = i;
		}
	}
	*max_index = index;
}

/////// QUEUE USED TO PRINT OUTPUT ///////////

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

Edge *dequeue(node_t **head) 
{
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

	
