
/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE:queue.h
-----------------------------------------------------------------------*/

#ifndef QUEUE_INCLUDED
#define QUEUE_INCLUDED

/*        %%s  Node      */
/*****************************************************************
     This structure is used to store the List linkage information of a
ListInfo structure.  It contains all the necessary information for the
List functions to function properly.  This structure must be the first
one defined in any block of memory to be linked with the List functions.
for an example of the used of The Node structure look in the files
ipd2dms.c and ipd2man.h
******************************************************************/
#include <stdio.h>
#define FALSE 0
#define TRUE  1
typedef struct
{
     void  *Next;
     void  *Previous;
}
     Node, * PNODE;

/*****************************************************************
     Next     :  is a pointer to the next structure in this List.
     Previous :  is a pointer to the previous structure in this List.
     priority :  this is the priority of this structure in the List.  The
                 highest priority is 0.  This field is only used by the
                 functions EnQue and DeQue.
******************************************************************/
/*        %%e       */


/*        %%s  ListInfo      */

/*****************************************************************
      This is the general means of linking application defined information into
Lists and queues.  All structures must begin with the Node Structure.  All
other data in the structure is user definable.
******************************************************************/

typedef struct List
{
     Node     ListNode;       /*  link to the next Listinfo Structure  */
     /*  user definable data  */
}    ListInfo, *PLISTINFO;

/*****************************************************************
     ListNode  :  this is the required node structure for the List
                  mainpulation functions.  This must be the first
                  element of a user definable structure.

     In order for an application to use the List routines, it must define
a structure with all the needed information.  The first element in the
user definable structure must be a Node structure.  The Node structure
contains all the necessary information for the List routines to do their
magic.  For an example of a user defined List structure see the file
ipd2i.h.  The User definable structure can be passed to any List function
that excepts a pointer to a ListInfo structure.

example:

typedef  mstruct
{
     Node   ListNode;
     int    a,b,c,d,e,f,g;
}
     mystruct;

     the user definable portion of the above structure is represented by
the integers a,b,c,d,e,f,g.  When passing this structure to a List
function a cast of (ListInfo *) must be made to satisify the "C" complier.
******************************************************************/
/*        %%e       */


/*        %%s ListHead        */
/*****************************************************************
     ListHead is used as a header to a List.  LHeaders[0] points to the
head of the List.  LHeaders[1] points the tail of the list.  When
accessing these variables use the defines  LISTHEAD, LISTTAIL.
******************************************************************/

typedef struct LHead
{
     PLISTINFO  LHeaders[2];
     int         NumList;
}
ListHead, *PLISTHEAD;

/*****************************************************************
     LHeaders   :  this is an array of two pointers to ListInfo structures.
                   This information is used to point to the head and tail of
                   a list.
     NumList    :  this integer hold the number of structures linked into this
                   list.

ListHead #define:

     LISTHEAD  :  when Peeking down a list this specifies you should
                  start at the Head of the list and search downward.

     LISTTAIL  :  when Peeking down a list this specifies you should
                  start at the tail of the list and search foward.
 ******************************************************************/

#define  LISTHEAD  0

#define  LISTTAIL  1
/*        %%e       */ 

typedef int BOOL;
typedef void * PVOID;

#define PEEKFROMHEAD( lh, ind )     ( PeekList( (lh), LISTHEAD, (ind) ) )
#define PEEKFROMTAIL( lh, ind )     ( PeekList( (lh), LISTTAIL, (ind) ) )
#define EMPTYLIST( lh )             ( ( (lh)->LHeaders[LISTHEAD] == NULL ) )

/*   General utility routines   */
/*        %%s QueRoutines          */
BOOL    InitList      ( PLISTHEAD );

/*****************************************************************
     InitList :  Initialize a new list structure for use with the List
                 routines

     INPUTS   :  LHead : a pointer to a ListHead structure.
     OUTPUT   :  a boolean value TRUE if no errors occured FALSE
                 otherwise
******************************************************************/


PLISTINFO  PeekList      ( PLISTHEAD, int, int   );

