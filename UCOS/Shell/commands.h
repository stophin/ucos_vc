/*	 
 *	commands.h
 *	the genie shell commands part for ucosII
 *	under skyeye
 *
 *	Bugs report:	 Yang Ye  ( yangye@163.net )
 *	Last modified:	 2003-05-1 
 *  changed by:		wenjia
 *	bugs report:	ganganwen@163.com
 *	changed by:		stophin 2014/06/01
 */

#define MAX_FILE_BLOCK	1024
#define MAX_CMD_NAME	20

//#define  CREATE_MODE 0
#define  EN_DEBUG    0

#ifndef COMMANDDEF

#define MAX_COMMAND_NUM    17

typedef struct{
	int num;
	char name[MAX_CMD_NAME];
	INT8U (*CommandFunc)(INT8U argc,char **argv);
}command;

#endif 

#define _warning(msg) do{  \
	OS_ENTER_CRITICAL();	\
	printf("%s",msg);		\
	OS_EXIT_CRITICAL();	\
}while(0)

#define _error(msg) do{  \
 OS_ENTER_CRITICAL();	\
 printf("%s",msg);		\
  OS_EXIT_CRITICAL();	\
}while(0)


#define _log(msg)	do{   \
 OS_ENTER_CRITICAL();	\
 printf("%s",msg);		\
  OS_EXIT_CRITICAL();	\
}while(0)

INT8U InitCommands(void);
INT8U lsFunc(INT8U argc,char **argv);
INT8U rmFunc(INT8U argc,char **argv);
INT8U catFunc(INT8U argc,char **argv);
INT8U writeFunc(INT8U argc,char **argv);
INT8U formatFunc(INT8U argc,char **argv);
INT8U mkdirFunc(INT8U argc,char **argv);
INT8U rmdirFunc(INT8U argc,char **argv);
INT8U freeFunc(INT8U argc,char **argv);
INT8U helpFunc(INT8U argc,char **argv);

INT8U cdFunc(INT8U argc,char **argv);
INT8U pwdFunc(INT8U argc,char **argv);
INT8U ExitFunc(INT8U argc,char **argv);
INT8U ShowFunc(INT8U argc,char **argv);
INT8U clrscrFunc(INT8U argc,char **argv);
INT8U versionFunc(INT8U argc,char **argv);
INT8U shellFunc(INT8U argc,char **argv);
INT8U aliasFunc(INT8U argc,char **argv);

extern void ShowHelp(void);
extern void ShowVer(void);
extern void ShowPwd(void);
extern void ClearScr(void);
