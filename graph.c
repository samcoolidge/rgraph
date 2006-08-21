#include <math.h>
#include <string.h>
#include <search.h>
#include "prng.h"
#include "tools.h"

#include "graph.h"


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Node, link, and graph creation and memory allocation
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// A graph is a collection of nodes. This function creates an empty
// node to be placed at the top of the list.
// ---------------------------------------------------------------------
struct node_gra *CreateHeaderGraph()
{
  struct node_gra *temp;

  temp = (struct node_gra *)calloc(1, sizeof(struct node_gra));
  temp->label = NULL;
  temp->num = -1;
  temp->coorX = -1.0;
  temp->coorY = -1.0;
  temp->coorZ = -1.0;
  temp->state = 0;
  temp->neig = NULL;
  temp->next = NULL;
  temp->n_pack = -1;
  temp->inGroup = -1;
  temp->dvar1 = -1.;

  return temp;
}

// ---------------------------------------------------------------------
// Create a node and add it at the end of the list that starts at p
// ---------------------------------------------------------------------
struct node_gra *CreateNodeGraph(struct node_gra *p,
				 char *label)
{
  char *label_str;

  while (p->next != NULL)
    p = p->next;
      
  p->next = (struct node_gra *) calloc(1, sizeof(struct node_gra));
  label_str = (char *) calloc(MAX_LABEL_LENGTH, sizeof(char));
  (p->next)->label = strcpy(label_str, label);
  (p->next)->num = p->num + 1;
  (p->next)->state = 0;
  (p->next)->next = NULL;
  (p->next)->neig =
    (struct node_lis *) calloc(1, sizeof(struct node_lis));
  ((p->next)->neig)->node = -1;
  ((p->next)->neig)->next = NULL;
  (p->next)->n_pack = 0;
  (p->next)->inGroup = -1;
  (p->next)->coorX = -1.0;
  (p->next)->coorY = -1.0;
  (p->next)->coorZ = -1.0;
  (p->next)->dvar1 = -1.0;

  return p->next;
}

// ---------------------------------------------------------------------
// The BFS list is a collection of node_bfs, used for breadth first
// searches. This function creates an empty node_bfs to be placed at
// the top of the list.
// ---------------------------------------------------------------------
struct node_bfs *CreateHeaderList()
{
  struct node_bfs *temp;

  temp = (struct node_bfs *) calloc(1, sizeof(struct node_bfs));
  temp->d = -1;
  temp->ref = NULL;
  temp->next = NULL;
  temp->prev = NULL;
  temp->last = NULL;
  temp->pred = NULL;

  return temp;
}

// ---------------------------------------------------------------------
// Create an empty node for the tree data structure
// ---------------------------------------------------------------------
struct node_tree *CreateNodeTree()
{
  struct node_tree *temp=NULL;
  temp = (struct node_tree *) calloc(1, sizeof(struct node_tree));
  temp->ref = NULL;

  return temp;
}

// ---------------------------------------------------------------------
// Add node2 to the list of neighbors of node1: auto_link_sw=0
// prevents from self links being created; add_weight_sw!=0 enables
// the weight of the adjacency to increase if the adjacency already
// exists.
// ---------------------------------------------------------------------
int AddAdjacency(struct node_gra *node1,
		 struct node_gra *node2,
		 int auto_link_sw,
		 int add_weight_sw,
		 double weight,
		 int status)
{
  struct node_lis *adja=NULL;

  // If node1==node2 and autolinks are not allowed, do nothing...
  if (node1 == node2 && auto_link_sw == 0) { 
    return 0;
  }
  // ...otherwise go ahead and try to create the link
  else {
    adja = node1->neig;
    while (adja->next != NULL) {
      adja = adja->next;
      if (adja->ref == node2) {
	// The link already exists
	if (add_weight_sw != 0) {
	  adja->weight += weight; // Update the weight of the link
	  return 1;
	}
	else {
	  return 0;            // Do nothing
	}
      }
    }
    
    // Create a new adjacency
    adja->next = (struct node_lis *)calloc(1, sizeof(struct node_lis));
    (adja->next)->node = node2->num;
    (adja->next)->status = status;
    (adja->next)->next = NULL;
    (adja->next)->ref = node2;
    (adja->next)->btw = 0.0;
    (adja->next)->weight = weight;

    // Done
    return 1;
  }
}

// ---------------------------------------------------------------------
// Same as AddAdjacency, but we only pass the ID of node2, and the ref
// pointer of the new adjacency is set to NULL. RewireAdjacency needs
// to be run when AddAdjacencySoft is used
// ---------------------------------------------------------------------
int AddAdjacencySoft(struct node_gra *node1,
		     int node2_num,
		     int auto_link_sw,
		     int add_weight_sw,
		     double weight,
		     int status)
{
  struct node_lis *adja=NULL;

  // If node1==node2 and autolinks are not allowed, do nothing...
  if (node1->num == node2_num && auto_link_sw == 0) { 
    return 0;
  }
  // ...otherwise go ahead and try to create the adjacency
  else {
    adja = node1->neig;
    while((adja = adja->next) != NULL) {
      if (adja->node == node2_num) {
	// The link already exists
	if (add_weight_sw != 0) {
	  adja->weight += weight; // Update the weight of the link
	  return 1;
	}
	else {
	  return 0;            // Do nothing
	}
      }
    }
    
    // Create a new adjacency
    adja->next = (struct node_lis *) calloc(1, sizeof(struct node_lis));
    (adja->next)->node = node2_num;
    (adja->next)->status = status;
    (adja->next)->next = NULL;
    (adja->next)->ref = NULL;
    (adja->next)->btw = 0.0;
    (adja->next)->weight = weight;
    
    // Done
    return 1;
  }
}

// ---------------------------------------------------------------------
// Sets the ref pointers of all the adjacencies to the corresponding
// nodes. You MUST use this after using AddAdjacencySoft!
// ---------------------------------------------------------------------
void RewireAdjacency(struct node_gra *net)
{
  struct node_gra *p=NULL;
  struct node_lis *adja=NULL;
  struct node_gra **nlist;
  int nnod = CountNodes(net);

  // Map the nodes into an array for faster access
  nlist = (struct node_gra **) calloc(nnod, sizeof(struct node_gra *));
  p = net;
  while ((p = p->next) != NULL) {
    nlist[p->num] = p;
  }

  // Point the adjacency pointers to the nodes
  p = net;
  while ((p = p->next) != NULL) {
    adja = p->neig;
    while ((adja = adja->next) != NULL) {
      adja->ref = nlist[adja->node];
    }
  }

  // Done
  free(nlist);
  return;
}

// ---------------------------------------------------------------------
// Takes the adjacency list of the origin node nori and copies it to
// the destination node ndes. ONLY soft links are created!!
// ---------------------------------------------------------------------
void CopyAdjacencyList(struct node_gra *nori, struct node_gra *ndes)
{
  struct node_lis *pori=NULL;

  pori = nori->neig;
  while ((pori = pori->next) != NULL) {
    AddAdjacencySoft(ndes, pori->node, 1, 0,
		     pori->weight, pori->status);
  }
}

// ---------------------------------------------------------------------
// Generates a copy of a given network.
// ---------------------------------------------------------------------
struct node_gra *CopyNetwork(struct node_gra *p1)
{
  struct node_gra *root2=NULL, *p2=NULL;
  struct node_gra *last=NULL;

  // Copy the nodes and their adjacency lists
  root2 = CreateHeaderGraph();
  last = root2;
  while ((p1 = p1->next) !=  NULL) {
    p2 = CreateNodeGraph(last, p1->label);
    p2->coorX = p1->coorX;
    p2->coorY = p1->coorY;
    p2->coorZ = p1->coorZ;
    p2->state = p1->state;
    p2->n_pack = p1->n_pack;
    p2->inGroup = p1->inGroup;
    p2->dvar1 = p1->dvar1;
    last = p2;
    CopyAdjacencyList(p1, p2);
  }

