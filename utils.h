#ifndef UTILS_H_
#define UTILS_H_

typedef struct Point Point;
struct Point {
	double x;
	double y;
	int id; // negative id indicates symbolic point, see Dutch book. 
	        // lexico. max point is aliased as 0
		// all other points have ids that indicate their (1-indexed) 
		// location in the .node input file
};
typedef struct Edge Edge;
typedef struct Node Node;
struct Edge {
	Point *Org; 
	Point *Dest;
	Edge *Next; // next ccw edge with the same left face
	Edge *Rot; // edge in quadedge obtained by 90 deg ccw rotation
	int trav; // boolean indicates whether this edge's left face has been 
		  // recorded in the .ele output file
	Node *face; // pointer to the left face node in the DAG

};
struct Node {
	int type; // 0 for edge, 1 for face
	
	// Properties for internal node
	Node *left; // nodes to the left/right by comparison with edge org -> dest 
	Node *right; 

	Point *Org; // coordinates of origin and destination of corresponding edge in triangulation
	Point *Dest; // to be used if node is an edge node

	
	// Properties for leaf node
	Edge *edge; // pointer to an edge in the triangulation 
		    // such that this face lies to the left (if this node is a face)

};
	


int min(int i, int j);

//////////// RANDOM POINT INSERTION ////////////

void generate_indices(int *array, int length, int shuffled);

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

void find_max(read_io *io, int *max_index, Point *max);

//////////// QUEUE USED TO PRINT OUTPUT ///////////

typedef struct node {
	Edge *val; 
	struct node *next;
	struct node *prev;
} node_t;

void enqueue(node_t **head, Edge *val);
Edge *dequeue(node_t **head);

#endif // UTILS_H_
