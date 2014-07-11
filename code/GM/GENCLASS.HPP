//---------------------------------------------------------------------------
// genclass.hpp                Programmer: Andy Stone   Created: July 8,1992-
//      This header file contains all the truely generic classes:           -
//                      1. A Linked list class                              -
//                      2. A Network class                                  -
//                      3. A Binary Tree                                    -
//                      4. A Sort class                                     -
//      NO                5. A memory mallocing class                       -
//                      
//---------------------------------------------------------------------------
#ifndef GENCLASS
#define GENCLASS
#include <mem.h>
#include <alloc.h>
#include <stdlib.h>
#include <stdio.h>

void NewBombOut(void);

#define ERROR           2
#define MAXLINKS        10000
//---------------------------------------------------------------------------
//                              C L A S S E S
//---------------------------------------------------------------------------

class NodeClass
  {
  protected:
  unsigned int          MaxLinks;
  unsigned int          DataSize;
  NodeClass           **Links;
  void                 *Data;

  public:
//  int x,y;
//  unsigned char flag;  // used in various network algorythms
  
                NodeClass(unsigned int Max_Links, unsigned int Data_Size=0, void *InData=NULL);
                ~NodeClass();
  char          SetData(unsigned int Data_Size, void *InData);  // Sets Data to InData 
  char          CpyData(unsigned int Data_Size, void *InData);  // Copies *InData to another location, and sets Data to it
  inline void  *GetData(void)  { return(Data); }                // Returns pointer to the data.
  void          DelData(void);                                  // Erases and frees data memory.

  char          SetLink(unsigned int LinkNum, NodeClass *Link); // Sets link to links[LinkNum]
  NodeClass    *GetLink(unsigned int LinkNum);                  // Returns links[LinkNum]
  int           FindLink(NodeClass *Node=NULL);                 // Default is find a free link.
  };

class NetworkClass
  {
  protected:
  NodeClass *CurNode;
  int NodeLinkNum;  // = 0 means any # of nodes, > 0 means node MUST have this # of links
  public:
  NodeClass     *Root;
                NetworkClass(int MaxLinks=0);
  char          Link(NodeClass *NodeFrom, NodeClass *NodeTo=NULL,int Linknum=0); // If Null, Add to the CurNode
  char          SetRoot(NodeClass *Node);
  NodeClass    *GetRoot(void);
  };

class LinkedListClass   // : public NetworkClass
  {
  private:
  void          SetNext(NodeClass *Node, NodeClass *NextNode);
  NodeClass    *FindPrev(NodeClass *Node);
  public:  
  NodeClass    *Add(NodeClass *Node);
  NodeClass    *Next(NodeClass *CurNode);
  NodeClass    *First;                    // Pointer to the first Node
  char          KillNode(NodeClass *nodeaddr);
  char          Swap(NodeClass *Node1, NodeClass *node2);
  char          PutAfter(char *node, char *AfterNode); 
                LinkedListClass();  
               ~LinkedListClass();
  int           Traverse(char DoToEach(NodeClass *Curnode),NodeClass *StartNode=NULL);
  };
#endif
