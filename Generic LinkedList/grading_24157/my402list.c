#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<locale.h>
#include<sys/time.h>
#include<sys/stat.h>
#include "my402list.h"
#include "cs402.h"

/*
 * This is Doubly Linked List implementation and the first part of  the Assignment.
 */
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


/*
 * This is the Sorting part of the assignment.
 */
typedef struct tagMy402ElemData {
    char type;
    time_t timestamp;
    unsigned long amount;
    char *description;
} Data;

void VerifyType(char symbol)
{
	if(symbol != '-' && symbol != '+')
	{
		perror("Invalid Symbol, Transaction can have only + and -, Exitting!!");
		exit(1);
	}
}

/*
void VerifyTime(time_t time)
{
	int difference=0;
	//time(NULL);
	//difference=difftime(time,current_time);
	if(difference<0)
	{
		perror("One of the Timestamp is greater than the current time,Exitting!!\n");
		exit(1);
	}
	return;
}*/
void SeperateIntegerAndDecimalValues(char *amount_value,unsigned long *current_amount)
{
	char *integer_part=(char*) malloc(sizeof(10));
	char *decimal_part=(char*)malloc(sizeof(2));
	integer_part=strtok(amount_value,".");
	decimal_part=strtok(NULL,"");
	strcat(integer_part,decimal_part);
	*current_amount=(unsigned long)atol(integer_part);
	return;
}
Data *CreateElementObj(My402List* list,Data* object, char buff[])
{
	char *amount_value=(char*)malloc(sizeof(14));
	unsigned long current_amount;
	object->type=*(strtok(buff,"\t"));
	VerifyType(object->type);
	object->timestamp=atoi(strtok(NULL,"\t"));
	//VerifyTime(object->timestamp);
	amount_value=strtok(NULL,"\t");

	object->description=strdup(strtok(NULL,"\t\n"));
	SeperateIntegerAndDecimalValues(amount_value,&current_amount);
	if(strlen(object->description)>24)
	{
		object->description[24]='\0';
	}
	object->amount=current_amount;
	return object;
}

void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)
{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

void SortTheList(My402List *pList, int num_items)
{
    My402ListElem *elem=NULL;
    int i=0;

    if (My402ListLength(pList) != num_items) {
        fprintf(stderr, "List length is not %1d in BubbleSortForwardList().\n", num_items);
        exit(1);
    }
    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;

        for (elem=My402ListFirst(pList), j=0; j < num_items-i-1; elem=next_elem, j++) {
            int cur_val=(int)(((Data*)elem->obj)->timestamp), next_val=0;

            next_elem=My402ListNext(pList, elem);
            next_val = (int)(((Data*)elem->next->obj)->timestamp);

            if(cur_val==next_val)
            {
            	perror("Two Timestamps cannot be same. Exitting!!");
            	exit(1);
            }
            if(cur_val > next_val)
            {
                BubbleForward(pList, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}

void GetAmountValues(unsigned long current_value,unsigned long *integer_value,char *decimal_value)
{
	unsigned int last=0, secondlast=0;
	*integer_value=current_value/100;
	last=(unsigned int)current_value%10;
	current_value/=10;
	secondlast=(unsigned int)current_value%10;
	sprintf(decimal_value,"%u%u",secondlast,last);
	return;
}

int GetBalance(char type,unsigned long current_value,long *balance)
{
	if(type=='-')
		*balance=*balance + (current_value*-1);
	else
		*balance=*balance+current_value;
	if(*balance<0)
		return 1;
	else
		return 0;
}
void PrintOut(My402List *list)
{
	long balance=0;
	unsigned long tempbalance;
	int flag=0;
	char type;
	My402ListElem *elem=list->anchor.next;
	printf("+-----------------+--------------------------+----------------+----------------+\n");
	printf("|       Date      | Description              |         Amount |        Balance |\n");
	printf("+-----------------+--------------------------+----------------+----------------+\n");
	setlocale(LC_ALL,"");
	struct tm *loctime;
	while(elem->next!=list->anchor.next)
	{
		char *date=(char*)malloc(sizeof(15));
		char *decimal_value=(char*)malloc(sizeof(2));
		char *balance_decimal=(char*)malloc(sizeof(2));
		type=((Data*)elem->obj)->type;
		loctime = localtime (&((Data*)elem->obj)->timestamp);
		strftime(date,256,"%a %b %d %Y",loctime);
		unsigned long current_value=((Data*)elem->obj)->amount;
		unsigned long integer_value=0;
		unsigned long balance_integer=0;

		GetAmountValues(current_value,&integer_value,decimal_value);
		flag = GetBalance(type,current_value,&balance);

		if(flag)
			tempbalance=balance * -1;
		else
			tempbalance=balance;
		GetAmountValues(tempbalance,&balance_integer,balance_decimal);

		if(type=='+' && flag==0)
			printf("| %14s | %24s | %'10lu.%s  | %'10ld.%s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance/100,balance_decimal);
		else if(type=='+' && flag==1)
			printf("| %14s | %24s | %'10lu.%s  | ( %'9lu.%s) |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,tempbalance/100,balance_decimal);
		else if(type=='-' && flag==0)
			printf("| %14s | %24s | (%'9lu.%s) | %'10ld.%s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance/100,balance_decimal);
		else
			printf("| %14s | %24s | (%'9lu.%s) | (%'9lu.%s) |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,tempbalance/100,balance_decimal);

		elem=elem->next;
	}
	printf("+-----------------+--------------------------+----------------+----------------+\n");
}

int main(int argc, char* argv[])
{
	FILE *fp;
	char buff[85];
	if(argc==0)
	{
		fp=stdin;
	}
	if(argc==0 || argc==1)
	{
		fprintf(stderr,"Insufficient Command Arguments");
		exit(1);
	}
	if(argc==2)
	{
		if(strcmp(argv[1],"sort")==0)
		{
			fp=stdin;
		}
		else
			fprintf(stderr,"Error");
	}
	fp=fopen("tfile","r");
	if(fp==NULL)
	{
		perror("File pointer is NULL. Exitting!!\n");
		exit(1);
	}
	My402List list;
	if(!My402ListInit(&list))
	{
		perror("Not able to Initialize List. Exitting!!\n");
		exit(1);
	}
	while(fgets(buff,sizeof(buff),fp)!=NULL)
	{
		if(strlen(buff)>81)
		{
			perror("Transaction Malformed, Too Big!!, Exitting\n");
			exit(1);
		}
		Data *object=(Data*)malloc(sizeof(Data));
		CreateElementObj(&list,object,buff);
		My402ListAppend(&list,object);
		SortTheList(&list,list.num_members);
	}
	PrintOut(&list);
	return 0;
}
