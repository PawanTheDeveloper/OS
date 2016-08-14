#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/time.h>
#include<getopt.h>
#include<pthread.h>
#include<ctype.h>
#include<string.h>
#include<signal.h>
#include "my402list.h"
#include "cs402.h"

#define Max_Value = 256;
int total_packet_count=0;
int total_token_count=0;
int current_token_number=0;
int dropped_token_count=0;
int dropped_packet_count=0;
int packet_served=0;
int packets_processed_by_token_and_packet_thread=0;
int bucketthreadstop=0;
int stopservers=0;
int stop_packet_thread=0;
int stop_bucket_thread=0;
int stopserver1=0;
int stopserver2=0;
unsigned int fileflag=0;
double program_start_time_in_double=0;
double relative_time=0;
double sum_interval_arrival=0;
double sum_q1_service_time=0;
double sum_q2_service_time=0;
double sum_S1_service_time=0;
double sum_S2_service_time=0;
double sum_system_service_time=0;
double sum_system_service_time_square=0;
struct timeval program_start_time,program_exit_time;
My402List Q1_list,Q2_list,token_list;
pthread_t bucket_thread,packet_thread,server1_thread,server2_thread,handle_thread;
pthread_mutex_t token_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t server_wait = PTHREAD_COND_INITIALIZER;
FILE *fp=NULL;
sigset_t set;
typedef struct
{
	int packet_id;
	int required_token;
	double required_service_time;
	struct timeval packet_arrival,q1_arrival_time,q1_exit_time,q2_arrival_time,q2_exit_time,exit_time;
	double arrival_time,q1_service_time,q2_service_time,total_service_time;
}Packet;
typedef struct
{
	char *filename;
	double packet_arrival;
	double serving_time;
	double token_arrival;
	int bucket_size;
	int token_required;
	int packet_count;

}Parameters;