  // Rewire the network to make all soft links hard
  RewireAdjacency(root2);

  // Done
  return root2;
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Node, link, and graph removal
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Frees the memory allocated to a node_tree (needed by
// tdestroy). VISIT value and int level are not used but are required
// by tdestroy.
// ---------------------------------------------------------------------
void FreeNodeTree(struct node_tree *ntree, VISIT value, int level)
{
  free(ntree);
  return;
}

// ---------------------------------------------------------------------
// Recursively removes all the adjacencies in the adjacency list that
// hangs from p and frees the memory
// ---------------------------------------------------------------------
void FreeAdjacencyList(struct node_lis *p)
{
  if (p->next != NULL) {
    FreeAdjacencyList(p->next);
  }
  free(p);
}

// ---------------------------------------------------------------------
// Frees the memory allocated to a node_gra
// ---------------------------------------------------------------------
void FreeNode(struct node_gra *node)
{
  if (node->neig != NULL) {
    FreeAdjacencyList(node->neig);
  }
  free(node->label);
  free(node);
  return;
}

// ---------------------------------------------------------------------
// Recursively removes all the nodes in a network and frees the memory
// ---------------------------------------------------------------------
void RemoveGraph(struct node_gra *p)
{
  if (p->next != NULL) {
    RemoveGraph(p->next);
  }
  FreeNode(p);
}

// ---------------------------------------------------------------------
// Removes the link between nodes n1 and n2 (and frees the memory). If
// symmetric_sw != 0, RemoveLink will also remove the link from n2 to
// n1. CAUTION: RemoveLink will crash if there is no link between n1
// and n2 (or between n2 and n1 when symmetric_sq != 0).
// ---------------------------------------------------------------------
void RemoveLink(struct node_gra *n1, struct node_gra *n2,
		int symmetric_sw)
{
  struct node_lis *nn1;
  struct node_lis *nn2;
  struct node_lis *temp1;
  struct node_lis *temp2;

  // Link n1-n2
  nn1 = n1->neig;
  while ((nn1->next)->ref != n2) {
    nn1 = nn1->next;
  }
  temp1 = nn1->next;
  nn1->next = temp1->next;
  free(temp1);

  // Link n2-n1
  if (symmetric_sw != 0) {
    nn2 = n2->neig;
    while ((nn2->next)->ref != n1) {
      nn2 = nn2->next;
    }
    temp2 = nn2->next;
    nn2->next = temp2->next;
    free(temp2);
  }
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Network input
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Compares two node_tree (required by tsearch)
// ---------------------------------------------------------------------
int NodeTreeLabelCompare(const void *n1, const void *n2)
{
  return strcmp(((const struct node_tree *) n1)->label,
		((const struct node_tree *) n2)->label);
}

// ---------------------------------------------------------------------
// Builds a network from an imput file, which contains the list of
// links in the network (and, maybe, the weight of each link).
// ---------------------------------------------------------------------
struct node_gra *FBuildNetwork(FILE *inFile,
			       int weight_sw,
			       int auto_link_sw,
			       int add_weight_sw,
			       int symmetric_sw)
{
  struct node_gra *root=NULL, *last_add=NULL;
  struct node_gra *n1=NULL, *n2=NULL;
  char label1[MAX_LABEL_LENGTH], label2[MAX_LABEL_LENGTH];
  void *node_dict=NULL;
  struct node_tree *n_tree=NULL, *ntree1=NULL, *ntree2=NULL;
  double weight;

  // Create the header of the graph
  root = last_add = CreateHeaderGraph();

  // Go through the input file
  while (!feof(inFile)) {
    // Read the data
    if (weight_sw == 0) {
      fscanf(inFile,"%s %s\n", &label1, &label2);
      weight = 1.;
    }
    else {
      fscanf(inFile,"%s %s %lf\n", &label1, &label2, &weight);
    }
/*     printf("%s %s\n", label1, label2); */
    
    // Check if the nodes already exist, and create them otherwise
    n_tree = CreateNodeTree();
    strcpy(n_tree->label, label1);
    ntree1 = *(struct node_tree **)tsearch((void *)n_tree,
					   &node_dict,
					   NodeTreeLabelCompare);
    if (ntree1->ref == NULL) {
      ntree1->ref = last_add = CreateNodeGraph(last_add, label1);
    }
    else {
      FreeNodeTree(n_tree, preorder, 0);
    }
    n1 = ntree1->ref;
/*     printf("Node: %d\tLabel: %s\n", n1->num, n1->label); */
    
    n_tree = CreateNodeTree();
    strcpy(n_tree->label, label2);
    ntree2 = *(struct node_tree **)tsearch((void *)n_tree,
					   &node_dict,
					   NodeTreeLabelCompare);
    if (ntree2->ref == NULL) {
      ntree2->ref = last_add = CreateNodeGraph(last_add, label2);
    }
    else {
      FreeNodeTree(n_tree, preorder, 0);
    }
    n2 = ntree2->ref;
/*     printf("Node: %d\tLabel: %s\n", n2->num, n2->label); */
    
    // Add the link
    AddAdjacency(n1, n2, auto_link_sw, add_weight_sw, weight, 0);
    if (symmetric_sw != 0 && n1 != n2) {
      AddAdjacency(n2, n1, auto_link_sw, add_weight_sw, weight, 0);
    }
  }

  // Done
  tdestroy(node_dict, FreeNodeTree);
  return root;
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Network printing and output
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Prints the degrees of all the nodes hanging from p
// ---------------------------------------------------------------------
void FPrintDegrees(FILE *file, struct node_gra *p)
{
  while ((p = p->next) !=  NULL)
    fprintf(file, "%s %d\n", p->label, CountLinks(p));
}

// ---------------------------------------------------------------------
// Prints a network as a list of adjacencies (pairs of nodes) with the
// corresponding weight if weight_sw is not 0. If symmetric_sw == 1,
// the link is only printed once (a-b), otherwise it will be printed
// twice if necessary (a-b, and b-a).
// ---------------------------------------------------------------------
void FPrintNetAdjacencyList(FILE *outf,
			    struct node_gra *p,
			    int weight_sw,
			    int symmetric_sw)
{
  struct node_lis *n=NULL;
  int label_cmp;

  while((p = p->next) != NULL) {
    n = p->neig;
    while((n = n->next) != NULL) {
      label_cmp = strcmp(p->label, n->ref->label);
      if (weight_sw == 0) {
	if (symmetric_sw == 0 || label_cmp <= 0) {
	  fprintf(outf, "%s %s\n", p->label, n->ref->label);
	}
      }
      else {
	if (symmetric_sw == 0 || label_cmp <= 0) {
	  fprintf(outf, "%s %s %g\n", 
		  p->label, n->ref->label, n->weight);
	}
      }
    }
  }
}

// ---------------------------------------------------------------------
// Prints a network in Pajek format. Node coordinates are printed if
// coor_sw is not 0. Weight is printed if weight_sw is not 0. If
// symmetric_sw == 1, the link is only printed once (a-b), otherwise
// it will be printed twice if necessary (a-b, and b-a).
// ---------------------------------------------------------------------
void FPrintPajekFile(char *fname,
		     struct node_gra *root,
		     int coor_sw,
		     int weight_sw,
		     int symmetric_sw)

{
  struct node_gra *p=root;
  struct node_lis *n=NULL;
  FILE *outF;

  outF = fopen(fname, "w");
  fprintf(outF, "*Vertices %d\n", CountNodes(root));

  // Print the nodes
  p = root;
  while((p = p->next) != NULL) {
    if (coor_sw == 1) {
      fprintf(outF, "%d \"%s\"           %lf %lf %lf\n",
	      p->num+1, p->label, p->coorX, p->coorY, p->coorZ);
    }
    else {
      fprintf(outF, "%d \"%s\"\n", p->num+1, p->label);
    }
  }
  
  // Print the links title
  if (symmetric_sw == 0) {
    fprintf(outF, "*Arcs\n");
  }
  else {
    fprintf(outF,"*Edges\n");
  }
  
  // Print links
  p = root;
  while((p = p->next) != NULL) {
    n = p->neig;
    while((n = n->next) != NULL) {
      if (weight_sw == 0) {
	if (symmetric_sw == 0 || p->num <= n->ref->num) {
	  fprintf(outF, "%d   %d\n", p->num+1, n->ref->num+1);
	}
      }
      else {
	if (symmetric_sw == 0 || p->num <= n->ref->num) {
	  fprintf(outF, "%d   %d   %g\n", 
		  p->num+1, n->ref->num+1, n->weight);
	}
      }
    }
  }

  // Close the file and return
  fclose(outF);
  return;
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Node, link, and graph operations
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Find and return a given node
// ---------------------------------------------------------------------
struct node_gra *GetNode(int num, struct node_gra *p)
{
  while((p->next)->num != num)
    p = p->next;
  return p->next;
}

// ---------------------------------------------------------------------
// Find and return a given link
// ---------------------------------------------------------------------
struct node_lis *GetLink(struct node_gra *n1, int n2)
{
  struct node_lis *pn = n1->neig;

  while((pn->next)->node !=  n2)
    pn = pn->next;
  return pn->next;
}

// ---------------------------------------------------------------------
// Returns 1 if there is a link between two nodes a and b
// ---------------------------------------------------------------------
int IsThereLink(struct node_gra *n1, struct node_gra *n2)
{
  struct node_lis *n1n = n1->neig;

  while ((n1n = n1n->next) != NULL) {
    if (n1n->ref == n2) {
      return 1;
    }
  }

  return 0;
}

// ---------------------------------------------------------------------
// Returns 1 if there is a link between n1 and a node with number
// n2_num, and 0 otherwise
// ---------------------------------------------------------------------
int IsThereLinkSoft(struct node_gra *n1,int n2_num)
{
  struct node_lis *pn;

  pn = n1->neig;
  while ((pn = pn->next) != NULL) {
    if(pn->node == n2_num)
      return 1;
  }

  return 0;
}

// ---------------------------------------------------------------------
// Removes isolated nodes from the network and returns the number of
// removed nodes. When done removing, nodes are renumbered so that
// they continue having consecutive integers. NEEDS TESTING.
// ---------------------------------------------------------------------
int RemoveIsolatedNodes(struct node_gra *root)
{
  struct node_gra *p=NULL, *temp=NULL;
  struct node_lis *n=NULL;
  int nrem=0;
  int count=0;

  // Revome isolated nodes
  p = root;
  while (p->next !=  NULL) {
    if (CountLinks(p->next) ==  0) {
      temp = p->next;
      p->next = (p->next)->next;
      free(temp);
      nrem++;
    }
    else{
      p = p->next;
    }
  }

  // Renumber the nodes and links in the network
  RenumberNodes(root);

  // Done
  return nrem;
}


// ---------------------------------------------------------------------
// Given a soft-linked network and an array that tells which nodes are
// in the network, remove all links that involve nodes that are not in
// the network.
// ---------------------------------------------------------------------
void CleanAdjacencies(struct node_gra *p, int *nlist)
{
  struct node_lis *nei,*temp;

  while ((p = p->next) != NULL) {
    nei = p->neig;
    while (nei->next != NULL) {
      if (nlist[(nei->next)->node] == 0) {
	temp = nei->next;
	if (temp->next == NULL)
	  nei->next = NULL;
	else
	  nei->next = temp->next;
	free(temp);
      }
      else {
	nei = nei->next;
      }
    }
  }

  return;
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Node_lis operations
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Returns the node_bfs in list that corresponds to the node_gra node
// ---------------------------------------------------------------------
struct node_bfs *GetBFS(struct node_gra *node, struct node_bfs *list)
{
  while (list != NULL) {
    if (list->ref == node) {
      return list;
    }
    else{
      list = list->next;
    }
  }

  return list;
}

// ---------------------------------------------------------------------
// ??????
// ---------------------------------------------------------------------
void AddPredecessor(struct node_bfs *node, struct node_bfs *pred)
{
  struct pred *p = node->pred;
  
  while(p->next != NULL)
    p = p->next;

  p->next = (struct pred *)calloc(1,sizeof(struct pred));
  (p->next)->ref = pred;
  (p->next)->next = NULL;
}

// ---------------------------------------------------------------------
// Recursively remove all the predecessors in a predecessor list
// ---------------------------------------------------------------------
void ClearPredecessors(struct pred *p)
{
  if (p->next != NULL)
    ClearPredecessors(p->next);
  free(p);
}

// ---------------------------------------------------------------------
// Count the number of predecessors of a given node in a BFS list
// ---------------------------------------------------------------------
int CountPredecessors(struct node_bfs *node)
{
  struct pred *p = node->pred;
  int counter = 0;

  if((p->ref)->ref != NULL){  // Do not count the header as a
			      // predecessor
    while (p != NULL) {
      counter++;
      p = p->next;
    }
  }

  return counter;
}

// ---------------------------------------------------------------------
// Add a node_bfs to a list
// ---------------------------------------------------------------------
void Enqueue(struct node_gra *node,
	     struct node_bfs *predecessor,
	     struct node_bfs *header,
	     int *size,
	     int dist)
{
  struct node_bfs *temp;
  
  if(node->state == 0){
    temp = (struct node_bfs *)calloc(1,sizeof(struct node_bfs));
    temp->d = dist;
    temp->next = NULL;
    temp->ref = node;
    temp->last = NULL;
    temp->pred = (struct pred *)calloc(1,sizeof(struct pred));
    (temp->pred)->ref = predecessor;
    (temp->pred)->next = NULL;
    *size += 1;
    node->state = 1;
    // Actualitzem l'apuntador a l'ultim element de la llista
    if(header->last == NULL){
      header->next = header->last = temp;
      temp->prev = header;
    }
    else{
      temp->prev = header->last;
      (header->last)->next = temp;
      header->last = temp;
    }
  }
  else{
    temp = GetBFS(node,predecessor);
    if((temp != NULL)&&(temp->d == dist))
      AddPredecessor(temp,predecessor);
  }
}

// ---------------------------------------------------------------------
// Add all the neighbors of a given node to a BFS list
// ---------------------------------------------------------------------
void EnqueueAdjaList(struct node_bfs *lp,
		     struct node_bfs *list,
		     int *size,
		     int d)
{
  struct node_lis *p=(lp->ref)->neig;

  while ((p = p->next) != NULL)
    Enqueue(p->ref,lp,list,size,d);
}

// ---------------------------------------------------------------------
// ????????
// ---------------------------------------------------------------------
struct node_gra *DequeueOne(struct node_bfs *list,
			    struct node_bfs *one,
			    int *size)
{
  struct node_bfs *temp;
  struct node_bfs *p;
  struct node_gra *node;
  
  temp = one->next;

  if(list->last == temp){
    p = list;
    while(p->next != NULL){
      p->last = one;
      p = p->next;
    }
  }

  if(temp->next == NULL)
    one->next = NULL;
  else
    one->next = temp->next;

  *size -= 1;

  node = temp->ref;

  ClearPredecessors(temp->pred);
  free(temp);

  return node;
}

// ---------------------------------------------------------------------
// ???????
// ---------------------------------------------------------------------
struct node_gra *Dequeue(struct node_bfs *list, int *size)
{
  struct node_bfs *temp;
  struct node_gra *node;

  temp = list->next;
  list->next = temp->next;
  if(list->next == NULL)
    list->last = NULL;
  else
    (list->next)->prev = list;

  *size -= 1;

  node = temp->ref;

  ClearPredecessors(temp->pred);
  free(temp);

  return node;
}

// ---------------------------------------------------------------------
// Update a BFS list with all the nodes at the following distance
// ---------------------------------------------------------------------
struct node_bfs *RenewQueue(struct node_bfs *list,
			    struct node_bfs *lp,
			    int *size,
			    int d)
{
  while ((lp->next != NULL) && ((lp->next)->d == d)) {
    EnqueueAdjaList(lp->next, list, size, d+1);
    lp = lp->next;
  }
  return lp;
}

// ---------------------------------------------------------------------
// Dequeue all the nodes from a BFS list
// ---------------------------------------------------------------------
void ClearList(struct node_bfs *list, int *size)
{
  struct node_gra *temp=NULL;

  while (list->next != NULL) {
    temp = Dequeue(list, size);
  }
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Network resetting
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Set the state of all nodes to 0
// ---------------------------------------------------------------------
void ResetNodesState(struct node_gra *p)
{
  while ((p = p->next) != NULL)
    p->state = 0;
}

// ---------------------------------------------------------------------
// Make the numbers of the nodes consecutive
// ---------------------------------------------------------------------
void RenumberNodes(struct node_gra *net)
{
  struct node_gra *p=NULL;
  struct node_lis *n=NULL;
  int count=0;

  // Renumber the nodes in the network
  p = net;
  while ((p = p->next) !=  NULL) {
    p->num = count++;
  }

  // Renumber the links
  p = net;
  while ((p = p->next) !=  NULL) {
    n = p->neig;
    while ((n = n->next) !=  NULL) {
      n->node = n->ref->num;
    }
  }
}

// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Network randomization
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Randomize the links of a network using the Markov chain switching
// algorithm
// ---------------------------------------------------------------------
struct node_gra *RandomizeSymmetricNetwork(struct node_gra *net,
					   double times,
					   struct prng *gen)
{
  int i;
  int nlink=0;
  int target1=0, target2=0;
  struct node_gra *p=NULL;
  struct node_lis *l=NULL;
  struct node_gra *n1, *n2, *n3, *n4;
  struct node_gra **ori, **des;
  int coun=0;
  int niter=0;

  // Build the link lists (one for link origins and one for ends)
  nlink = TotalNLinks(net, 1);
  niter =  ceil(times * (double)nlink);
  ori = (struct node_gra **)calloc(nlink, sizeof(struct node_gra *));
  des = (struct node_gra **)calloc(nlink, sizeof(struct node_gra *));
  p = net;
  while((p = p->next) !=  NULL){
    l = p->neig;
    while((l = l->next) !=  NULL){
      if (p->num > l->node) {
	ori[coun] = p;
	des[coun] = l->ref;
	coun++;
      }
    }
  }
  if(coun !=  nlink)
    fprintf(stderr, "Error in RandomizeNetwork: coun != nlink!!\n");

  // Randomize the links
  for (i=0; i<niter; i++) {
    // select the 4 nodes different nodes
    do {
      target1 = floor(prng_get_next(gen) * (double)nlink);
      n1 = ori[target1];
      n2 = des[target1];

      do {
	target2 = floor(prng_get_next(gen) * (double)nlink);
	if (prng_get_next(gen) < 0.5) {
	  n3 = des[target2];
	  n4 = ori[target2];
	}
	else {
	  n3 = ori[target2];
	  n4 = des[target2];
	}
      } while (n3 == n1 || n3 == n2 || n4 == n1 || n4 == n2);
    } while (IsThereLinkSoft(n1, n4->num) == 1 ||
	     IsThereLinkSoft(n2, n3->num) == 1);

    printf("%s-%s %s-%s\n",
	   n1->label, n2->label, n3->label, n4->label);

    // switch the link
    RemoveLink(n1, n2, 1);
    RemoveLink(n3, n4, 1);
    AddAdjacency(n1, n4, 0, 0, 1., 1);
    AddAdjacency(n4, n1, 0, 0, 1., 1);
    AddAdjacency(n3, n2, 0, 0, 1., 1);
    AddAdjacency(n2, n3, 0, 0, 1., 1);

    ori[target1] = n1;
    des[target1] = n4;
    ori[target2] = n3;
    des[target2] = n2;
  }

  // Free memory and return the network
  free(ori);
  free(des);
  return net;
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Network properties
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Counts the number of nodes in a network
// ---------------------------------------------------------------------
int CountNodes(struct node_gra *p)
{
  int nodes = 0;
  while ((p = p->next) != NULL)
    nodes++;
  return nodes;
}

// ---------------------------------------------------------------------
// Counts the degree of a node
// ---------------------------------------------------------------------
int CountLinks(struct node_gra *node)
{
  struct node_lis *p=node->neig;
  int count = 0;

  while ((p = p->next) != NULL)
    count++;
  return count;
}

// ---------------------------------------------------------------------
// Computes the average degree of the network
// ---------------------------------------------------------------------
double AverageDegree(struct node_gra *root, int symmetric_sw)
{
  return (double)TotalNLinks(root, symmetric_sw) /
    (double)CountNodes(root);
}

// ---------------------------------------------------------------------
// Counts the number of links in the network. If symmetric_sw != 0,
// that is, if the network is symmetric, the number of links is
// divided by 2.
// ---------------------------------------------------------------------
int TotalNLinks(struct node_gra *p,
		int symmetric_sw)
{
  int total = 0;

  while((p = p->next) != NULL){
    total += CountLinks(p);
  }
  
  if (symmetric_sw == 0)
    return total;
  else
    return total / 2;
}

// ---------------------------------------------------------------------
// Calculates the strength of a node, that is, the sum of the weights
// of all its links
// ---------------------------------------------------------------------
double NodeStrength(struct node_gra *node)
{
  struct node_lis *p=node->neig;
  double count = 0.0;

  while ((p = p->next) != NULL)
    count +=  p->weight;
  return count;
}

// ---------------------------------------------------------------------
// Print to a file the distribution of path lengths between all pairs
// of nodes. NEEDS TESTING.
// ---------------------------------------------------------------------
void FPrintDistanceHistogram(FILE *file, struct node_gra *root)
{
  double histogram[10000];
  int a, d, nodes, *size, i, counter, size_ant;
  struct node_gra *p = root;
  struct node_bfs *list, *lp;
  double av_dis, norm, max_dis=0;
  int **max_dis_pairs;

  max_dis_pairs = allocate_i_mat(CountNodes(root), 2);

  list = CreateHeaderList();

  for(i = 0;i<10000;i++)
    histogram[i] = 0.0;

  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);

  p = root;
  while(p->next != NULL){
    counter++;
    d = 0;
    printf("%d %d %d %d\n",counter,nodes,(p->next)->num,*size);
    ResetNodesState(root);
    p = p->next;

    Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				 //predecessor del primer!!!!!
    lp = list;
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      d++;
      if(*size != size_ant){
	histogram[d] += (double)(*size-size_ant)/(double)nodes;
      }
    }while(*size != size_ant);

    ClearList(list, size);
  }

  // Output the histogram
  av_dis = norm = 0.0;
  for(i = 0;i<10000;i++){
    if(histogram[i] != 0.0){
      fprintf(file,"%d %lf\n",i,histogram[i]/(double)(nodes-1));
      av_dis += (double)(i)*histogram[i]/(double)(nodes-1);
      norm += (double)(histogram[i])/(double)(nodes-1);
    }
  }
  fprintf(file,"#Average distance = %lf\n",av_dis);
  fprintf(file,"#Normalization = %lf\n",norm);
  fprintf(file,"#z = %lf\n",histogram[1]);

  free_i_mat(max_dis_pairs, CountNodes(root));
  free(list);
}

// ---------------------------------------------------------------------
// Print to a file the distribution of path lengths between a node and
// all other nodes. NEEDS TESTING.
// ---------------------------------------------------------------------
void FPrintDistanceHistogramFromNode(FILE *file,
				     struct node_gra *root,
				     int orinode)
{
  double histogram[10000];
  int a,d,nodes,*size,i,counter,size_ant;
  struct node_gra *p = root;
  struct node_bfs *list,*lp;
  double av_dis,norm;

  list = CreateHeaderList();

  for(i = 0;i<10000;i++)
    histogram[i] = 0.0;

  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);

  p = root;
  while(p->next != NULL){
    counter++;
    d = 0;
    p = p->next;

    if(p->num ==  orinode){
      ResetNodesState(root);
      
      Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				   //predecessor del primer!!!!!
      lp = list;
      do{
	size_ant = *size;
	lp = RenewQueue(list,lp,size,d);
	d++;
	if(*size != size_ant){
	  histogram[d] += (double)(*size-size_ant);
	}
      }while(*size != size_ant);

      ClearList(list,size);
    }
  }

  av_dis = norm = 0.0;
  for(i = 0;i<10000;i++){
    if(histogram[i] != 0.0){
/*       fprintf(file,"%d %lf\n",i,histogram[i]); */
      av_dis += (double)(i)*histogram[i]/(double)(nodes-1);
      norm += (double)(histogram[i])/(double)(nodes-1);
    }
  }
  fprintf(file,"#Average distance = %lf\n",av_dis);
/*   fprintf(file,"#Normalization = %lf\n",norm); */
  fprintf(file,"#z = %lf\n",histogram[1]);

  free(list);
}

// ---------------------------------------------------------------------
// Calculates the average path length between nodes in a network
// ---------------------------------------------------------------------
double AveragePathLength(struct node_gra *root)
{
  double histogram[10000];
  int a, d, nodes, *size, i, counter, size_ant;
  struct node_gra *p=root;
  struct node_bfs *list,*lp;
  double av_dis,norm;

  list = CreateHeaderList();

  for(i = 0;i<10000;i++)
    histogram[i] = 0.0;

  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);
  
  p = root;
  while(p->next != NULL){
    counter++;
    d = 0;
    ResetNodesState(root);
    p = p->next;

    Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				 //predecessor del primer!!!!!
    lp = list;
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      d++;
      if(*size != size_ant){
	histogram[d] += (double)(*size-size_ant)/(double)nodes;
      }
    }while(*size != size_ant);

    if(*size != nodes){
      printf("Sorry, the network is not connected!\n");
      ClearList(list,size);
      free(list);
      return -1;
    }

    ClearList(list,size);
  }

  av_dis = norm = 0.0;
  for(i = 0;i<10000;i++){
    if(histogram[i] != 0.0){
      av_dis += (double)(i)*histogram[i]/(double)(nodes-1);
    }
  }

  free(list);

  return av_dis;
}

// ---------------------------------------------------------------------
// Calculates the average of the inverse of the path length between
// nodes in a network
// ---------------------------------------------------------------------
double AverageInverseDistance(struct node_gra *root)
{
  double histogram[10000];
  int a,d,nodes,*size,i,counter,size_ant;
  struct node_gra *p = root;
  struct node_bfs *list,*lp;
  double av_dis,norm;

  list = CreateHeaderList();

  for(i = 0; i<10000; i++)
    histogram[i] = 0.0;

  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);
  
  p = root;
  while(p->next !=  NULL){
    counter++;
    d = 0;
    ResetNodesState(root);
    p = p->next;

    Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				 //predecessor del primer!!!!!
    lp = list;
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      d++;
      if(*size !=  size_ant){
	histogram[d] +=  (double)(*size-size_ant)/(double)nodes;
      }
    }while(*size !=  size_ant);

    ClearList(list,size);
  }

  // Calculate the average inverse distance
  av_dis = norm = 0.0;
  for(i = 0; i<10000; i++){
    if(histogram[i] !=  0.0){
      av_dis +=  histogram[i]/(double)( i * (nodes-1) );
    }
  }

  free(list);

  return av_dis;
}


// ---------------------------------------------------------------------
// ??????????
// ---------------------------------------------------------------------
int SumCommonLinks(struct node_gra *node, struct node_gra *root)
{
  int sum=0;
  struct node_lis *p;

  p = node->neig;
  while ((p = p->next) != NULL) {
    if ((p->ref)->state == 1) {
      sum++;
    }
  }

  return sum;
}

// ---------------------------------------------------------------------
// ??????????
// ---------------------------------------------------------------------
int CalculateLinksBetweenNeig(struct node_bfs *p,
			      struct node_gra *root)
{
  int sum = 0;

