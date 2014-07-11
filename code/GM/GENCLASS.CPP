//---------------------------------------------------------------------------
//                      F U N C T I O N S
//---------------------------------------------------------------------------
#include "gen.h"
#include "genclass.hpp"
static char     FreeIt(NodeClass *node);  

//---------------------------------------------------------------------------
//      C L A S S    F U N C T I O N    D E F I N A T I O N S
//---------------------------------------------------------------------------
char NetworkClass::SetRoot(NodeClass *Node)
  {
  Root=Node;
  return(TRUE);
  }

NodeClass *NetworkClass::GetRoot(void)
  {
  return(Root);
  }

char NetworkClass::Link(NodeClass *From, NodeClass *To,int Linknum)
  {
  if ((From==NULL)||(To==NULL)) return(FALSE);  // Return if no node present
  if (Linknum==0) if ( (Linknum=From->FindLink())==MAXLINKS) return(FALSE); // return if no free links
  From->SetLink(Linknum,To);
  return(TRUE);
  }


NetworkClass::NetworkClass(int MaxLinks)
  {
#ifdef DEBUG
  if (!NewAbortMSG("GM:NetworkClass Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
  NodeLinkNum=MaxLinks;
  CurNode=NULL;
  }

int NodeClass::FindLink(NodeClass *Node)
  {
  register int l;
  for (l=0;l<MaxLinks;l++) if (Links[l]==Node) return(l);
  return(MAXLINKS);  
  }


NodeClass::NodeClass(unsigned int Max_Links, unsigned int Data_Size, void *InData)
  {
  register unsigned int l;

#ifdef DEBUG
  if (!NewAbortMSG("GM:NodeClass Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
  //printf("%s AT %d.\n",__FILE__,__LINE__);
  MaxLinks=Max_Links;  
  DataSize=Data_Size;
  Links=NULL;
  Data=NULL;
  //printf("%s AT %d.\n",__FILE__,__LINE__);
  if ((Links = ::new NodeClass* [MaxLinks])==NULL) NewBombOut();
  else for (l=0;l<MaxLinks;l++) Links[l]=NULL;
  //printf("%s AT %d.\n",__FILE__,__LINE__);
  if (Data_Size!=0)  // Get mem and copy over data if it is supplied
    if (CpyData(Data_Size, InData)==FALSE) NewBombOut();
  //printf("%s AT END!\n",__FILE__,__LINE__);
#ifdef DEBUG
  printf("Node initialized!\n");
#endif
  }

NodeClass *NodeClass::GetLink(unsigned int LinkNum)
  {
  return(Links[LinkNum]);
  }

char NodeClass::SetLink(unsigned int LinkNum, NodeClass *Link)
  {
  if (LinkNum<MaxLinks) { Links[LinkNum]=Link; return(TRUE); }
  return(FALSE);
  }


void NodeClass::DelData(void)
  {
  free(Data);
  DataSize=0;
  }

char NodeClass::SetData(unsigned int Data_Size, void *InData)
  {
  if (Data!=NULL) free(Data);
  DataSize=Data_Size;
  Data=InData;
  return(TRUE);
  }

char NodeClass::CpyData(unsigned int Data_Size, void *InData)
  {
  if (Data!=NULL) free(Data);
  Data=NULL;
  DataSize=Data_Size;
  if ( (Data= (void *) malloc( (size_t) DataSize))==NULL) return (FALSE);
  memcpy(Data, InData, (size_t) DataSize);
  return(TRUE);
  }


NodeClass::~NodeClass()
  {
  if (Links!=NULL) delete Links;
  if (Data!=NULL)
    {
    free(Data);
#ifdef DEBUG
    printf("Data freed. ");
    }
  printf("Node destroyed.\n");
#else
    }
#endif
  }


char LinkedListClass::Swap(NodeClass *Node1, NodeClass *Node2)
  {
  NodeClass *temp;
  NodeClass *before;
  
  before=FindPrev(Node1);  // Find the Previous nodes before any ordering is disrupted
  temp=FindPrev(Node2);    

  SetNext(before,Node2);   // Swap them in the previous nodes
  SetNext(temp,Node1);  

  temp=Next(Node2);
  SetNext(Node2,Next(Node1));  // Swap who they are pointing to
  SetNext(Node1,temp);        

  return(TRUE);
  }

int LinkedListClass::Traverse(char DoToEach(NodeClass *Curnode),NodeClass *StartNode)
  {
  NodeClass *Temp;
  int Counter=0;
  
  if (StartNode==NULL) StartNode=First;     // Start at beginning if caller 
                                            // gave nothing else.  
  while(StartNode!=NULL)  // Go through every node
    {
    Temp=Next(StartNode); // Get the next node - can't do it after DoToEach
                          // because he might delete StartNode.
    if (DoToEach(StartNode)==FALSE) return(Counter);  // Send the whole node
                          // and abort if the guy returns FALSE
    StartNode=Temp;       // StartNode gets the next node in the list
    Counter++;                  // Keep a count of the # of nodes traversed
    }
  return(Counter); 
  }



NodeClass *LinkedListClass::FindPrev(NodeClass *Node)
  {
  NodeClass *CurNode;
  CurNode=First;
  
  if (Node==First) return (NULL);
  // Look for the node to kill, and end loop with curnode as the one before it.
  while ((Next(CurNode)!=NULL)&&(Next(CurNode)!=Node)) CurNode=Next(CurNode);
  return(CurNode);
  }



char LinkedListClass::KillNode(NodeClass *nodetokill)
  {
  register int l;
  NodeClass *curnode;

  if (nodetokill==First)
    {
    First=Next(nodetokill);
    delete nodetokill;
    return(TRUE);
    }  

  if ((curnode=FindPrev(nodetokill))==NULL) return(FALSE);
  else                                    // Delete the node
    {
    NodeClass *temp;
    temp=Next(nodetokill); // Set Next(curnode) to skip the node to delete 
    SetNext(curnode,temp);
    delete nodetokill;  // Free the node's memory
    return(TRUE);
    }
  }

LinkedListClass::~LinkedListClass()
  {
  Traverse(FreeIt);
#ifdef DEBUG
  printf("Linked list destroyed.\n");
#endif
  }


LinkedListClass::LinkedListClass()
  {
#ifdef DEBUG
  if (!NewAbortMSG("GM:LinkedListClass Initialization.\n")) printf("Abort Initialization Error!\n");
#endif
  First=NULL;
#ifdef DEBUG
  printf("Linked List initialized.\n");
#endif
  }

NodeClass *LinkedListClass::Add(NodeClass *newnode)
  {
  SetNext(newnode,First);
  First=newnode;
  return(First);
  }

// Selectors

NodeClass *LinkedListClass::Next(NodeClass *curnode)
  {
  return (curnode->GetLink(0));
  // return((char *) *((char **) curnode));
  }

void LinkedListClass::SetNext(NodeClass *Node, NodeClass *NextNode)
  {
  Node->SetLink(0,NextNode);
  // char **temp;  // A pointer to a pointer
  // temp = (char **) Node;
  // *temp = NextNode;
  }

//---------------------------------------------------------------------------
//             G E N E R I C    F U N C T I O N S
//---------------------------------------------------------------------------



char FreeIt(NodeClass *node)
  {
  delete node;
  return(TRUE);
  }

//---------------------------------------------------------------------------
//                  E N D    O F        F I L E
//---------------------------------------------------------------------------