static struct option long_Options[] = {
{"lambda", required_argument, 0, 'l'},
{"mu", required_argument,0, 'm'},
{"r", required_argument, 0, 'r'},
{"B", required_argument, 0, 'B'},
{"P", required_argument, 0, 'P'},
{"n", required_argument, 0, 'n'},
{"t", required_argument, 0, 't'},
{0,0,0,0}
};
double time_difference_in_double(struct timeval now_time)
{
	//Get everything in microseconds, before sending the double value, convert the time into milliseconds , that is divide by 1000.
	double value1,value2,value3;
	value1=now_time.tv_sec*1000000 +now_time.tv_usec;
	value2=program_start_time.tv_sec*1000000 + program_start_time.tv_usec;
	value3=(value1-value2)/1000;
	return value3;
}
void fill_packet_data_deterministically(Packet *newpacket,Parameters *argobj)
{
	total_packet_count++;
	newpacket->packet_id=total_packet_count;

	if(argobj->packet_arrival!=0)
		newpacket->arrival_time=(1/(argobj->packet_arrival))*1000;

	if(argobj->serving_time!=0)
		newpacket->required_service_time=(1/(argobj->serving_time))*1000;

	newpacket->required_token=argobj->token_required;
}
void fill_packet_data_tracefile(Packet *newpacket)
{
	char line[1024];
	char *token;
	if(fgets(line, sizeof(line), fp) != NULL)
	{
		if(isspace(*line) || *line == '\t')
		{
			fprintf(stderr, "Error: Contains leading spaces\n");
			exit(0);
		}
		token = strtok(line," /t");
		if(token!= NULL)
		{
			newpacket->arrival_time = atoi(token);
			if(newpacket->arrival_time <= 0)
			{
				fprintf(stderr,"Error while parsing ");
			}
		}
		else
		{
			fprintf(stderr, "Error while parsing file parameters\n");
			exit(0);
		}
		token = strtok(NULL," /t");
		if (token != NULL)
			newpacket->required_token = atoi(token);
		else
		{
			fprintf(stderr, "Error while parsing file parameters\n");
			exit(0);
		}
		token = strtok(NULL," /t");
		if (token != NULL)
			newpacket->required_service_time = atoi(token);
		else
		{
			fprintf(stderr, "Error while parsing file parameters\n");
			exit(0);
		}

		token= strtok(NULL," /t");
		if(token != NULL)
		{
			printf("Error:Extra Paramets in the line\n");
			exit(1);
		}
	}
	total_packet_count++;
	newpacket->packet_id=total_packet_count;
}
void packet_thread_function(Parameters *argobj)
{
	double inter_arrival=0;
	double packet_arrival_in_double=0;
	double previous_packet_arrival=program_start_time_in_double;
	int requiredtoken=0;
	double q1_leaving_time=0;
	double q1_arrival_time=0;
	//double q1_service_time=0;
	double q2_arrival_time;
	int i=0;
	while(1)
	{
		if(i>=argobj->packet_count)
		{
			stop_packet_thread=1;
			pthread_cond_broadcast(&server_wait);
			break;
		}
		Packet *newpacket=(Packet*)malloc(sizeof(Packet));

		if(!fileflag)
			fill_packet_data_deterministically(newpacket,argobj);
		else
			fill_packet_data_tracefile(newpacket);

		if(newpacket->arrival_time>10000)
			newpacket->arrival_time=10000;

		usleep((newpacket->arrival_time*1000));

		gettimeofday(&(newpacket->packet_arrival), NULL);

		packet_arrival_in_double=time_difference_in_double(newpacket->packet_arrival);

		inter_arrival = packet_arrival_in_double - previous_packet_arrival;

		previous_packet_arrival=packet_arrival_in_double;

		sum_interval_arrival+=(inter_arrival);

		if ( newpacket->required_token > argobj->bucket_size )
		{
			printf("%012.3fms: p%d dropped, requires %d token, Bucket size: %d\n",packet_arrival_in_double,newpacket->packet_id,newpacket->required_token,argobj->bucket_size);
			free(newpacket);
			dropped_packet_count++;
			packets_processed_by_token_and_packet_thread++;
			i++;
			continue;
		}
		else
		{
			pthread_mutex_lock(&(token_mutex));
			printf("%012.3fms: p%d arrives, requires %d tokens,inter arrival time= %.3fms\n",packet_arrival_in_double,newpacket->packet_id,newpacket->required_token,inter_arrival);
			My402ListAppend(&Q1_list,newpacket);
			gettimeofday(&(newpacket->q1_arrival_time), NULL);
			q1_arrival_time=time_difference_in_double(newpacket->q1_arrival_time);
			printf("%012.3fms: p%d enters Q1 \n",q1_arrival_time,newpacket->packet_id);

			if ( My402ListLength(&Q1_list) > 0 )
			{
				My402ListElem *tempElem = My402ListFirst(&Q1_list);
				Packet *temppacket = (Packet*)tempElem->obj;
				requiredtoken=temppacket->required_token;
				if ( requiredtoken <= current_token_number)
				{
					My402ListUnlink(&Q1_list,tempElem);
					current_token_number = current_token_number - requiredtoken;
					gettimeofday(&temppacket->q1_exit_time, NULL);
					q1_leaving_time=time_difference_in_double(temppacket->q1_exit_time);
					temppacket->q1_service_time=q1_leaving_time-time_difference_in_double(temppacket->q1_arrival_time);
					printf("%012.3fms: p%d leaves Q1,time in Q1 = %.3fms, token bucket now has %d token\n",q1_leaving_time,temppacket->packet_id,temppacket->q1_service_time, current_token_number);
					My402ListAppend(&Q2_list, temppacket);
					packets_processed_by_token_and_packet_thread++;
					gettimeofday(&temppacket->q2_arrival_time, NULL);
					q2_arrival_time=time_difference_in_double(temppacket->q2_arrival_time);
					printf("%012.3fms: p%d arrives Q2\n",q2_arrival_time,temppacket->packet_id);
					if(My402ListLength(&Q2_list)>=1)
						pthread_cond_broadcast(&server_wait);
				}
			}
			pthread_mutex_unlock(&token_mutex);
		}
		i++;
	}
}