  while(p->next != NULL){
    p = p->next;
    sum += SumCommonLinks(p->ref,root);
  }

  return sum;
}

// ---------------------------------------------------------------------
// Calculates the clustering coefficent of a network as the average of
// the nodes individual clustering coefficient
// ---------------------------------------------------------------------
double ClusteringCoefficient(struct node_gra *root)
{
  int nodes,*size,res_size;
  struct node_bfs *list;
  struct node_gra *p = root;
  double C_av;
  int k_v;
  int kk;
  int C_v;

  C_av = 0.0;
  res_size = 0;
  size = &res_size;
  list = CreateHeaderList();
  
  nodes = CountNodes(root);

  while(p->next != NULL){
    p = p->next;
    ResetNodesState(root);
    
    Enqueue(p,list,list,size,0); //Posem el header com a predecessor
				 //pero no afecta res!
    RenewQueue(list,list,size,0);
    Dequeue(list,size);
    k_v = *size;
    if(k_v>1){
      p->state = 0;
      C_v = CalculateLinksBetweenNeig(list,root);
      C_av += (double)C_v/(double)(k_v*(k_v-1));
    }
    else {
      nodes--;
    }
    ClearList(list,size);
  }

  C_av = C_av/(double)nodes;

  free(list);
  return C_av;
}

// ---------------------------------------------------------------------
// Calculates the clustering coefficent of a network as the total
// number of triangles in the network over the maximum number of
// possible triangles.
// ---------------------------------------------------------------------
double ClusteringCoefficient2(struct node_gra *root)
{
  int nodes,*size,res_size;
  struct node_bfs *list;
  struct node_gra *p = root;
  int tri,ctr;  //#triangles and #connected triplets
  int k_v;
  int kk;
  double C;

  tri = ctr = 0;
  res_size = 0;
  size = &res_size;
  list = CreateHeaderList();
  
  nodes = CountNodes(root);

  while(p->next != NULL){
    p = p->next;
    ResetNodesState(root);
    
    Enqueue(p,list,list,size,0); //Posem el header com a predecessor
				 //pero no afecta res!
    RenewQueue(list,list,size,0);
    Dequeue(list,size);
    k_v = *size;
    if(k_v>1){
      p->state = 0;
      tri += CalculateLinksBetweenNeig(list,root);
      ctr += k_v*(k_v-1);
    }
    else
      nodes--;
    ClearList(list,size);
  }

  C = (double)tri/(double)ctr;

  free(list);
  return C;
}

// ---------------------------------------------------------------------
// Same as the clustering coefficient, but counting squares rather
// than triangles.
// ---------------------------------------------------------------------
double SquareClustering(struct node_gra *root)
{
  int nodes,*size,res_size;
  struct node_bfs *list;
  struct node_bfs *lp;
  struct node_bfs *pl;
  struct node_gra *p = root;
  double C_av;
  int k1,k2;
  int kk;
  int C_v;
  int npre;
  int totnodes;
  int k1b;

  C_av = 0.0;
  res_size = 0;
  size = &res_size;
  list = CreateHeaderList();
  
  totnodes = nodes = CountNodes(root);

  while(p->next != NULL){
    p = p->next;
    ResetNodesState(root);
    
    Enqueue(p,list,list,size,0); //Posem el header com a predecessor
				 //pero no afecta res!
    lp = list;
    if(*size<totnodes)
      lp = RenewQueue(list,lp,size,0); // First neighbors
    else{ // If the size of the cluster is 1
      return -1.0;
    }
    k1b = *size-1;
    if(*size<totnodes){
      lp = RenewQueue(list,lp,size,1); // Second neighbors

      k1 = k2 = 0;
      pl = (list->next)->next; // Pointer to the first neighbor
      while(pl->d<2){
	pl = pl->next;
	k1++;
      }   // count first neighbors and place the pointer at the first
	  // second neighbor
      printf("%d %d = %d %d\n",p->num+1,k1,k1b,totnodes);
    
      C_v = 0;
      while(pl != NULL){
	k2++;
	npre = CountPredecessors(pl);
	C_v += npre*(npre-1);
	pl = pl->next;
      }

      if(k1>1){
	C_av += (double)(C_v)/(double)(k2*k1*(k1-1));
      }
      else
	nodes--;
    }
    else
      nodes--;

    ClearList(list,size);
  }

  C_av = C_av/(double)nodes;

  free(list);

  return C_av;
}

// ---------------------------------------------------------------------
// Compute the clustering coefficient of a single node
// ---------------------------------------------------------------------
double OneNodeClusteringCoefficient(struct node_gra *node,
				    struct node_gra *root)
{
  int nodes,*size,res_size;
  struct node_bfs *list;
  int k_v;
  int kk;
  int C_v;
  double C;

