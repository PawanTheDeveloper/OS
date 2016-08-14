/*
 * warmup1.c
 *
 *  Created on: Jan 29, 2016
 *      Author: pawan
 */
/*
 * This is the Sorting part of the assignment.
 */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<locale.h>
#include<sys/time.h>
#include<ctype.h>
#include<sys/stat.h>
#include<unistd.h>
#include"my402list.h"

typedef struct tagMy402ElemData {
    char type;
    time_t timestamp;
    unsigned long amount;
    char *description;
} Data;


void RemoveSpacesFromDesciption(char *str)
{
	int i=0;
	for (i=0; isspace(str[i]); i++)
	{
		//Do Nothing , here , just parse the spaces.
	}
	str=str+i;
}
void VerifyType(char symbol)
{
	if(symbol != '-' && symbol != '+')
	{
		fprintf(stderr,"Invalid Symbol, Transaction can have only + and -, Exitting!!\n");
		exit(1);
	}
}


void VerifyTime(time_t times)
{
	time_t current_t;
	current_t=time(NULL);
	if(current_t <= times)
	{
		fprintf(stderr,"Timestamp greater than the current time.\n");
		exit(1);
	}

}
void SeperateIntegerAndDecimalValues(char *amount_value,unsigned long *current_amount)
{
	char *integer_part=(char*) malloc(sizeof(10));
	char *decimal_part=(char*)malloc(sizeof(2));
	integer_part=strtok(amount_value,".");
	decimal_part=strtok(NULL,"");
	if(*amount_value=='.')
	{
		fprintf(stderr,"Integer Part Doesn't Exists, Amount cannot be NULL");
		exit(1);
	}
	if(strlen(decimal_part)==1)
	{
		strcat(decimal_part,"0");
	}
	if(strlen(decimal_part)>2)
	{
		fprintf(stderr,"Decimal part cannot have more than 2 decimal points\n");
		exit(1);
	}
	if(strlen(integer_part)>7)
	{
		fprintf(stderr,"Integer Part cannot be more than 10 Millions");
		exit(1);
	}
	strcat(integer_part,decimal_part);
	*current_amount=(unsigned long)atol(integer_part);
	return;
}
Data *CreateElementObj(My402List* list,Data* object, char buff[])
{
	char *amount_value=(char*)malloc(sizeof(14));
	*amount_value='\n';
	char *values[5];
	unsigned long current_amount=0;
	values[0]=strtok(buff,"\t");
	if(values[0]==NULL)
	{
		fprintf(stderr,"Too Few Parameters\n");
		exit(1);
	}
	object->type=*(values[0]);
	values[1]=strtok(NULL,"\t");
	if(values[1]==NULL)
	{
		fprintf(stderr,"Too Few Parameters\n");
		exit(1);
	}
	if(strlen(values[1])<10 ||strlen(values[1])>10)
	{
		fprintf(stderr,"Timestamp should be 10 character long");
	}
	object->timestamp=atoi(values[1]);
	values[2]=strtok(NULL,"\t");
	if(values[2]==NULL)
	{
		fprintf(stderr,"Too Few Parameters\n");
		exit(1);
	}
	amount_value=values[2];
	values[3]=strtok(NULL,"\t\n");
	if(values[3]==NULL)
	{
		fprintf(stderr,"Too Few Parameters\n");
		exit(1);
	}
	object->description=strdup(values[3]);
	if(strtok(NULL,"\t")!=NULL)
	{
		fprintf(stderr,"Too Many parameters in Transaction.\n");
		exit(1);
	}
	VerifyType(object->type);
	if(object->timestamp<0)
	{
		fprintf(stderr,"Error in Transaction.\n");
		exit(1);
	}
	VerifyTime(object->timestamp);
	if(amount_value == NULL)
	{
		fprintf(stderr,"Error in Transaction.\n");
		exit(1);
	}

	if(object->description==NULL)
	{
			fprintf(stderr,"Error in Transaction\n");
			exit(1);
	}

	SeperateIntegerAndDecimalValues(amount_value,&current_amount);
	RemoveSpacesFromDesciption(object->description);
	if(strlen(object->description)==0)
	{
		fprintf(stderr,"Description cannot be empty\n");
		exit(1);
	}
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
            	fprintf(stderr,"Two Timestamps cannot be same. Exitting!!\n");
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
	setlocale(LC_NUMERIC,"en_US");
	struct tm *loctime;
	while(elem->next!=list->anchor.next)
	{
		char *date=(char*)malloc(sizeof(15));
		char *decimal_value=(char*)malloc(sizeof(2));
		char *balance_decimal=(char*)malloc(sizeof(2));
		type=((Data*)elem->obj)->type;
		loctime = localtime (&((Data*)elem->obj)->timestamp);
		strftime(date,256,"%a %b %e %Y",loctime);
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
		long tb=balance/100;
		if((tb<1000000000) && (tb > -1000000000))
		{

			if(type=='+' && flag==0)
				printf("| %14s | %-24s | %'10lu.%s  | %'10ld.%s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance/100,balance_decimal);
			else if(type=='+' && flag==1)
				printf("| %14s | %-24s | %'10lu.%s  | (%'9lu.%s) |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,tempbalance/100,balance_decimal);
			else if(type=='-' && flag==0)
				printf("| %14s | %-24s | (%'9lu.%s) | %'10ld.%s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance/100,balance_decimal);
			else
				printf("| %14s | %-24s | (%'9lu.%s) | (%'9lu.%s) |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,tempbalance/100,balance_decimal);
		}
		else
		{
			char *balance_string="?,???,???.??";
			if(type=='+' && flag==0)
				printf("| %14s | %-24s | %'10lu.%s  |  %s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance_string);
			else if(type=='+' && flag==1)
				printf("| %14s | %-24s | %'10lu.%s  |  %s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance_string);
			else if(type=='-' && flag==0)
				printf("| %14s | %-24s | (%'9lu.%s) |  %s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance_string);
			else
				printf("| %14s | %-24s | (%'9lu.%s) |  %s  |\n",date,((Data*)elem->obj)->description,current_value/100,decimal_value,balance_string);
		}


		elem=elem->next;
	}
	printf("+-----------------+--------------------------+----------------+----------------+\n");
}

int main(int argc, char* argv[])
{
	FILE *fp;
	My402List list;
	char buff[1030];
	char * filename;

	if(argc==0 || argc==1)
	{
		fprintf(stderr,"Insufficient Command Arguments\n");
		exit(1);
	}
	if(argc==2)
	{
		if(!strcmp(argv[1],"sort"))
			fp=stdin;
		else
		{
			fprintf(stderr,"Sort should be present as second Argument.\n");
			exit(1);
		}
	}

	else if(argc==3)
	{
		filename=argv[2];
		if(strcmp(argv[1],"sort")!=0)
		{
			fprintf(stderr,"Sort should be present as second Argument\n");
			exit(1);
		}
		struct stat s;
		if((access(filename,F_OK) ==-1))
		{
			fprintf(stderr,"File doesn't exist\n");
			exit(1);
		}
		if((access(filename,R_OK) ==-1))
		{
			fprintf(stderr,"File no Read Permission.\n");
			exit(1);
		}
		if(!stat(filename,&s))
		{
			if(s.st_mode & S_IFDIR)
			{
				fprintf(stderr,"It's a Directory\n");
				exit(1);
			}
		}
		fp=fopen(filename,"r");

		if(fp==NULL)
		{
			fprintf(stderr,"File pointer is NULL. Exitting!!\n");
			exit(1);
		}


	}
	else
	{
		fprintf(stderr,"\nError in Command Line\n");
		exit(1);
	}

	if(!My402ListInit(&list))
	{
		fprintf(stderr,"Not able to Initialize List. Exitting!!\n");
		exit(1);
	}
	while(fgets(buff,sizeof(buff),fp)!=NULL)
	{
		if(strlen(buff)>1024)
		{
			fprintf(stderr,"Transaction Malformed, Too Big!!, Exitting\n");
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