void bucket_thread_function(Parameters *argobj)
{
	double q1_leaving_time=0;
	//double q1_service_time=0;
	double q2_arrival_time_difference=0;
	double time_dropped,token_generated;
	struct timeval now_time;
	double token_rate= (1/(argobj->token_arrival))*1000;
	if(token_rate>10000)
		token_rate=10000;
	My402ListElem *tempElement=(My402ListElem*)malloc(sizeof(My402ListElem));
	Packet *tempPacket=(Packet*)malloc(sizeof(Packet));
	while(1)
	{
		if(stop_packet_thread==1 && My402ListLength(&Q1_list)==0)
		{
			stop_bucket_thread=1;
			pthread_cond_broadcast(&server_wait);
			break;
		}
		usleep(token_rate*1000);
		pthread_mutex_lock(&token_mutex);
		total_token_count++;
		if(current_token_number>=(argobj->bucket_size))
		{
			dropped_token_count++;
			gettimeofday(&now_time,0);
			time_dropped=time_difference_in_double(now_time);
			printf("%012.3fms: token t%d token dropped\n",time_dropped,total_token_count);
			pthread_mutex_unlock(&token_mutex);
		}

		else
		{
			current_token_number++;
			gettimeofday(&now_time,0);
			token_generated=time_difference_in_double(now_time);
			printf("%012.3fms: token t%d arrives, Bucket holds %d tokens\n",token_generated,total_token_count,current_token_number);

			if(My402ListLength(&Q1_list)>0)
			{
				tempElement=My402ListFirst(&Q1_list);
				tempPacket = (Packet*)tempElement->obj;
				if(tempPacket->required_token <= current_token_number)
				{
					current_token_number=current_token_number-tempPacket->required_token;
					My402ListUnlink(&Q1_list,tempElement);
					gettimeofday(&(tempPacket->q1_exit_time),0);
					q1_leaving_time=time_difference_in_double(tempPacket->q1_exit_time);
					tempPacket->q1_service_time=q1_leaving_time-time_difference_in_double(tempPacket->q1_arrival_time);
					printf("%012.3fms: p%d leaves Q1,time in Q1 = %.3fms, token bucket now has %d token\n",q1_leaving_time,tempPacket->packet_id,tempPacket->q1_service_time, current_token_number);
					My402ListAppend(&Q2_list,tempPacket);
					packets_processed_by_token_and_packet_thread++;
					gettimeofday(&tempPacket->q2_arrival_time, NULL);
					q2_arrival_time_difference=time_difference_in_double(tempPacket->q2_arrival_time);
					printf("%012.3fms: p%d enters Q2\n",q2_arrival_time_difference,tempPacket->packet_id);

					if(My402ListLength(&Q2_list)>=1)
					{
						pthread_cond_broadcast(&server_wait);
					}
				}
			}
			pthread_mutex_unlock(&token_mutex);
		}
	}
}

