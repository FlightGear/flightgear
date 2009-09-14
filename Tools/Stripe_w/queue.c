/********************************************************************/
/*   STRIPE: converting a polygonal model to triangle strips    
     Francine Evans, 1996.
     SUNY @ Stony Brook
     Advisors: Steven Skiena and Amitabh Varshney
*/
/********************************************************************/

/*---------------------------------------------------------------------*/
/*   STRIPE: queue.c
     This file contains the routines used in the data structures lists, which
     are queues.
*/
/*---------------------------------------------------------------------*/

 #include "queue.h"
 


/*----------------------------------------------------------------------------
 * InitList:
 */
BOOL  InitList  (PLISTHEAD LHead)
 
{
  if (LHead == NULL) return(FALSE);

  LHead->LHeaders[LISTHEAD] = LHead->LHeaders[LISTTAIL] = NULL;
  LHead->NumList = 0;
  return(TRUE);
}

/*----------------------------------------------------------------------------
 * AddHead:
 */
BOOL  AddHead(PLISTHEAD LHead, PLISTINFO LInfo)
{
     if (LHead == NULL || LInfo == NULL)
          return(FALSE);
     if (EMPTYLIST(LHead))
          LHead->LHeaders[LISTTAIL] = LInfo;
     else LHead->LHeaders[LISTHEAD]->ListNode.Previous = (void  *) LInfo;

     LInfo->ListNode.Next = (void  *) LHead->LHeaders[LISTHEAD];
     LHead->LHeaders[LISTHEAD] = LInfo;
     LInfo->ListNode.Previous = NULL;
     LHead->NumList++;
     return(TRUE);
}

/*----------------------------------------------------------------------------
 * AddTail
 */
BOOL  AddTail(PLISTHEAD LHead, PLISTINFO LInfo)
{
     if (LHead == NULL || LInfo == NULL)
          return(FALSE);
     if (EMPTYLIST(LHead))
          LHead->LHeaders[LISTHEAD] = LInfo;
     else LHead->LHeaders[LISTTAIL]->ListNode.Next = (void *) LInfo;

     LInfo->ListNode.Previous = (void  *) LHead->LHeaders[LISTTAIL];
     LHead->LHeaders[LISTTAIL] = LInfo;
     LInfo->ListNode.Next = NULL;
     LHead->NumList++;
     return(TRUE);
}


BOOL  InsertNode( PLISTHEAD LHead, int nPos, PLISTINFO LInfo )
{
PLISTINFO LAddNode;

     if ( LHead == NULL || LInfo == NULL || nPos > NumOnList( LHead ) ) 
          return( FALSE );

     if ( nPos == 0 )
          AddHead( LHead, LInfo );
     else if ( nPos == NumOnList( LHead ) ) 
          AddTail( LHead, LInfo );
     else
     {
          if ( (LAddNode = PeekList( LHead, LISTHEAD, nPos - 1 )) == NULL )
               return( FALSE );
          
          ((PLISTINFO)LAddNode->ListNode.Next)->ListNode.Previous = LInfo;
          LInfo->ListNode.Next      = LAddNode->ListNode.Next;
          LInfo->ListNode.Previous  = LAddNode;
          LAddNode->ListNode.Next   = LInfo;
          
          LHead->NumList++;
     }

     return( TRUE );
}




/*----------------------------------------------------------------------------
 *  RemHead:
 */
PLISTINFO  RemHead(PLISTHEAD LHead)
{
     PLISTINFO t, t1;

     if ( LHead == NULL || EMPTYLIST(LHead) )
          return(NULL);

     t = LHead->LHeaders[LISTHEAD];
     LHead->LHeaders[LISTHEAD] = (PLISTINFO) t->ListNode.Next;

     if (LHead->LHeaders[LISTHEAD] != NULL)
     {
          t1 = (PLISTINFO) t->ListNode.Next;
          t1->ListNode.Previous = NULL;
     }
     else
          LHead->LHeaders[LISTTAIL] = NULL;

     LHead->NumList--;

     return(t);
}

/*----------------------------------------------------------------------------
 *  RemTail:
 */
PLISTINFO  RemTail(PLISTHEAD   LHead)
{
     PLISTINFO   t, t1;

     if ( LHead == NULL || EMPTYLIST(LHead) )
          return(NULL);

     t = LHead->LHeaders[LISTTAIL];
     LHead->LHeaders[LISTTAIL] = (PLISTINFO) t->ListNode.Previous;
     if (LHead->LHeaders[LISTTAIL] != NULL)
     {
          t1 = (PLISTINFO) t->ListNode.Previous;
          t1->ListNode.Next = NULL;
     }
     else
          LHead->LHeaders[LISTHEAD] = NULL;

     LHead->NumList--;
     return(t);
}

/*----------------------------------------------------------------------------
 * PeekList:
 */
PLISTINFO  PeekList(PLISTHEAD LHead, int wch, int index )
{
     PLISTINFO  t;

     if (LHead == NULL)
          return(NULL);
     if ( (t = LHead->LHeaders[wch]) == NULL )
          return(NULL);

     for (; t != NULL && index > 0; index-- )
          t = (wch == LISTHEAD)  ? (PLISTINFO) t->ListNode.Next  :
                                   (PLISTINFO) t->ListNode.Previous;
     return(t);
}


/*----------------------------------------------------------------------------
 * RemoveList:
 */
PLISTINFO   RemoveList( PLISTHEAD LHead, PLISTINFO LInfo )
{
     PLISTINFO     t, t1;

     t = LInfo;
     if (LHead == NULL)
          return(NULL);
     if (LHead->LHeaders[LISTHEAD] == t)
          t = (PLISTINFO) RemHead(LHead);
     else if (LHead->LHeaders[LISTTAIL] == t)
          t = (PLISTINFO) RemTail(LHead);
     else
     {
          t1                    = (PLISTINFO) t->ListNode.Previous;
          t1->ListNode.Next     = t->ListNode.Next;
          t1                    = (PLISTINFO) t->ListNode.Next;
          t1->ListNode.Previous = t->ListNode.Previous;
          LHead->NumList--;
     }

     return(t);
}

/*----------------------------------------------------------------------------
 * SearchList:
 *       Try to find a specific node in the queue whose key matches with
 *  searching key. Return the pointer to that node if found, return NULL
 *  otherwise
 *
 *  Input:
 *    lpHashTbl       => a far pointer to the hash table
 *    lpKey           => a far poniter to searching key
 *    CompareCallBack => comparision function
 *
 *  Output: a far pointer to the node to be found
 *
 */
PLISTINFO  SearchList(
                        PLISTHEAD lpListHead,
                        PVOID lpSKey,
                        int (* CompareCallBack) ( PVOID, PVOID ) )
{
PLISTINFO lpListInfo;

     lpListInfo = PeekList( lpListHead, LISTHEAD, 0);
     while ( lpListInfo != NULL )
     {
          if ( CompareCallBack( lpListInfo, lpSKey ) )
               break;
          lpListInfo = GetNextNode( lpListInfo );
     }

     return( lpListInfo );
}
 
