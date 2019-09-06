#ifndef UTILS_H_
#define UTILS_H_

int min(int i, int j);

void shuffle(int *array, int length);


////////// READ COMMAND LINE //////////

typedef struct {
	int randomized;
	int fast;
	char *in_filename;
	char *out_filename;
} command_line;

int read_input(int argc, char *argv[], command_line *input_args);


////////// READ INPUT FILE //////////

typedef struct {
	int num_points;
	double *point_list;
} read_io;

int file_readnodes(FILE *file, int *firstnode, read_io *io);


#endif // UTILS_H_
