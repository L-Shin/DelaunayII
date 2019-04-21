#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "predicates.h"
#define INPUTLINESIZE 1024
//Data structures//

typedef struct Edge Edge;
typedef struct node {
	Edge *val;
	struct node *next;
	struct node *prev;
} node_t;

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
		
	printf("in something good\n");
	return retval;
}
// Point will store vertex coordinates, 
// but each quad-edge will have its own two Point fields, 
// quad-edges that share a vertex will not reference the same Point objects, 
// but the two edges in the same quad-edge WILL reference the same point fields if appropriate. 
// This makes updating Org/Dest easy. 
struct Point {
	double x;
	double y;
	int id; //negative id indicates symbolic point, 0 indicates lexico max point
		// other ids indicate location in .node input file
};

typedef struct Point Point; 

int symbolic(Point *p) 
{
	return (p->id < 0);
}

int real(Point *p) 
{
	return !symbolic(p);
}
//return if points identical by id comparison
int equal(Point *p1, Point *p2) 
{
	return p1->id == p2->id;
}

// returns 1 if p1>p2, 0 if equal, -1 if p1<p2
int lexico_comp(Point *p1, Point *p2) 
{	
	if ((p1->x == p2->x) && (p1->y == p2->y)) return 0;
	else if (p1->id == -1) return -1; // p-1 is smaller than all points
	else if (p2->id == -1) return 1;
	else if (p1->id == -2) return 1; // p-2 is larger than all points
	else if (p2->id == -2) return -1;
	// both points real
	else if ((p1->x > p2->x) || ((p1->x == p2->x) && (p1->y > p2->y)))
		return 1;
	else return -1;
}

// Return 1 if (p1, p2, p3) CCW, 0 if collinear, -1 if clockwise
// Should only be used for real points
int ccw(Point *p1, Point *p2, Point *p3) 
{
	double ap1[2] = {p1->x, p1->y};
	double ap2[2] = {p2->x, p2->y};
	double ap3[3] = {p3->x, p3->y};
	double o2d = orient2d(ap1, ap2, ap3);
	if (o2d < 0) return -1;
	else if (o2d == 0) return 0;
	else return 1;
	
}

// Return 1 if p4 lies inside p1p2p3, 0 if cocircular, -1 if p4 lies outside p1p2p3
// Should only be used for real points
int in_circle(Point *p1, Point *p2, Point *p3, Point *p4)
{
		
	double ap1[2] = {p1->x, p1->y};
	double ap2[2] = {p2->x, p2->y};
	double ap3[2] = {p3->x, p3->y};
	double ap4[2] = {p4->x, p4->y};
	return incircle(ap1, ap2, ap3, ap4) > 0; 
} 
// Utility for parsing command line
typedef struct {
	int randomized;
	int fast;
	char *in_filename;
	char *out_filename;
} command_line;

typedef struct {
	int num_points;
	double *point_list;
} read_io;


// Quad-edge will be 4 Edge structures pointing to each other. Two will correspond to an actual edge in the triangulation, and two their duals

struct Edge {
	Point *Org;
	Point *Dest;
	Edge *Next;
	Edge *Rot;
	int trav; // set trav to 1 if added to queue, 2 if its left face has been processed
};


//Basic edge operations//

Edge *rot(Edge *e) 
	{return e->Rot;}
Edge *onext(Edge *e) 
	{return e->Next;}
Edge *sym(Edge *e) 
	{return e->Rot->Rot;}
Edge *rot_inv(Edge *e) 
	{return e->Rot->Rot->Rot;}
Edge *oprev(Edge *e)
	{return rot(onext(rot(e)));}
Edge *lprev(Edge *e)
	{return sym(onext(e));}
Edge *lnext(Edge *e) 
	{return rot_inv(onext(rot(e)));}
Edge *dprev(Edge *e)
	{return rot_inv(onext(rot_inv(e)));}