  res_size = 0;
  size = &res_size;
  list = CreateHeaderList();
  
  ResetNodesState(root);
    
  Enqueue(node,list,list,size,0); // Posem el header com a predecessor
				  // pero no afecta res!
  RenewQueue(list,list,size,0);
  Dequeue(list,size);
  k_v = *size;
  if(k_v>1){
    node->state = 0;
    C_v = CalculateLinksBetweenNeig(list,root);
    C = (double)C_v/((double)k_v*(double)(k_v-1));
  }
  else
    C = -1.0000;

  ClearList(list,size);
  free(list);

  return C;
}

// ---------------------------------------------------------------------
// Compute the square clustering coefficient of a single node
// ---------------------------------------------------------------------
double OneNodeSquareClustering(struct node_gra *node,
			       struct node_gra *root)
{
  int nodes,*size,res_size;
  struct node_bfs *list;
  struct node_bfs *lp;
  struct node_bfs *pl;
  double C_av;
  int k1,k2;
  int kk;
  int C_v;
  int npre;
  double T;
  int sizeant;

  res_size = 0;
  size = &res_size;
  list = CreateHeaderList();
  
  if(CountLinks(node) == 0)
    return -1;

  ResetNodesState(root);
    
  Enqueue(node,list,list,size,0); //Posem el header com a predecessor
				  //pero no afecta res!
  lp = list;
  lp = RenewQueue(list,lp,size,0); // First neighbors

