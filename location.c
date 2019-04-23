#include "triangle.h"
#include <stdlib.h>
#include <stdio.h>
int compare(Point *p, Node *n) // -1 if p left, 0 if p on, 1 if p right
{
	if (n->type == 0) { // n is an edge (internal node of the DAG) 
		if (ccw(n->Dest, n->Org, p) > 0) {
			return 1;
		} else return -1; // if p lies on n's edge, go left
	}
	else return 0; // search reached a leaf node (time to stop)

}

Node *search(Point *p, Node *n) 
{
	while(1) {
		if (compare(p, n) < 0) {
			n = n->left;
		} else if (compare(p, n) > 0) {
			n = n->right;
		} else return n;
	}
}
Node *init_edge(Edge *e)
{
	Node *n = malloc(sizeof(Node));
	n->type = 0; // edge node
	n->edge = NULL; // won't use this
	n->Org = malloc(sizeof(Point)); n->Dest = malloc(sizeof(Point)); // create copies in case e is destroyed
	set_equal(n->Org, e->Org); set_equal(n->Dest, e->Dest);
	n->pointers = NULL; // won't use this
	n->num_pointers = 0; // won't use this
	return n;
}
// create new face node that lies to the left of e
Node *init_face(Edge *e)
{
	Node *n = malloc(sizeof(Node));
	n->type = 1; // face node
	n->left = NULL; n->right = NULL; // won't use this
	n->edge = e; 
	n->Org = NULL; n->Dest = NULL; // won't use these
	return n;
}

// initialize DAG with the first (outer bounding) triangle. Assume the universe lies to the left of e
void initialize(Edge *a, Node **root)
{
	*root = init_face(a); // initialize face node
	(*root)->pointers = malloc(sizeof(Node***));
	((*root)->pointers)[0]  = root; // only one pointer points to the new node
	(*root)->num_pointers = 1;
	set_left_face(a, *root); // set pointers from all edges in the triangulation 
				// for which this is their left face
}

Edge *find(Point *p, Node *n)
{
	Edge *found = search(p, n)->edge;
	// must check whether p is on any edge of the face 
	if (on(p, lnext(found))) found = lnext(found);
	else if (on(p, lprev(found))) found = lprev(found);
	return found;
}


// Assume p lies on edge e.
// Delete the edge and form edges from p to all vertices
// of the new face.
//         e2                      e2
//       <-----                  <------
//      |^     ^                 |\  ep/^
//      | \ D2 |                 | \e / |
//   e1 |  \e  |e4   ---->    e1 |  \/  |e4
//      |   \  |                 |  /\e |
//      | D1 \ |                 | /  \ |
//      |     \|                 |/epp \|
//	------>                  L------>
//         e3                       e3
//
Edge *extend_on(Point *p, Edge *e)
{
	printf("Extending point on edge\n");
	// need this to pass into the method in triangle.c
	Edge *t = oprev(e);

	Node *D1 = left_face(e);
	Node *D2 = left_face(sym(e));

	Edge *e1 = lnext(e);
	Edge *e3 = lprev(e);
	Edge *e4 = lnext(sym(e));
	Edge *e2 = lprev(sym(e));

	// do updates in triangulation
	// keep edge "base" to give back to insert_site method
	Edge *base = prepare_on(p, e, t);
	
	// create 4 new faces
	Node *D1p = init_face(e1);	
	Node *D2p = init_face(e2);
	Node *D3p = init_face(e3);
	Node *D4p = init_face(e4);
	
	// create 2 new edges for comparison 
	// (we will keep e in the DAG even though it no longer exists)
	Node *ep = init_edge(lnext(e1));
	Node *epp = init_edge(lprev(e2));
	
	// set pointers for ep
	ep->right = D3p; 
	D3p->num_pointers = 1;
	D3p->pointers = malloc(sizeof(Node***));
	(D3p->pointers)[0] = &(ep->right);

	ep->left = D1p;
	D1p->num_pointers = 1;
	D1p->pointers = malloc(sizeof(Node***));
	(D1p->pointers)[0] = &(ep->left);

	set_left_face(e1, D1p); set_left_face(e3, D3p);		
	int i;
	for (i = 0; i < D1->num_pointers; i++) *(D1->pointers[i]) = ep;
	
	// do same for epp
	epp->right = D4p;
	D4p->num_pointers = 1;
	D4p->pointers = malloc(sizeof(Node***));
	(D4p->pointers)[0] = &(epp->right);
	
	epp->left = D2p;
	D2p->num_pointers = 1;
	D2p->pointers = malloc(sizeof(Node***));
	(D2p->pointers)[0] = &(epp->left);

	set_left_face(e2, D2p); set_left_face(e4, D4p);
	for (i = 0; i < D2->num_pointers; i++) *(D2->pointers[i]) = epp;
	
	// free old faces
	free(D1->pointers); free(D1);
	free(D2->pointers); free(D2);
	
	return base;		
}