Edge *make_edge()
{
	Edge *e1 = malloc(sizeof(Edge)); Edge *e2 = malloc(sizeof(Edge));
	Edge *e3 = malloc(sizeof(Edge)); Edge *e4 = malloc(sizeof(Edge));
	Point *org = malloc(sizeof(Point));
	Point *dest = malloc(sizeof(Point));
	e1->trav = 0; e3->trav = 0;	
	e1->Rot = e2; e2->Rot = e3; e3->Rot = e4; e4->Rot = e1;
	e1->Next = e1; e3->Next = e3;
	e2->Next = e4; e4->Next = e2;
	e1->Org = org; e1->Dest = dest;
	e3->Org = dest; e3->Dest = org;
	return e1;
}
//Used to update the Org/Dest of e and sym(e) simultaneously
void set_equal(Point *p1, Point *p2)
{
	p1->x = p2->x;
	p1->y = p2->y;
	p1->id = p2->id;
}

//splice edges
void splice(Edge *a, Edge *b) 
{
	Edge *alpha = rot(onext(a));
	Edge *beta = rot(onext(b));
	Edge *bOnext = onext(b); Edge *aOnext = onext(a);
	Edge *betaOnext = onext(beta); Edge *alphaOnext = onext(alpha);
	
	a->Next = bOnext; b->Next = aOnext;
	alpha->Next = betaOnext; beta->Next = alphaOnext;
//	rot(a)->Next = beta; rot(b)->Next = alpha;
//	rot(alpha)->Next = b; rot(beta)->Next = a;
}

void delete_edge(Edge *e) 
{
	splice(e, oprev(e));
	splice(sym(e), oprev(sym(e)));
	free(e->Org); free(e->Dest);
	
	for (int i = 0; i < 3; i++) {
		Edge *prev = e; e = rot(e);
		free(prev);
	}
	free(e);
}

void swap(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a); splice(sym(e), b);
	splice(e, lnext(e)); splice(sym(e), lnext(b));
	set_equal(e->Org, a->Dest); set_equal(e->Dest, b->Dest);
}
// p = alpha*org + (1-alpha)*dest <-> alpha = (p - dest)/(org - dest)
// Use for real or symbolic points
int on(Point *p, Edge *e)
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return 0;
	else {
		double point[2] = {p->x, p->y};
		double p1[2] = {e->Org->x, e->Org->y};
		double p2[2] = {e->Dest->x, e->Dest->y};
		return (orient2d(point, p1, p2) == 0) && 
		(((point[0] <= p1[0]) || (point[0] <= p2[0])) && 
		((point[0] >= p1[0]) || (point[0] >= p2[0])));
	}
}
// Determine whether point pj lies to the left of the line from pi -> pk
int left_of_symbolic(Point *pj, Point *pi, Point *pk)
{
	if (pk->id == -1) return lexico_comp(pi, pj) < 0;
	else if (pi->id == -1) return lexico_comp(pi, pj) > 0;
	else if (pi->id == -2) return lexico_comp(pk, pj) < 0;
	else if (pk->id == -2) return lexico_comp(pk, pj) > 0;
	// p-1 left of pi->pk if pk left of p-1 -> pi	
	// p-2 left of pi->pk if pk left of p-2 -> pi
	else return left_of_symbolic(pk, pj, pi);
}

int right_of(Point *p, Edge *e) 
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return left_of_symbolic(p, e->Dest, e->Org);
	else return ccw(e->Dest, e->Org, p) == 1;
}
int left_of(Point *p, Edge *e)
{
	if (symbolic(p) || symbolic(e->Org) || symbolic(e->Dest)) return left_of_symbolic(p, e->Org, e->Dest);
	else return ccw(e->Org, e->Dest, p) == -1;
}

Edge *connect(Edge *a, Edge *b) 
{
	Edge *e = make_edge();
	set_equal(e->Org, a->Dest);
	set_equal(e->Dest, b->Org);
	splice(e, lnext(a));
	splice(sym(e), b);
	return e;
}