  sizeant = *size;

  lp = RenewQueue(list,lp,size,1); // Second neighbors

  if(*size !=  sizeant){

    k1 = k2 = 0;
    pl = (list->next)->next; // Pointer to the first neighbor
    while(pl->d<2){
      pl = pl->next;
      k1++;
    }   // count first neighbors and place the pointer at the first
	// second neighbor
    
    C_v = 0;
    while(pl != NULL){
      k2++;
      npre = CountPredecessors(pl);
      C_v += npre*(npre-1);
      pl = pl->next;
    }

    if(k1<= 1)
      return -2; // only 1 first neighbor!!
  }
  else
    return -3; // all nodes are first neighbors!!
  
  T = (double)C_v/((double)k2*(double)k1*(double)(k1-1));

  ClearList(list,size);
  free(list);

  return T;
}

// ---------------------------------------------------------------------
// Calculates the betweenness of each link and stores in the btw field
// of the node_lis
// ---------------------------------------------------------------------
void CalculateLinkBetweenness(struct node_gra *root)
{
  int a,d,nodes,*size,i,counter,size_ant;
  struct node_gra *p = root;
  struct node_bfs *list,*lp;
  double *bet_loc;
  int n_pred;
  struct pred *ppre;
  int *ocupacio;
  struct node_lis *tar,*temp;
  int nnod=CountNodes(root);

  ocupacio = allocate_i_vec(nnod);
  bet_loc = allocate_d_vec(nnod);

  list = CreateHeaderList();

  for(i=0; i<nnod; i++){
    ocupacio[i] = 0;
  }
  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);

