#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<locale.h>
#include<sys/time.h>
#include<sys/stat.h>
#include "my402list.h"
#include "cs402.h"

int  My402ListLength(My402List* list)
{
	return list->num_members;
}
int  My402ListEmpty(My402List* list)
{
	if(list->num_members==0)
		return TRUE;
	else
		return FALSE;
}
int  My402ListAppend(My402List* list, void* object)
{
	My402ListElem *temp= (My402ListElem*)malloc(sizeof(My402ListElem));

	if(temp==NULL)
		return FALSE;

	temp->obj=object;
	//If there is no element in the list. Add the element temp.
	if(list->num_members==0)
	{
		temp->next=&(list->anchor);
		temp->prev=&(list->anchor);
		list->anchor.next=temp;
		list->anchor.prev=temp;
		list->num_members++;
		return TRUE;
	}
	//If there is any element in the list, add the element at the last.
	else
	{
		temp->next=&(list->anchor);
		temp->prev=list->anchor.prev;
		(list->anchor.prev)->next=temp;
		list->anchor.prev=temp;
		list->num_members++;
		return TRUE;
	}
	return FALSE;
}
int  My402ListPrepend(My402List* list, void* object)
{
	My402ListElem *temp= (My402ListElem*)malloc(sizeof(My402ListElem));
	if(temp==NULL)
		return FALSE;
	temp->obj=object;
	//If there is only no element in the list, Add the element.
	if(list->num_members==0)
	{
		temp->next=&(list->anchor);
		temp->prev=&(list->anchor);
		list->anchor.next=temp;
		list->anchor.prev=temp;
		list->num_members++;
		return TRUE;
	}
	else
	{
		temp->next=list->anchor.next;
		temp->prev=&(list->anchor);
		(list->anchor.next)->prev=temp;
		list->anchor.next=temp;
		list->num_members++;
		return TRUE;
	}
	return FALSE;
}
My402ListElem *My402ListFirst(My402List* list)
{
	if(list->num_members==0)
		return NULL;
	return (list->anchor.next);
}
My402ListElem *My402ListLast(My402List* list)
{
	if(list->num_members==0)
		return NULL;
	return (list->anchor.prev);
}
void My402ListUnlink(My402List* list, My402ListElem* elem)
{

	elem->prev->next=elem->next;
	elem->next->prev=elem->prev;
	free(elem);
	list->num_members--;
	return;
}
void My402ListUnlinkAll(My402List* list)
{
	if(list==NULL)
		return;
	if(list->anchor.next== &(list->anchor))
		return;
	My402ListElem *element=list->anchor.next;
	My402ListElem *elementnext=element->next;
	while(element->next!= list->anchor.next)
	{
		elementnext=element->next;
		list->anchor.next=elementnext;
		elementnext->prev=&(list->anchor);
		free(element);
		element=elementnext;
	}
	list->anchor.next=&(list->anchor);
	list->anchor.prev=&(list->anchor);
	list->num_members=0;
	return;
}
int  My402ListInsertAfter(My402List* list, void* object, My402ListElem* elem)
{
	My402ListElem *temp=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(temp==NULL)
		return FALSE;
	if(elem==NULL)
		return (My402ListAppend(list,object));
	temp->obj=object;
	temp->next=elem->next;
	temp->prev=elem;
	elem->next->prev=temp;
	elem->next=temp;
	list->num_members++;
	return TRUE;
}
int  My402ListInsertBefore(My402List* list, void* object, My402ListElem* elem)
{
	My402ListElem *temp=(My402ListElem*) malloc(sizeof(My402ListElem));
	if(temp==NULL)
		return FALSE;
	if(elem==NULL)
		return (My402ListPrepend(list,object));

	temp->obj=object;
	temp->next=elem;
	temp->prev=elem->prev;
	elem->prev->next=temp;
	elem->prev=temp;
	list->num_members++;
	return TRUE;

}
My402ListElem *My402ListNext(My402List* list, My402ListElem* elem)
{
	if(elem->next== &(list->anchor))
		return NULL;
	return elem->next;
}
My402ListElem *My402ListPrev(My402List* list, My402ListElem* elem)
{
	if(elem->prev== &(list->anchor))
		return NULL;
	return elem->prev;
}
My402ListElem *My402ListFind(My402List* list, void* object)
{
	My402ListElem* temp=list->anchor.next;
	while(temp->next != list->anchor.next)
	{
		if(temp->obj==object)
			return temp;
		temp=temp->next;
	}
	return NULL;
}
int My402ListInit(My402List* list)
{
	list->num_members=0;
	list->anchor.next= &(list->anchor);
	list->anchor.prev= &(list->anchor);
	return TRUE;
}