Edge *locate(Point *x, Edge *e)
{
	do {
		if (on(x, e)) return e;
		else if (right_of(x, e)) e = sym(e);
		else if (left_of(x, onext(e))) e = onext(e);
		else if (left_of(x, dprev(e))) e = dprev(e);
		else {
			if (on(x, onext(e))) return onext(e);
			else if (on(x, dprev(e))) return dprev(e);
			else return e;
		}
	} while(1);
}
int min(int i, int j)
{
	if (i <= j) return i; 
	else return j;
}
int legal(Point *pi, Point *pj, Point *pk, Point *pl)
{
	// check if edge pi-pj on bounding triangle (always lega)
	if ((pi->id <= 0) && (pj->id <=0)) return 1;
	// if all points real use predicate
	else if (real(pi) && real(pj) && real(pk) && real(pl)) 
		return in_circle(pi, pk, pj, pl);
	// See Dutch book p. 204
	else return min(pk->id, pl->id) < min(pi->id, pj->id);  
}
// return 1 if point x is a duplicate
int insert_site(Point *x, Edge *existing)
{
	printf("locating point\n");
	Edge *e = locate(x, existing);
	if (!lexico_comp(x, e->Org) || !lexico_comp(x, e->Dest)) return 0;
	else if (on(x, e)) {
		printf("point on existing edge\n");
		Edge *t = oprev(e);
		delete_edge(e);
		e = t;
	}
	Edge *base = make_edge();
	Point *first = e->Org;
	set_equal(base->Org, first);
	set_equal(base->Dest, x);
	splice(base, e);
	do {
		printf("connecting point to surrounding points\n");
		base = connect(e, sym(base));
		e = oprev(base);
	} while (!equal(e->Dest, first));
	e = oprev(base);
	do {
//		printf("flipping illegal edges\n");
		printf("e->Org id: %d\n", e->Org->id);
		printf("first id: %d\n", first->id);
		Edge *t = oprev(e);
		if (right_of(t->Dest, e) && !legal(e->Org, e->Dest, t->Dest, x)) {
			printf("swapping\n");
			swap(e); 
			e = oprev(e);
		} else if (equal(e->Org, first)) return 0;
		else e = lprev(onext(e));
	} while(1);
}
//read line from input file-inspiration from Triangle/triangle_io.c
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
//find the next field-copied from Triangle/triangle_io.c
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
// read vertices from a .node file. Inspiration from Triangle/triangle_io.c
int file_readnodes(FILE *file, int *firstnode, read_io *io)
{
	char inputline[INPUTLINESIZE];
	char *stringptr;
	int mesh_dim;
	int invertices;
	int i;
	double x, y;
	if (file == (FILE *) NULL) {
		return 1;
	}
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
	//Read the vertices
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
		printf("Reading point: %d, %f, %f\n", i, x, y);
		pointlist[2*i+0] = x;
		pointlist[2*i+1] = y;
	
	}
	io->num_points = invertices;
	io->point_list = pointlist;
	return 0;
}

//read command line arguments

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
				input_args->in_filename =optarg;
				break;
			case 'o':
				input_args->out_filename = optarg;
				break;
			case '?':
				if (optopt == 'i')
					fprintf(stderr, "Please specify input filename\n");
				else if (isprint (optopt))
					fprintf(stderr, "Unknown option '-%c'\n",optopt);
				else 
					fprintf(stderr, "Unknown option character '\\x%x'\n", optopt);
				return 1;
			default:
				abort();
		}
	}
	if (!input_args->in_filename) {
		fprintf(stderr, "Please specify input filename\n");
		return 1;
 	}	
	if (!input_args->out_filename) {
		input_args->out_filename = "output.ele";
	}
	printf("randomized insertion = %d\n", input_args->randomized);
	printf("fast point location = %d\n", input_args->fast);
	printf("input filename = %s\n", input_args->in_filename);
	printf("output_filename = %s\n", input_args->out_filename);
	for(index = optind; index < argc; index++){
		printf("extra argument: %s\n", argv[index]);
	}
	return 0;
} 