  // set the btw of all links to 0
  while(p->next != NULL){
    p = p->next;
    ocupacio[p->num] = 1;
    temp = p->neig;
    while(temp->next != NULL){
      temp = temp->next;
      temp->btw = 0.0;
    }
  }
  
  p = root;
  while(p->next != NULL){
    counter++;
    d = 0;
    ResetNodesState(root);
    p = p->next;

    Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				 //predecessor del primer!!!!!
    lp = list;
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      d++;
    }while(*size != size_ant);

    //Update the betweenness
    for(i = 0;i<nnod;i++){
      bet_loc[i] = (double)ocupacio[i];
    }
    
    lp = list->last;
    while((n_pred = CountPredecessors(lp)) != 0){
      ppre = lp->pred;
      while(ppre != NULL){
	bet_loc[((ppre->ref)->ref)->num] +=
	  bet_loc[(lp->ref)->num]/(double)n_pred;
	
	tar = ((lp->ref)->neig)->next;
	while((tar->node) != ((ppre->ref)->ref)->num)
	  tar = tar->next;
	tar->btw += bet_loc[(lp->ref)->num]/(double)n_pred;

	tar = (((ppre->ref)->ref)->neig)->next;
	while((tar->node) != (lp->ref)->num)
	  tar = tar->next;
	tar->btw += bet_loc[(lp->ref)->num]/(double)n_pred;

	ppre = ppre->next;
      }
      lp = lp->prev;
    }
 
    ClearList(list,size);
  }
  
  free_i_vec(ocupacio);
  free_d_vec(bet_loc);
  free(list);
}

