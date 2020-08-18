/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
  description:	��ǰһ���汾�Ļ�����������cd����

  date��		20050429
  author��		�ļ� Email��ganganwen@163.com
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

HANDLE mainhandle;		//���߳̾��
CONTEXT Context;		//���߳��л�������
BOOLEAN FlagEn = 1;		//����һ��ȫ�ֱ�������Ϊ�Ƿ�ʱ�ӵ��ȵı�־


/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void VCInit(void);						//��ʼ����ر���,һ����Ҫ

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

int main(int argc, char **argv)
{
	VCInit();	//��ʼ��һЩ����
	
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
	cp = GetCurrentProcess();	//�õ���ǰ���̾��
	ct = GetCurrentThread();	//�õ���ǰ�߳�α���
	DuplicateHandle(cp, ct, cp, &mainhandle, 0, TRUE, 2);	//α���ת��,�õ��߳�����
}

/**��������**/
void TaskStart(void * pParam) 
{	
	char err;
	char ch;
	char now[100];

	timeSetEvent(1000/OS_TICKS_PER_SEC, 0, OSTickISR, 0, TIME_PERIODIC);	//����һ����ʱ���߳�,�о�10 ticks/s�ȽϺ�
	
	//�����������񣬲���ȡ����������ź���
	OSTaskCreate(Task1,(void *)&ch,&TaskStk[2][TASK_STK_SIZE-1], Task1_Prio);
	semtask1=OSSemCreate(0);
	OSTaskCreate(Editor,0,&TaskStk[3][TASK_STK_SIZE-1], Editor_Prio);
	semeditor=OSSemCreate(0);

	//��ȡ��������ź���
	semstarter = OSSemCreate(0);
	while(1)
	{		
		OSSemPend(semstarter, 0, &err);   //�����ź������ȴ���ʹ֮����Shell

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