/*****************************************************************
     PeekList :  This funciton peeks down a list for the N'th element
                 from the HEAD or TAIL of the list

     INPUTS   :  LHead    :  a pointer to a List head structure.
                 from     :  can either search from the HEAD or TAIL
                             of the list
                 where    :  how many nodes from the begining should the
                             List routines look.
     OUTPUT   :  a pointer to a ListInfo structure identified by
                 from/where or NULL if an error occurred.
******************************************************************/


PLISTINFO   RemoveList( PLISTHEAD LHead, PLISTINFO LInfo );


/*****************************************************************
     RemoveList: Remove a ListInfo structure from a List.

     INPUTS    : LHead  :  a pointer to a ListHead structure.
                 LInfo  :  a pointer to the ListInfo structure to remove
                           from the list.
     OUTPUT    : a pointer to the ListInfo structure that was removed or
                 NULL if an error occurred.
******************************************************************/

BOOL  InsertNode( PLISTHEAD LHead, int nPos, PLISTINFO LInfo );

/*****************************************************************
     InsertNode: add a node to a list after a given node
     
     INPUTS    : LHead : a pointer to a ListHead structure.
                 nPos  : the position to insert the node into
                 LInfo : a pointer to the new node to add to the list.
     OUTPUT: a boolean value TRUE if all goes well false otherwise
*****************************************************************/

BOOL   AddHead       ( PLISTHEAD, PLISTINFO );

/*****************************************************************
     AddHead   : add a ListInfo structure to the HEAD of a list.

     INPUTS    : LHead  : a pointer to a ListHead structure of the list
                          to add to.
                 LInfo  : a pointer to the ListInfo structure to add to
                          the list.
     OUTPUT    : A boolean value TRUE if no errors occurred FALSE
                 otherwise.
******************************************************************/


BOOL     AddTail       ( PLISTHEAD, PLISTINFO );

/*****************************************************************
     AddTail   : Add a ListInfo structure to the TAIL of a list.

     INPUTS    : LHead  : a pointer to a ListHead structure of the List
                          to add to.
                 LInfo  : a pointer to the ListInfo structure to add to
                          the List.
     OUTPUT    : a boolean value TRUE if no errors occurred FALSE
                 otherwise.
******************************************************************/


PLISTINFO  RemTail       ( PLISTHEAD );

/*****************************************************************
     RemTail   : Remove a ListInfo structure from the TAIL of a List.

     INPUTS    : LHead  : a pointer to a ListHead structure of the List
                          to remove from.
     OUTPUT    : a pointer to the ListInfo structure that was removed
                 or NULL if an error occurred.
******************************************************************/


PLISTINFO  RemHead       ( PLISTHEAD );

/*****************************************************************
     RemHead   : Remove a ListInfo structure from the Head of a List.

     INPUTS    : LHead  : a pointer to a ListHead structure of the List
                          to remove from.
     OUTPUT    : a pointer to the ListInfo structure that was removed or
                 NULL if an error occurred.
******************************************************************/

PLISTINFO  SearchList(
                        PLISTHEAD lpListHead,
                        PVOID lpSKey,
                        int ( * CompareCallBack) ( PVOID, PVOID ) );

/*****************************************************************
  SearchList:
        Try to find a specific node in the queue whose key matches with
   searching key. Return the pointer to that node if found, return NULL
   otherwise

   Input:
     lpHashTbl       => a far pointer to the hash table
     lpKey           => a far poniter to searching key
     CompareCallBack => comparision function

   Output: a far pointer to the node to be found

 ******************************************************************/

#define           NumOnList(lh) ( ((lh)->NumList)        )

/*****************************************************************
     NumOnList: Returns the number of Nodes linked to a ListHead
                structure.  This number is maintained by the List
                routines.
******************************************************************/

#define           GetNextNode(pli) ( ((pli)->ListNode.Next) )

/********************************************************
     GetNextNode: This macro returns the Next Structure in this list.
                  This macro will return NULL if no more structures are
                  in the List.
*********************************************************/

#define           GetPrevNode(pli) ( ((pli)->ListNode.Previous) )

/********************************************************
     GetPrevNode: This macro returns the Previous Structure in this list.
                  This macro will reutrn NULL if no more structures are
                  in the List.
********************************************************/
/*        %%e       */

#endif