// ---------------------------------------------------------------------
// Calculates the betweenness of each link and writes in n1 and n2 the
// nodes corresponding to the largest betweenness in the network.
// ---------------------------------------------------------------------
void CalculateBiggestLinkBetweenness(struct node_gra *root,int *n1,int *n2)
{
  int a,d,nodes,*size,i,counter,size_ant;
  struct node_gra *p = root;
  struct node_bfs *list,*lp;
  double *bet_loc;
  int n_pred;
  struct pred *ppre;
  int *ocupacio;
  struct node_lis *big = NULL;
  struct node_lis *tar,*temp;
  int nnod=CountNodes(root);

  ocupacio = allocate_i_vec(nnod);
  bet_loc = allocate_d_vec(nnod);
  list = CreateHeaderList();

  for(i = 0;i<nnod;i++){
    ocupacio[i] = 0;
  }
  counter = 0;
  a = 0;
  size = &a;

  nodes = CountNodes(root);

  while(p->next != NULL){
    p = p->next;
    ocupacio[p->num] = 1;
    temp = p->neig;
    while(temp->next != NULL){
      temp = temp->next;
      temp->btw = 0.0;
    }
  }
  
  big = ((root->next)->neig)->next;
  p = root;
  while(p->next != NULL){
    counter++;
    d = 0;
    ResetNodesState(root);
    p = p->next;

    Enqueue(p,list,list,size,0); //ATENCIO!! Posem el header com a
				 //predecessor del primer!!!!!
    lp = list;
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      d++;
    }while(*size !=  size_ant);

    //Actualize the betweenness
    for(i = 0;i<nnod;i++){
      bet_loc[i] = (double)ocupacio[i];
    }
    
    lp = list->last;
    while((n_pred = CountPredecessors(lp)) != 0){
      ppre = lp->pred;
      while(ppre != NULL){
	bet_loc[((ppre->ref)->ref)->num] +=
	  bet_loc[(lp->ref)->num]/(double)n_pred;
	
	tar = ((lp->ref)->neig)->next;
	while((tar->node) != ((ppre->ref)->ref)->num)
	  tar = tar->next;
	tar->btw += bet_loc[(lp->ref)->num]/(double)n_pred;
	if(big ==  NULL || tar->btw > big->btw){
	  big = tar;
	  *n2 = (lp->ref)->num;
	  *n1 = ((ppre->ref)->ref)->num;
	}

	tar = (((ppre->ref)->ref)->neig)->next;
	while((tar->node) != (lp->ref)->num)
	  tar = tar->next;
	tar->btw += bet_loc[(lp->ref)->num]/(double)n_pred;

	ppre = ppre->next;
      }
      lp = lp->prev;
    }
 
    ClearList(list,size);
  }

  free_i_vec(ocupacio);
  free_d_vec(bet_loc);
  free(list);
}

// ---------------------------------------------------------------------
// Calculates the assortativity of a network
// ---------------------------------------------------------------------
double Assortativity(struct node_gra *net)
{
  struct node_gra *p;
  struct node_lis *li;
  int *deg;
  int i;
  int jk,jpk,j2pk2;
  int M;
  double r;

  // Initialize variables
  deg = allocate_i_vec(CountNodes(net));
  M = 0;
  jk = 0;
  jpk = 0;
  j2pk2 = 0;

  // Calculate nodes degrees
  p = net;
  while ((p = p->next) != NULL) {
    deg[p->num] = CountLinks(p);
    M += deg[p->num];
  }

  // Calculate assortativity
  p = net;
  while ((p = p->next) != NULL) {
    li = p->neig;
    while ((li = li->next) != NULL) {
      jk += deg[p->num] * deg[(li->ref)->num];
      jpk += deg[p->num] + deg[(li->ref)->num];
      j2pk2 += deg[p->num] * deg[p->num] + 
	deg[(li->ref)->num] * deg[(li->ref)->num];
    }
  }

  r = ((double)jk / (double)M - ((double)jpk / (double)(2 * M)) *
       ((double)jpk / (double)(2 * M))) / 
    ((double)j2pk2 / (double)(2 * M) -
     ((double)jpk / (double)(2 * M)) *
     ((double)jpk / (double)(2 * M)));

  // Free memory and return
  free_i_vec(deg);
  return r;
}

// ---------------------------------------------------------------------
// Calculate the average degree of the neighbors of a node
// ---------------------------------------------------------------------
double CalculateKnn(struct node_gra *node)
{
  struct node_lis *p=node->neig;
  int nneig=0, totdeg=0;

  while ((p = p->next) != NULL) {
    totdeg += CountLinks(p->ref);
    nneig++;
  }
  
  return (double)totdeg / (double)nneig;
}

// ---------------------------------------------------------------------
// Returns 1 if the graph is connected and 0 if the graph has more
// than one component.
// ---------------------------------------------------------------------
int IsGraphConnected(struct node_gra *p, int N)
{
  struct node_bfs *list,*lp;
  int *size,r1;
  int size_ant;
  int d;

  r1 = 0;
  size = &r1;

  ResetNodesState(p);
  p = p->next;
  list = CreateHeaderList();

  Enqueue(p,list,list,size,0);
  lp = list;
  d = 0;
  do{
    size_ant = *size;
    lp = RenewQueue(list,lp,size,d);
    d++;
  }while(*size != size_ant);

  ClearList(list,size);

  free(list);

  if(size_ant == N)
    return 1;
  else
    return 0;
}

// ---------------------------------------------------------------------
// ???????????
// ---------------------------------------------------------------------
int AreConnectedList(struct node_gra *root,
		     struct node_gra *n1,
		     int cluslis[])
{
  struct node_bfs *list,*lp,*lp2;
  int *size,res1;
  int size_ant;
  int d;
  
  res1 = 0;
  size = &res1;

  list = CreateHeaderList();

  d = 0;
  ResetNodesState(root);
  Enqueue(n1,list,list,size,d); //ATENCIO!! Posem el header com a
				//predecessor del primer!!!!!
  lp = list;
  do{
    size_ant = *size;
    lp = RenewQueue(list,lp,size,d);
    d++;
    lp2 = lp;
    while(lp2->next != NULL){
      lp2 = lp2->next;
      if(cluslis[(lp2->ref)->num] == 1){
	ClearList(list,size);
	free(list);
	return 1;
      }
    }
  }while(*size != size_ant);
  
  ClearList(list,size);
  free(list);
  return 0;
}

// ---------------------------------------------------------------------
// Counts the number of strongly connected sets in the network
// ---------------------------------------------------------------------
int CountStronglyConnectedSets(struct node_gra *root)
{
  struct node_gra *root_cop = NULL;
  struct node_gra *p,*p2;
  struct node_bfs *list,*lp,*lp2;
  int nnod = CountNodes(root);
  int anod = 0;
  int *size,res1;
  int *selected;
  int *this_net;
  int i;
  int size_ant;
  int d;
  int z;
  double C,T;
  int nclus = 0;

  selected = allocate_i_vec(nnod);
  this_net = allocate_i_vec(nnod);

  for(i = 0;i<nnod;i++)
    selected[i] = 0;
  
  root_cop = CopyNetwork(root);

  res1 = 0;
  size = &res1;

  list = CreateHeaderList();

  do{
    p = root->next;
    while(selected[p->num] != 0)
      p = p->next;
    selected[p->num] = 1;

    for(i = 0;i<nnod;i++)
      this_net[i] = 0;
    d = 0;

    ResetNodesState(root);
    Enqueue(p,list,list,size,d);
    anod++;
    lp = list;
    this_net[p->num] = 1;
    
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      lp2 = lp;
      while(lp2->next !=  NULL){
	if(AreConnectedList(root_cop,GetNode(((lp2->next)->ref)->num,root_cop),this_net) == 0){
	  DequeueOne(list,lp2,size);
	}
	else{
	  this_net[((lp2->next)->ref)->num] = 1;
	  selected[((lp2->next)->ref)->num] = 1;
	  anod++;
	  lp2 = lp2->next;
	}
      }
      d++;
    }while(*size !=  size_ant);

    ClearList(list,size);

    nclus++;

  }while(anod<nnod);

  free(list);
  RemoveGraph(root_cop);

  free_i_vec(selected);
  free_i_vec(this_net);
  return nclus;
}

