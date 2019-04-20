#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "predicates.h"

//Data structures//

struct Point {
	double x;
	double y;
};

typedef struct Point Point; 

typedef struct {
	int randomized;
	int fast;
	char *in_filename;
	char *out_filename;
} Input;

typedef struct Edge Edge;
typedef struct QEdge QEdge;

struct Edge {
	Point *Org;
	Point *Dest;
	Edge *Next;
	QEdge *Parent;
	unsigned int r;
};

struct QEdge {
	Edge *edge[4];
};

//Basic edge operations//

Edge *rot(Edge *e) 
	{return e->Parent->edge[(e->r + 1)%4];}
Edge *onext(Edge *e) 
	{return e->Next;}
Edge *sym(Edge *e) 
	{return e->Parent->edge[(e->r + 2)%4];}
Edge *rot_inv(Edge *e) 
	{return e->Parent->edge[(e->r + 3)%4];}
Edge *oprev(Edge *e)
	{return rot(onext(rot(e)));}
Edge *lprev(Edge *e)
	{return sym(onext(e));}
Edge *lnext(Edge *e) 
	{return rot_inv(onext(rot(e)));}
//TODO: Make this accept input points for edge
Edge *make_edge()
{
	QEdge *quad = malloc(sizeof(QEdge));
 	for (int i = 0; i < 4; i++) {
		Edge *e = malloc(sizeof(Edge));
		quad->edge[i] = e;
		e->Parent = quad;
	}
	quad->edge[0]->Next = quad->edge[2];
	quad->edge[2]->Next = quad->edge[0];
	quad->edge[1]->Next = quad->edge[3];
	quad->edge[3]->Next = quad->edge[1];
	return quad->edge[0];	
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
	rot(a)->Next = beta; rot(b)->Next = alpha;
	rot(alpha)->Next = b; rot(beta)->Next = a;
}

void delete_edge(Edge *e) 
{
	splice(e, oprev(e));
	splice(sym(e), oprev(sym(e)));
	free(e->Org); free(e->Dest);
	QEdge *parent = e->Parent;
	for (int i = 0; i < 4; i++) free(parent->edge[i]);
	free(parent);
}

void swap(Edge *e) 
{
	Edge *a = oprev(e);
	Edge *b = oprev(sym(e));
	splice(e, a); splice(sym(e), b);
	splice(e, lnext(e)); splice(sym(e), lnext(b));
	e->Org = a->Dest; e->Dest = b->Dest;
}

//TODO: figure out if this is ok fp arithmetic
int equal(Point *p1, Point *p2) 
{
	return (p1->x==p2->x) && (p1->y==p2->y);
}
// p = alpha*org + (1-alpha)*dest <-> alpha = (p - dest)/(org - dest)
int on(Point *p, Edge *e)
{
	double point[2] = {p->x, p->y};
	double p1[2] = {e->Org->x, e->Org->y};
	double p2[2] = {e->Dest->x, e->Dest->y};
	double alpha = (point[0] - p1[0])/(p2[0] - p1[0]);
	return (orient2d(point, p1, p2) == 0) && (alpha >= 0) && (alpha <= 1);
}


void insert_site(Point *x)
{
	Edge *e = locate(x);
	if (equals(x, e->Org) || equals(x, e->Dest)) return;
	else if (on(x, e)) {
		Edge *t = oprev(e);
		delete_edge(e);
		e = t;
	}
	Edge *base = make_edge();
	Point *first = e->Org;
	base->Org = first;
	base->Dest = x;
	splice(base, e);
	while (!equal(e->Dest, first)) {
		base = connect(e, sym(base));
		e = oprev(base);
	}
	e = oprev(base);
	while(1) {
		t = oprev(e);
		if (right_of(t->Dest, e) && incircle(e->Org, t->Dest, e->Dest, x))
			swap(e); 
			e = t;
		else if (equals(e->Org), first)
			return;
		else e = lprev(onext(e));
	}
}
//TODO: right_of, make sure memory freed, connect, locate

//read command line arguments

int read_input(int argc, char *argv[], Input *input_args)
{
	int opt;
	opterr = 0;

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
	printf("randomized insertion = %d\nfast point location = %d\ninput filename = %s\noutput_filename = %s\n", input_args->randomized, input_args->fast, input_args->in_filename, input_args->out_filename);
 
	for(; optind < argc; optind++){
		printf("extra argument: %s\n", argv[optind]);
	}
	return 0;
} 

//read input file (.node)

int read_node(char *filename) 
{
	FILE *file;
	file = fopen(filename, "r");
	return 0;
}

int main(int argc, char *argv[])
{
	exactinit();
	Input input_args = {0,0,NULL,NULL};
	int error = read_input(argc, argv, &input_args);
	if (error) return 1;
	return 0;
}

