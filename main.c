/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
  description:	在前一个版本的基础上增加了cd命令

  date：		20050429
  author：		文佳 Email：ganganwen@163.com
  changed:		stophin
  change log:	2014/06/01 Using Semaphore to switch among tasks.
*********************************************************************************************************
*/

#include "includes.h"
#include "fs_api.h"


/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

HANDLE mainhandle;		//主线程句柄
CONTEXT Context;		//主线程切换上下文
BOOLEAN FlagEn = 1;		//增加一个全局变量，做为是否时钟调度的标志


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void VCInit(void);						//初始化相关变量,一定需要

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(int argc, char **argv)
{
	VCInit();	//初始化一些变量
	
	OSInit();
	FS_Init();			/* Init the file system */

	OSTaskCreate(TaskStart, (void *)&bkparam, &TaskStk[0][TASK_STK_SIZE-1], TaskStart_Prio);
	OSTaskCreate(shelltask, 0               , &TaskStk[1][TASK_STK_SIZE-1], Shell_Prio);
	OSStart();			//start never return
	FS_Exit();			/* End using the file system */
	return 0;
}

void VCInit(void)
{
	HANDLE cp,ct;
	Context.ContextFlags = CONTEXT_CONTROL;
	cp = GetCurrentProcess();	//得到当前进程句柄
	ct = GetCurrentThread();	//得到当前线程伪句柄
	DuplicateHandle(cp, ct, cp, &mainhandle, 0, TRUE, 2);	//伪句柄转换,得到线程真句柄
}

/**主任务函数**/
void TaskStart(void * pParam) 
{	
	char err;
	char ch;
	char now[100];

	timeSetEvent(1000/OS_TICKS_PER_SEC, 0, OSTickISR, 0, TIME_PERIODIC);	//开启一个定时器线程,感觉10 ticks/s比较好
	
	//新增两个任务，并获取各个任务的信号量
	OSTaskCreate(Task1,(void *)&ch,&TaskStk[2][TASK_STK_SIZE-1], Task1_Prio);
	semtask1=OSSemCreate(0);
	OSTaskCreate(Editor,0,&TaskStk[3][TASK_STK_SIZE-1], Editor_Prio);
	semeditor=OSSemCreate(0);

	//获取主任务的信号量
	semstarter = OSSemCreate(0);
	while(1)
	{		
		OSSemPend(semstarter, 0, &err);   //阻塞信号量，等待，使之运行Shell

		if (*(char *)pParam=='\0')
		{
			printf("\nTaskStart is waiting for your choice:");
			PC_GetKeyWait((INT16S *)&err);
			printf("%c\n",err);
		}
		else
		{
			err=*(char *)pParam;
		}
		switch(err)
		{
		case '1':
			ch='\0';
			OSSemPost(semtask1);
			while (ch=='\0')
			{
				PC_GetDateTime(now);
				if (OS_NO_ERR!=OSMboxPost(EditorMbox,(void *)now))
				{
					ch='1';
					break;
				}
				//	OSTimeDly(100);
			}
			break;
		case '2':
			OSSemPost(semeditor);
			break;
		default:
			OSSemPost(semshell);
			break;
		}
		if (*(char *)pParam)
		{
			OSSemPend(semstarter,0,&err);
			OSSemPost(semshell);
			*(char *)pParam='\0';
		}
	}
}

void Task1(void * pParam)
{
	char err;
	char *now=NULL;
	while (1)
	{
		OSSemPend(semtask1, 0, &err);

		if (EditorMbox==NULL)
			EditorMbox=OSMboxCreate((void *)0);
		if (EditorMbox==NULL)
			printf("\nWarning: mailbox not created!\n");

		printf("Hello This is Task1!\n");
		
		while(*((char *)pParam)=='\0')
		{
			printf(" Press anykey to terminate this task!\n");
			if (PC_GetKey((INT16S *)&err))
				break;
			now=OSMboxPend(EditorMbox,0,&err);
			if (now==(void *)0||err!=OS_NO_ERR)
				break;
			printf("\rtime%s",now);
		}
		printf("\nTask1 has exited!\n");
		*((char *)pParam)='1';

		OSSemPost(semstarter);
		
	}
}

void Editor(void * pParam)
{
	char err;
	char ch;
	int id;
	char tx[MAX_FILE_BLOCK];

	while (1)
	{
		OSSemPend(semeditor, 0, &err);

		printf("\nHello, this is Editor.\n");
		id=0;
		tx[id]='\0';
		do
		{
			PC_GetKeyWait((INT16S *)&ch);
			switch(ch)
			{
			case 13:
				printf("\n");
				tx[id++]='\n';
				break;
			case 27:
				break;
			case '\b':
				if (id)
					id--;
				printf("\b \b");
				break;
			default:
				printf("%c",ch);
				tx[id++]=ch;
			}
			if (id>=MAX_FILE_BLOCK)
			{
				break;
			}	
		}while(ch!=27);
		tx[id]='\0';
		
		printf("\n\nPosting data to Mailbox: %p\n",EditorMbox);

		OSMboxPost(EditorMbox,tx);

		OSSemPost(semstarter);
		
	}
}