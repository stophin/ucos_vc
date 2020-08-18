/*	 
 *	shelltask.h
 *	the genie shell task for ucosII
 *	under skyeye
 *
 *	Bugs report:	 Yang Ye  ( yangye@163.net )
 *	Last modified:	 2003-02-19 
 *	Last editor: Stophin
 *	2014/06/01
 */

#define ERRORCOMMAND    255

#define MaxLenComBuf	100

BOOLEAN inputlegal(const char ch);
void aliasget(char *name,char *alias,int *aliasnum,int *aliaspos);
INT8U aliasname(char *name,char *alias);
INT8U CommandAnalys(char *Buf);
void genieshell(char *CommandBuf);
void shelltask(void *pParam);

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 2048       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        4         /* Number of identical tasks                          */

//Tasks' priority
#define TaskStart_Prio	1
#define Shell_Prio		2
#define Task1_Prio		3
#define Editor_Prio		4

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

/* Function prototypes of tasks                  */
void TaskStart(void * pParam) ;
void Task1(void * pParam) ;
void Editor(void * pParam);

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

/* Task stacks                                  */
OS_STK  TaskStk[N_TASKS][TASK_STK_SIZE];    

/* Semaphores                                  */
OS_EVENT *semshell,*semstarter,*semtask1,*semeditor;

/* Mailbox                                     */
OS_EVENT *EditorMbox;

/* Others                                      */
char bkparam;