void server1_thread_function(Parameters *argobj)
{
	My402ListElem *tempElement=(My402ListElem*) malloc (sizeof(My402ListElem));
	Packet *tempPacket=(Packet*) malloc (sizeof(Packet));
	double s1_service_time=0;
	double q2_exit_time=0;

	while(1)
	{
		pthread_mutex_lock(&token_mutex);
		while((My402ListLength(&Q2_list)==0) && (stop_bucket_thread==0 || stop_packet_thread==0))
		{
			 pthread_cond_wait(&server_wait, &token_mutex);
		}

		if(My402ListLength(&Q2_list)>0)
		{
			tempElement = My402ListFirst(&Q2_list);
			if(tempElement == NULL)
			{
				pthread_mutex_unlock(&token_mutex);
				continue;
			}
			tempPacket = (Packet*)tempElement->obj;
			My402ListUnlink(&Q2_list, tempElement);
			pthread_mutex_unlock(&token_mutex);

			gettimeofday(&(tempPacket->q2_exit_time), NULL);
			q2_exit_time=time_difference_in_double(tempPacket->q2_exit_time);

			tempPacket->q2_service_time=time_difference_in_double(tempPacket->q2_exit_time)-time_difference_in_double(tempPacket->q2_arrival_time);
			printf("%012.3fms: p%d leaves Q2,time in Q2 = %.3fms\n",q2_exit_time,tempPacket->packet_id,tempPacket->q2_service_time);

			printf("%012.3fms: p%d begin service at S1,requesting %.3fms of service\n",time_difference_in_double(tempPacket->q2_exit_time),tempPacket->packet_id,tempPacket->required_service_time);
			usleep((tempPacket->required_service_time)*1000);

			gettimeofday(&(tempPacket->exit_time), NULL);
			tempPacket->total_service_time = time_difference_in_double((tempPacket->exit_time))- time_difference_in_double((tempPacket->q1_arrival_time));
			s1_service_time=time_difference_in_double(tempPacket->exit_time)-time_difference_in_double(tempPacket->q2_exit_time);
			printf("%012.3fms: p%d departs from S1, service time = %.3fms, time in system = %.3fms\n",time_difference_in_double(tempPacket->exit_time),tempPacket->packet_id,s1_service_time,tempPacket->total_service_time);
			sum_q1_service_time+=tempPacket->q1_service_time;
			sum_q2_service_time+=tempPacket->q2_service_time;
			sum_S1_service_time+=s1_service_time;
			sum_system_service_time+=tempPacket->total_service_time;
			sum_system_service_time_square+=tempPacket->total_service_time*tempPacket->total_service_time;
			packet_served ++;
		}
		if(stop_bucket_thread==1 && stop_packet_thread==1 && (My402ListLength(&Q2_list)==0))
		{

			pthread_mutex_unlock(&token_mutex);
			stopserver1=1;
			break;
		}
	}
}

void server2_thread_function(Parameters *argobj)
{
	My402ListElem *tempElement=(My402ListElem*) malloc(sizeof(My402ListElem));
	Packet *tempPacket=(Packet*) malloc(sizeof(Packet));
	double s2_service_time=0;
	double q2_exit_time=0;

	while(1)
	{
		pthread_mutex_lock(&token_mutex);

		while((My402ListLength(&Q2_list)==0) && (stop_bucket_thread==0 || stop_packet_thread==0))
		{
			 pthread_cond_wait(&server_wait, &token_mutex);
		}

		if(My402ListLength(&Q2_list)>0)
		{
			tempElement = My402ListFirst(&Q2_list);
			if(tempElement == NULL)
			{
				pthread_mutex_unlock(&token_mutex);
				continue;
			}
			tempPacket = (Packet*)tempElement->obj;
			My402ListUnlink(&Q2_list, tempElement);
			pthread_mutex_unlock(&token_mutex);

			gettimeofday(&(tempPacket->q2_exit_time), NULL);
			q2_exit_time=time_difference_in_double(tempPacket->q2_exit_time);

			tempPacket->q2_service_time=time_difference_in_double(tempPacket->q2_exit_time)-time_difference_in_double(tempPacket->q2_arrival_time);
			printf("%012.3fms: p%d leaves Q2,time in Q2 = %.3fms\n",q2_exit_time,tempPacket->packet_id,tempPacket->q2_service_time);

			printf("%012.3fms: p%d begin service at S2,requesting %.3fms of service\n",time_difference_in_double(tempPacket->q2_exit_time),tempPacket->packet_id,tempPacket->required_service_time);
			usleep((tempPacket->required_service_time)*1000);

			gettimeofday(&(tempPacket->exit_time), NULL);
			tempPacket->total_service_time = time_difference_in_double((tempPacket->exit_time))- time_difference_in_double((tempPacket->q1_arrival_time));
			s2_service_time=time_difference_in_double(tempPacket->exit_time)-time_difference_in_double(tempPacket->q2_exit_time);
			printf("%012.3fms: p%d departs from S2, service time = %.3fms, time in system = %.3fms\n",time_difference_in_double(tempPacket->exit_time),tempPacket->packet_id,s2_service_time,tempPacket->total_service_time);
			sum_q1_service_time+=tempPacket->q1_service_time;
			sum_q2_service_time+=tempPacket->q2_service_time;
			sum_S2_service_time+=s2_service_time;
			sum_system_service_time+=tempPacket->total_service_time;
			sum_system_service_time_square+=tempPacket->total_service_time*tempPacket->total_service_time;
			packet_served ++;
		}
		if(stop_bucket_thread==1 && stop_packet_thread==1 && (My402ListLength(&Q2_list)==0))
		{

			pthread_mutex_unlock(&token_mutex);
			stopserver2=1;
			break;
		}
	}
}
void setdefaults(Parameters *data)
{
	if(!data->packet_arrival)
		data->packet_arrival=1;
	if(!data->serving_time)
		data->serving_time=0.35;
	if(!data->token_arrival)
		data->token_arrival=1.5;
	if(!data->bucket_size)
		data->bucket_size=10;
	if(!data->token_required)
		data->token_required=3;
	if(!data->packet_count)
		data->packet_count=20;
}