// Assume D is a face and p lies strictly inside D. 
// Form edges from p to all vertices of D.
// 
//  
//            /^
//           /  \
//          /    \
//       e3/      \e2
//        /        \
//       L          \
//       ----------->
//            e
//
Edge *extend_in(Point *x, Edge *e)
{
	printf("Extending point inside face\n");
	Edge *e1 = e;
	Edge *e2 = lnext(e1);
	Edge *e3 = lprev(e1);
	Node *D = left_face(e);
	
	// do updates in triangulation
	// keep base to give back to insert_site
	printf("Updating triangulation\n");
	Edge *base = prepare_in(x, e);
	
	// create 3 new faces
	Node *D1 = init_face(e1);
	Node *D2 = init_face(e2);
	Node *D3 = init_face(e3);

	// create 3 new comparison edges
	Node *f1 = init_edge(lnext(e1));
	Node *f2 = init_edge(lnext(e2));
	Node *f3 = init_edge(lnext(e3));

	// set pointers
	f1->left = f3;
	f3->left = D3;
	f3->right = D1;
	
	f1->right = f2;
	f2->left = D2;
	f2->right = D3;

	D1->pointers = malloc(sizeof(Node***));
	(D1->pointers)[0] = &(f3->right);
	D1->num_pointers = 1;

	D2->pointers = malloc(sizeof(Node***));
	(D2->pointers)[0] = &(f2->left);
	D2->num_pointers = 1;

	D3->pointers = malloc(2*sizeof(Node***));
	(D3->pointers)[0] = &(f2->right);
	(D3->pointers)[1] = &(f3->left);
	D3->num_pointers = 2;
		
	int i;
	for (i = 0; i < D->num_pointers; i++) *((D->pointers)[i]) = f1;
	
	// set pointers from appropriate edges in triangulation
	set_left_face(e1, D1); set_left_face(e2, D2); set_left_face(e3, D3);
	
	// free old face
	free(D->pointers);
	free(D);
	
	return base;	
	
}

//                 
//       <-----     
//      |^     ^     
//      | \ D2 |      
//   e1 |  \e  |e2   
//      |   \  |        
//      | D1 \ |         
//      L     \|
//	------>  
//                
//
void swap_location(Edge *e)
{
	printf("Swapping edge %d -> %d\n", e->Org->id, e->Dest->id);	
	
	Edge *e1 = lnext(e);
	Edge *e2 = lnext(sym(e));
	Node *D1 = left_face(e);
	Node *D2 = left_face(sym(e));
	
	// swap e in triangulation
	swap(e);
	
	// create 2 new faces
	Node *D1p = init_face(e1);
	Node *D2p = init_face(e2);

	// create 1 new comparison edge (for the new swapped e)
	Node *ep = init_edge(lnext(e1));

	// set pointers
	ep->left = D1p; ep->right = D2p;

	D1p->pointers = malloc(sizeof(Node***));
	(D1p->pointers)[0] = &(ep->left);
	D1p->num_pointers = 1;

	D2p->pointers = malloc(sizeof(Node***));
	(D2p->pointers)[0] = &(ep->right);
	D2p->num_pointers = 1;

	set_left_face(e1, D1p);
	set_left_face(e2, D2p);
	int i;
	for (i = 0; i < D1->num_pointers; i++) *((D1->pointers)[i]) = ep;
	for (i = 0; i < D2->num_pointers; i++) *((D2->pointers)[i]) = ep;
	
	// free old faces
	free(D1->pointers); free(D1); 
	free(D2->pointers); free(D2);
	printf("Done swapping\n");
}

void free_nodes(Node **root)
{
	if (root == NULL) return;
	else if (*root == NULL) return;
	else if ((*root)->type == 0) {
		printf("edge node %d -> %d\n", (*root)->Org->id, (*root)->Dest->id);
		printf("freeing org\n");
		free((*root)->Org);
		printf("freeint dest\n");
		free((*root)->Dest);
		free_nodes(&((*root)->left));
		free_nodes(&((*root)->right));
	} else {
		
		printf("freeing pointers\n");
		free((*root)->pointers);
	}
	printf("freeing node\n");
	free(*root);
	*root = NULL;
}