Edge *delaunay(int random, int fast, read_io *io)
{
	int i, dup;
	double x,y;
	Point max = {io->point_list[0], io->point_list[1], 0};
	int index = 0;
	// find lexico. max input point
	for (i = 1; i < io->num_points; i++) {
		x = io->point_list[2*i]; 
		y = io->point_list[2*i+1];
		if ((x > max.x) || ((x == max.x) && (y > max.y))) {
			max.x = x;
			max.y = y;
			index = i;
		}
	
	}
	Point neg1 = {0.0,0.0,-1};
	Point neg2 = {0.0,0.0,-2};
	// form bounding triangle using two symbolic points as in the Dutch book
	printf("first make_edge\n");
	Edge *a = make_edge();
	Edge *b = make_edge();
	printf("first splice\n");
	splice(sym(a), b);
	set_equal(a->Org, &neg1); 
	set_equal(a->Dest, &max);
	set_equal(b->Org, &max);
	set_equal(b->Dest, &neg2);
	printf("first connect\n");
	connect(b, a); 
	// insert points
	int num_dups = 0;
	for (i = 0; i < io->num_points; i++) {
		if (i == index) continue;
		else {
			x = io->point_list[2*i];
			y = io->point_list[2*i+1];
			printf("Inserting point %d %f %f\n", i+1, x, y);
			Point p = {x, y, i+1};
			dup = insert_site(&p, a);
			if (dup) num_dups++;
		}
	}
	io->num_points -= num_dups;
	printf("done inserting points\n");
	return a;
}
int file_writenodes(FILE *file, Edge *e, read_io *io)
{
	printf("writing output file\n");
	if (file == (FILE *) NULL) {
		return 1;
	}
	// header
	//TODO: this is wrong. need number of triangles
	fprintf(file, "%d 3 0\n", io->num_points);
	node_t *q = NULL;
//	printf("enqueueing\n");
	enqueue(&q, e);
	Point *p1; Point *p2; Point *p3;
	// traverse all vertices in dual of quadedge data structure
	int i = 1;
	while (q) {
//		printf("dequeueing\n");
		e = dequeue(&q);
//		printf("dequeued edge %d -> %d\n", e->Org->id, e->Dest->id);
		if (e->trav == 2) continue; // left face already processed
		else {	
			e->trav = 2; lnext(e)->trav = 2; lprev(e)->trav = 2;
			if (!(sym(e)->trav)) {
				enqueue(&q, sym(e));
				sym(e)->trav = 1;
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
				//TODO: make sure node numbers correcti
				printf("Triangle %d %d %d %d\n", i, p1->id, p2->id, p3->id);
				fprintf(file, "%d %d %d %d\n", i, p1->id, p2->id, p3->id); 
				i++;	
			
				// add all symmetric edges to the queue
				// if edge has already been added, skip it
			}	// if both faces of the edge have been written, delete the edge
		}
						
		if (sym(e)->trav == 2) delete_edge(e);
		if (sym(lnext(e))->trav == 2) delete_edge(lnext(e));
		if (sym(lprev(e))->trav == 2) delete_edge(lprev(e));
	}
	return 0;
}

int main(int argc, char *argv[])
{
	exactinit();
	command_line input_args = {0,0,NULL,NULL};
	printf("reading command line\n");
	int error = read_input(argc, argv, &input_args);
	if (error) return 1;
	printf("done reading command line\n");
	FILE *file;
	file = fopen(input_args.in_filename, "r");
	if (file == (FILE *) NULL) return 1;
	int first_node;
	read_io io;
	printf("reading input file\n");
	error = file_readnodes(file, &first_node, &io);
	fclose(file);
	if (error) {
		return 1;
	}
	printf("constructing triangulation\n");
	Edge *e = delaunay(input_args.randomized, input_args.fast, &io);
	file = fopen(input_args.out_filename, "w");
	error = file_writenodes(file, e, &io);
	fclose(file);
	if (error) {
		//TODO: free all 
		return 1;
	}
	free(io.point_list);
	return 0;
}