void PrintFirstLine(Parameters *data)
{
	if(fileflag)
		fprintf(stdout,"\tnumber to arrive = %d\n\tr = %.6g\n\tB = %d\n\ttsfile = %s\n", data->packet_count, data->token_arrival, data->bucket_size,data->filename);
	else
		fprintf(stdout,"\tnumber to arrive = %d\n\tlambda = %.6g\n\tmu = %.6g\n\tr = %.6g\n\tB = %d\n\tP = %d\n", data->packet_count, data->packet_arrival, data->serving_time, data->token_arrival, data->bucket_size, data->token_required);
}

void VerifyFile(Parameters *data)
{
	char line[1024];
	struct stat s;
	if((access(data->filename,F_OK) ==-1))
	{
		fprintf(stderr,"File doesn't exist\n");
		exit(1);
	}
	if((access(data->filename,R_OK) ==-1))
	{
		fprintf(stderr,"File no Read Permission.\n");
		exit(1);
	}
	if(!stat(data->filename,&s))
	{
		if(s.st_mode & S_IFDIR)
		{
			fprintf(stderr,"It's a Directory\n");
			exit(1);
		}
	}
	fp=fopen(data->filename,"r");
	if(fp==NULL)
	{
		fprintf(stderr,"File pointer is NULL. Exitting!!\n");
		exit(1);
	}

	if(fgets(line, sizeof(line), fp) != NULL)
		data->packet_count=atoi(line);

	if(data->packet_count==0)
	{
		fprintf(stderr,"Packet cannot be zero , exitting.\n");
		exit(1);
	}
}
void get_Parameters(int argc, char**argv,Parameters *data)
{
	int parameter = 0;
	int index=0;
	int counterindex=1;
	memset(data,'\0',sizeof(Parameters));
	parameter = getopt_long_only(argc, argv, ":",long_Options, &index);

	while (parameter != -1)
	{
		switch(parameter)
		{
			case 'l': data->packet_arrival = atof(optarg);
				if(data->packet_arrival ==0)
				{
					fprintf(stderr,"Malformed Command Line .Please provide appropriate value for Packet arrival time\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 'm':data->serving_time = atof(optarg);
				if(data->serving_time ==0)
				{
					fprintf(stderr,"Malformed Command Line. Please provide appropriate value for packet Serving time\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 'r':data->token_arrival = atof(optarg);
				if(data->token_arrival==0)
				{
					fprintf(stderr,"Malformed Command Line, Please provide appropriate value for Token Arrival time.\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 'B':data->bucket_size = atoi(optarg);
				if(data->bucket_size<=0 || data->bucket_size>2147483647)
				{
					fprintf(stderr," Malformed Command Line Please provide appropriate value for Bucket Size\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 'P':data->token_required = atoi(optarg);
				if(data->token_required > 2147483647)
				{
					fprintf(stderr,"Invalid value of tokens, Token Required cannot be more than 2147483647\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				if(data->token_required < 1)
				{
					fprintf(stderr,"Invalid value of token, Token Required not appropriate\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 'n': data->packet_count = atoi(optarg);
				if(data->packet_count>2147483647)
				{
					fprintf(stderr,"Invalid value of packets, Packet count cannot be more than 2147483647\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				if(data->packet_count<1)
				{
					fprintf(stderr,"Invalid value for packets, Packet count must be positive value\n");
					fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(0);
				}
				break;
			case 't': data->filename = optarg;
				fileflag=1;
				break;
			case ':':
			case '?':
			default :
				fprintf(stderr, "\nIncomplete or Invalid command line arguments passed\n");
				fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(0);
				break;
		}
		counterindex+=2;
		parameter = getopt_long_only(argc, argv, ":", long_Options, &index);
	}

	if(counterindex < argc)
	{
		fprintf(stderr, "\n Malformed Command Line Invalid command line arguments passed\n");
		fprintf(stderr,"Correct Format: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
		exit(0);
	}
	if(fileflag)
		VerifyFile(data);
	setdefaults(data);
	PrintFirstLine(data);
}

void ProgramStatistics(Parameters* argobj)
{
	printf("\nStatistics:\n");
	struct timeval program_exit;
	gettimeofday(&program_exit,0);
	double average_time_spent_in_system=0;
	int packet_count=argobj->packet_count;
	double packet_service_time=sum_S1_service_time+sum_S2_service_time;
	double emulation_time=time_difference_in_double(program_exit)-time_difference_in_double(program_start_time);

	double average_time_spent_in_system_square=(sum_system_service_time_square)/packet_served;
	average_time_spent_in_system=(sum_system_service_time/packet_served);
	if(packet_count!=0)
	{
		double average_packet_inter_arrival=(sum_interval_arrival/packet_count);
		printf("average packet Inter Arrival Time = %.6g sec\n",average_packet_inter_arrival/1000);
	}
	else
	{
		printf("average packet Inter Arrival Time = 0.000000 Zero packets.\n");

	}
	if(packet_served!=0)
	{

		printf("average Packet Service time = %.6g sec\n",(packet_service_time/packet_served)/1000);
	}
	else
	{
		printf("average packet service time =0.000000 Zero packets Served\n");

	}

	if(emulation_time!=0)
	{
		printf("\n");
		double average_packet_time_spent_in_Q1=(sum_q1_service_time/emulation_time);
		printf("average number of packets in Q1 = %.6g packets\n",average_packet_time_spent_in_Q1);
		double average_packet_time_spent_in_Q2=(sum_q2_service_time/emulation_time);
		printf("average number of packets in Q2 = %.6g packets\n",average_packet_time_spent_in_Q2);
		double average_packet_time_spent_in_S1=(sum_S1_service_time/emulation_time);
		printf("average number of packets in S1 = %.6g packets\n",average_packet_time_spent_in_S1);
		double average_packet_time_spent_in_S2=(sum_S2_service_time/emulation_time);
		printf("average number of packets in S2 = %0.6g packets\n",average_packet_time_spent_in_S2);
	}
	else
	{
		printf("average number of packets in Q1: NA\n");
		printf("average packet of packets in Q2: NA\n");
		printf("average Packet of packets in S1= NA\n");
		printf("average Packet of packets in S2= NA\n");
	}
	if(packet_served!=0)
	{
		printf("\n");
		double average_packet_time_spent_in_system=(sum_system_service_time/packet_served)/1000;
		double standard_dev = sqrt(average_time_spent_in_system_square - (average_time_spent_in_system * average_time_spent_in_system));
		printf("average time Packet spent in system = %.6g sec\n",average_packet_time_spent_in_system);
		printf("standard Deviation of time spent in system = %.6g sec\n",(standard_dev/1000));
	}
	if(total_token_count!=0)
	{
		printf("\n");
		double token_dropped_probability=(double)dropped_token_count/total_token_count;
		printf("token Drop Probability = %.6g\n",token_dropped_probability);
	}
	else
	{
		printf("token Drop Probability = NA");
	}
	if(packet_count!=0)
	{
		double average_dropped_packet=(double)dropped_packet_count/packet_count;
		printf("packet Drop Probability = %.6g\n",average_dropped_packet);
	}
	else
	{
		printf("token Drop Probability = NA");
	}


}
void CleanQueue(My402List *Queue,int queue_name)
{
	My402ListElem* Q1_Element_clean = NULL;
	Packet* packet_clean = NULL;

	while ( ! My402ListEmpty(Queue) )
	{
		Q1_Element_clean = My402ListFirst(Queue);
		packet_clean = (Packet *) Q1_Element_clean->obj;
		fprintf(stdout, "\np%d is removed from Q%d.", packet_clean->packet_id,queue_name);
		My402ListUnlink(Queue, Q1_Element_clean);
	}
}
void handleUnfinishedBusiness()
{
        sigwait(&set);
        fprintf(stdout, " Interuppted Ctrl + C \n");
        pthread_mutex_lock(&token_mutex);
        pthread_cancel(packet_thread);
        pthread_cancel(bucket_thread);
        CleanQueue(&Q1_list,1);
        CleanQueue(&Q2_list,2);
        stop_bucket_thread=1;
        stop_packet_thread=1;
        pthread_cond_broadcast(&server_wait);
        pthread_mutex_unlock(&token_mutex);
}

void initialize(Parameters * argobj)
{
	My402ListInit(&Q1_list);
	My402ListInit(&Q2_list);
	My402ListInit(&token_list);
	argobj->bucket_size=0;
	argobj->packet_arrival=0;
	argobj->packet_count=0;
	argobj->serving_time=0;
	argobj->token_arrival=0;
	argobj->token_required=0;
}
int all_threads_success()
{
	if(stop_packet_thread==1 && stop_bucket_thread==1 && stopserver1==1 && stopserver2==1)
		return 1;
	else
		return 0;
}
void createThreads(Parameters *commandLineParameters)
{
	if(pthread_create(&packet_thread,0,(void*)packet_thread_function,commandLineParameters))
	{
		fprintf(stderr,"Error creating Packet thread.\n");
		exit(0);
	}
	if(pthread_create(&bucket_thread,0,(void*)bucket_thread_function,commandLineParameters))
	{
		fprintf(stderr,"Error creating Bucket thread.\n");
		exit(0);
	}
	if(pthread_create(&server1_thread,0,(void*)server1_thread_function,commandLineParameters))
	{
		fprintf(stderr,"Error creating Server1 thread.\n");
		exit(0);
	}
	if(pthread_create(&server2_thread,0,(void*)server2_thread_function,commandLineParameters))
	{
		fprintf(stderr,"Error creating Server2 thread.\n");
		exit(0);
	}
	if(pthread_create(&handle_thread,0,(void*)handleUnfinishedBusiness,NULL))
	{
		fprintf(stderr,"Error creating Handler thread\n");
		exit(0);
	}

}
int main(int argc,char *argv[])
{
	double initial_time=0;
	Parameters commandLineParameters;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	initialize(&commandLineParameters);
	get_Parameters(argc,argv,&commandLineParameters);
	gettimeofday(&program_start_time,0);
	program_start_time_in_double=time_difference_in_double(program_start_time);
	printf("%012.3fms Emulation Start\n",initial_time);
	createThreads(&commandLineParameters);
	pthread_join(packet_thread,0);
	pthread_join(bucket_thread,0);
	pthread_join(server2_thread,0);
	pthread_join(server1_thread,0);

	if(all_threads_success())
	{
		pthread_cancel(handle_thread);
	}
	pthread_join(handle_thread,0);
	gettimeofday(&program_exit_time,0);
	printf("%012.3fms Emulation Ends here\n",time_difference_in_double(program_exit_time));
	ProgramStatistics(&commandLineParameters);
	return 0;
}
