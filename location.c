#include "triangle.h"


int leaf(Node *n) {
	return n->type;
}

int compare(Point *p, Node *n) // -1 if p left, 0 if p on, 1 if p right
{
	if (leaf(n)) {
		//printf("leaf\n");
		return 0; // search reached leaf node, time to stop
	} else { // n is an edge (internal node of the DAG) 
		//printf("not leaf\n");
		if (ccw(n->Dest, n->Org, p) > 0) {
			return 1;
		} else return -1; // if p lies on n's edge, go left
	}

}

Node *search(Point *p, Node *n) 
{
	while(1) {
		if (compare(p, n) < 0) {
			n = n->left;
		} else if (compare(p, n) > 0) {
			n = n->right;
		} else {
			return n;
		}
	}
}
Node *init_edge(Edge *e)
{
//	printf("Creating edge node %d -> %d\n", e->Org->id, e->Dest->id);
	Node *n = malloc(sizeof(Node));
	n->type = 0; // edge node
	n->edge = NULL; // won't use this
	n->Org = malloc(sizeof(Point)); n->Dest = malloc(sizeof(Point)); // create copies in case e is destroyed
	set_equal(n->Org, e->Org); set_equal(n->Dest, e->Dest);
	//n->pointers = NULL; // won't use this
	//n->num_pointers = 0; // won't use this
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
/*Node *new_node(Edge *a) {
	Node *n = malloc(sizeof(Node));
	n->left = NULL; n->right = NULL;
	n->edge = a;
	return n;
}
*/
Node *left_face(Edge *e) { return e->face; }

// update pointers to DAG for all edges that share e's left face
void set_left_face(Edge *e, Node *f)
{
	Edge *next = lnext(e);
	e->face = f;
	while (next != e) {
		next->face = f;
		next = lnext(next);
	}
}

// initialize DAG with the first (outer bounding) triangle. Assume the universe lies to the left of e
void initialize(Edge *a, Node **root)
{
	*root = init_face(a); // initialize face node
	set_left_face(a, *root); // set pointers from all edges in the triangulation 
				// for which this is their left face
}
Node *repurpose(Node *n, Edge *e) {
	n->type = 0;
	n->edge = NULL;
	n->Org = malloc(sizeof(Point)); n->Dest = malloc(sizeof(Point));
	set_equal(n->Org, e->Org); set_equal(n->Dest, e->Dest);
	return n;
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
	
	// alias D1 as new comparison node ep
	Node *ep = repurpose(D1, lnext(e1));
	
	// set pointers for ep
	ep->right = D3p; 

	ep->left = D1p;

	set_left_face(e1, D1p); set_left_face(e3, D3p);		
	
	// do same for epp
	Node *epp = repurpose(D2, lprev(e2));

	epp->right = D4p;
	
	epp->left = D2p;

	set_left_face(e2, D2p); set_left_face(e4, D4p);
	
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
	//printf("Extending point inside face\n");
	Edge *e1 = e;
	Edge *e2 = lnext(e1);
	Edge *e3 = lprev(e1);
	Node *D = left_face(e);
	
	// do updates in triangulation
	// keep base to give back to insert_site
	Edge *base = prepare_in(x, e);
	
	// create 3 new faces
	Node *D1 = init_face(e1);
	Node *D2 = init_face(e2);
	Node *D3 = init_face(e3);
	
	// alias old face as new comparison edge
	Node *f1 = repurpose(D, lnext(e1));

	// create 2 new comparison edges
	Node *f2 = init_edge(lnext(e2));
	Node *f3 = init_edge(lnext(e3));

	// set pointers
	f1->left = f3;
	f3->left = D3;
	f3->right = D1;
	
	f1->right = f2;
	f2->left = D2;
	f2->right = D3;
	
	// set pointers from appropriate edges in triangulation
	set_left_face(e1, D1); set_left_face(e2, D2); set_left_face(e3, D3);
	
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
	//printf("Swapping edge %d -> %d\n", e->Org->id, e->Dest->id);	
	
	Edge *e1 = lnext(e);
	Edge *e2 = lnext(sym(e));
	Node *D1 = left_face(e);
	Node *D2 = left_face(sym(e));
	
	// swap e in triangulation
	swap(e);
	
	// create 2 new faces
	Node *D1p = init_face(e1);
	Node *D2p = init_face(e2);
	
	// D1 and D2 must now be comparison edges for new swapped e
	Node *ep = repurpose(D1, lnext(e1));
	Node *epp = repurpose(D2, lnext(e1));

	// set pointers
	ep->left = D1p; ep->right = D2p;
	epp->left = D1p; epp->right = D2p;

	set_left_face(e1, D1p);
	set_left_face(e2, D2p);
}