// ---------------------------------------------------------------------
// Creates a network that contains only the giant component of root
// ---------------------------------------------------------------------
struct node_gra *GetLargestStronglyConnectedSet(struct node_gra *root,
						int thres)
{
  struct node_gra *root_cop=NULL;
  struct node_gra *root_loc=NULL;
  struct node_gra *p, *p2;
  struct node_gra *temp;
  struct node_bfs *list, *lp, *lp2;
  int nnod=CountNodes(root);
  int anod=0;
  int *size, res1;
  int *selected;
  int *this_net;
  int i;
  int size_ant;
  int d;
  int z;
  int maxS = 0;
  struct node_gra *giant = NULL;

  selected = allocate_i_vec(nnod);
  this_net = allocate_i_vec(nnod);

  for(i = 0;i<nnod;i++)
    selected[i] = 0;
  
  root_cop = CopyNetwork(root);

  res1 = 0;
  size = &res1;

  list = CreateHeaderList();

  do{
    root_loc = CreateHeaderGraph();
    p = root->next;
    while(selected[p->num] != 0)
      p = p->next;
    selected[p->num] = 1;

    for(i = 0;i<nnod;i++)
      this_net[i] = 0;
    d = 0;

    ResetNodesState(root);
    Enqueue(p, list, list, size, d);
    anod++;
    lp = list;
    temp = CreateNodeGraph(root_loc, p->label);
    temp->num = p->num;
    CopyAdjacencyList(p, temp);
    this_net[p->num] = 1;
    
    do{
      size_ant = *size;
      lp = RenewQueue(list, lp, size, d);
      lp2 = lp;
      while(lp2->next != NULL){
	if(AreConnectedList(root_cop,
			    GetNode(((lp2->next)->ref)->num, root_cop),
			    this_net) == 0){
	  DequeueOne(list, lp2, size);
	}
	else{
	  temp = CreateNodeGraph(root_loc,((lp2->next)->ref)->label);
	  temp->num = lp2->next->ref->num;
	  CopyAdjacencyList(GetNode(((lp2->next)->ref)->num,root),temp);
	  this_net[temp->num] = 1;
	  selected[temp->num] = 1;
	  anod++;
	  lp2 = lp2->next;
	}
      }
      d++;
    }while(*size != size_ant);

    CleanAdjacencies(root_loc,this_net);
    RewireAdjacency(root_loc);
    RenumberNodes(root_loc);

    if (CountNodes(root_loc) > maxS) {
      maxS = CountNodes(root_loc);
      if (giant !=  NULL) {
	RemoveGraph(giant);
      }
      giant = root_loc;
      if (maxS > nnod/2 || maxS>thres) {
	ClearList(list, size);
	RemoveGraph(root_cop); // WAS COMMENTED OUT?!?!?!?!
	return giant;
      }
    }
    else
      RemoveGraph(root_loc);

    ClearList(list,size);
    
  }while(anod<nnod);

  RemoveGraph(root_cop);

  free(list);
  free_i_vec(selected);
  free_i_vec(this_net);

  return giant;
}

struct node_gra *GetLargestWeaklyConnectedSet(struct node_gra *root,int thres)
{
  struct node_gra *root_cop = NULL;
  struct node_gra *root_loc = NULL;
  struct node_gra *p, *p2, *p3;
  struct node_gra *temp;
  struct node_bfs *list,*lp,*lp2;
  int nnod=CountNodes(root);
  int anod=0;
  int *size, res1;
  int *selected;
  int *this_net;
  int i;
  int size_ant;
  int d;
  int z;
  int maxS = 0;
  struct node_gra *giant = NULL;

  selected = allocate_i_vec(nnod);
  this_net = allocate_i_vec(nnod);

  for(i = 0;i<nnod;i++)
    selected[i] = 0;
  
  root_cop = CopyNetwork(root);

  res1 = 0;
  size = &res1;

  list = CreateHeaderList();
  
  // Reachable from p
  do{
    root_loc = CreateHeaderGraph();
    p = root->next;

    while(selected[p->num] != 0)
      p = p->next;
    selected[p->num] = 1;

    for(i = 0;i<nnod;i++)
      this_net[i] = 0;
    d = 0;

    ResetNodesState(root);
    Enqueue(p,list,list,size,d);
    anod++;
    lp = list;
    temp = CreateNodeGraph(root_loc, p->label);
    temp->num = p->num;
    CopyAdjacencyList(p,temp);
    this_net[p->num] = 1;
    
    do{
      size_ant = *size;
      lp = RenewQueue(list,lp,size,d);
      lp2 = lp;
      while(lp2->next != NULL){
	temp = CreateNodeGraph(root_loc,((lp2->next)->ref)->label);
	temp->num = lp2->next->ref->num;
	CopyAdjacencyList(GetNode(((lp2->next)->ref)->num,root),temp);
	this_net[temp->num] = 1;
	selected[temp->num] = 1;
	anod++;
	lp2 = lp2->next;
      }
      d++;
    }while(*size != size_ant);

    // Nodes that enable one to reach any node reachable from p
    p3 = p;

    while (p3->next != NULL){
      p3 = p3->next;

      if ((AreConnectedList(root_cop,GetNode(p3->num,root_cop),this_net) == 1)
	  && (selected[p3->num] == 0)) {

	temp = CreateNodeGraph(root_loc, p3->label);
	temp->num = p3->num;
	CopyAdjacencyList(p3, temp);
	this_net[temp->num] = 1;
	selected[temp->num] = 1;
	anod++;
	
	Enqueue(p3,list,list,size,d);

	do{
	  size_ant = *size;
	  lp = RenewQueue(list,lp,size,d);
	  lp2 = lp;
	  while(lp2->next != NULL){
	    temp = CreateNodeGraph(root_loc,((lp2->next)->ref)->label);
	    temp->num = lp2->next->ref->num;
	    CopyAdjacencyList(GetNode(((lp2->next)->ref)->num,root),temp);
	    this_net[temp->num] = 1;
	    selected[temp->num] = 1;
	    anod++;
	    lp2 = lp2->next;
	  }
	  d++;
	}while(*size != size_ant);

      }
    }

    // Rewire the cluster
    CleanAdjacencies(root_loc,this_net);
    RewireAdjacency(root_loc);
    RenumberNodes(root_loc);

    // Check if this is the largest component
    if(CountNodes(root_loc)>maxS){
      maxS = CountNodes(root_loc);
      if (giant !=  NULL){
	RemoveGraph(giant);
      }
      giant = root_loc;
      if(maxS>nnod/2 || maxS>thres ){
	ClearList(list,size);
/* 	RemoveGraph(root_cop); */
	return giant;
      }
    }
    else
      RemoveGraph(root_loc);

    ClearList(list,size);

  }while(anod<nnod);

  RemoveGraph(root_cop);

  free(list);
  free_i_vec(selected);
  free_i_vec(this_net);
  return giant;
}


// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// Node and network comparison
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Calculates the topological overlap, as defined by Ravasz et al.,
// between two nodes.
// ---------------------------------------------------------------------
double TopologicalOverlap(struct node_gra *n1, struct node_gra *n2)
{
  double overlap;
  int ncom=0;
  int minim;
  struct node_gra *node;
  struct node_lis *p;
  int k1, k2;

  // Degrees of both nodes
  k1 = CountLinks(n1);
  k2 = CountLinks(n2);

  // Determine the node with less neighbors
  if (k1 < k2) {
    minim = k1;
    p = n1->neig;
    node = n2;
  }
  else{
    minim = k2;
    p = n2->neig;
    node = n1;
  }

  // Count the number of common neighbors
  while (p->next != NULL) {
    p = p->next;
    if (p->ref == node)
      ncom++;
    else
      ncom += IsThereLinkSoft( node, p->node);
  }

  // Evaluate overlap
  overlap = (double)ncom / (double)minim;

  return overlap;
}
