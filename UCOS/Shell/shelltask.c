/*	 
*	shelltask.c
*	the genie shell task for ucosII
*	under skyeye
*
*	Bugs report:	 Yang Ye  ( yangye@163.net )
*	Last modified:	 2005-05-6 
*   changed by: wenjia
*	changed by: stophin 2014/06/01
*/

#include   "includes.h"

extern command ShellComms[MAX_COMMAND_NUM];
extern void CommRxIntEn(INT8U ch);
extern char CurDir[64];
char *argv[10];
INT8U argc;

extern void InitRamDisk();

//excute command 
void genieshell(char *CommandBuf)
{
	INT8U (*Func)(INT8U argc,char **argv);
	INT8U j=0,offset,num,flag;	/*offset is the offset of the commandbuf, num is the return value of command analyss function*/
	INT8U combine=0;		/*combine multicommand by using '|',this var act as a counter*/

	//replace '|' with '\0',and calcuate the counter
	//double '|' means it's a part of command, do not divide
	//if '|' is in '\'' or '\"' then ignore it
	combine=0;
	offset=0;
	flag=1;
	for (j=0;CommandBuf[j]!='\0';j++)
	{
		if (CommandBuf[j]=='|')
		{
			if (offset==0)	//if '|' is not in '\'' or '\"'
			{
				if (CommandBuf[j+1]!='|')
				{
					combine++;
					CommandBuf[j]='\0';
				}
				else
				{
					//combine double '|' into one
					for (num=j;CommandBuf[num]!='\0';num++)
						CommandBuf[num]=CommandBuf[num+1];
				}
			}
			else
				flag=0;
		}
		else if (CommandBuf[j]=='\''||CommandBuf[j]=='\"')
		{
			if (flag)
				offset++;
			else
				offset--;
			if (offset==0)
				flag=1;
		}
		else
		{
			if (offset)
				flag=0;
		}
	}		/*j records the total length of commandbuf*/	
	combine+=1;	//conuter added,so that when no '|', perform once	
	offset=0;
	while(combine--)	//perform multicommand
	{
		num = CommandAnalys(CommandBuf+offset);	//analys the argv in the commandbuf
		if(num!=ERRORCOMMAND)
		{             	//error or none exist command
			Func = ShellComms[num].CommandFunc;	//call corresponding CommandFunc
			Func(argc,argv);
		}
		else
		{
			_log("error command:");
			_log(CommandBuf+offset);
			_log("\n\n");
		}
		if (combine)	//if command has '|'
			for (;CommandBuf[offset++]!='\0';);	//prepare next command
	}
	//take '|' back
	combine=j;
	for (j=0;j<combine;j++)
	{
		if (CommandBuf[j]=='\0')
		{
			CommandBuf[j]='|';
		}
	}
}

//get alias number of a command name
INT8U aliasname(char *name,char *alias)
{
	int aliasnum=0;
	aliasget(name,alias,&aliasnum,NULL);
	return aliasnum;
}

//get alias number and position of a command name
void aliasget(char *name,char *alias,int *aliasnum,int *aliaspos)
{
	INT8U combine,j,offset;
	char cmdname[MAX_CMD_NAME];
	BOOLEAN ret;
	//replace '|' with '\0',and calcuate the counter
	combine=0;
	for (j=0;alias[j]!='\0';j++)
	{
		if (alias[j]=='|')
		{
			combine++;
			cmdname[j]='\0';
		}
		else
			cmdname[j]=alias[j];
	}
	cmdname[j]='\0';
	j=combine+1;	//conuter added,so that when no '|', perform once
	offset=0;
	while(j--)	//perform multicommand
	{
		ret=strcmp(name,cmdname+offset);
		if (!ret)
			break;
		if (j)	//if command has '|'
			for (;cmdname[offset++]!='\0';);	//prepare next command
	}
	if (!ret)
	{
		if (aliaspos)
			*aliaspos=combine+1-j;	//this shows the position of alias name
		if (aliasnum)
			*aliasnum=combine+1;	//this shows the number of alias names for this command
	}
	else
	{
		if (aliaspos)	//if command not found then return 0
			*aliaspos=0;
		if (aliasnum)
			*aliasnum=0;
	}
}

//
BOOLEAN inputlegal(const char ch)
{
	/*you can add any char you want*/
	if (ch=='|')
		return (TRUE);
	if (ch==27)
		return (TRUE);
	if (ch=='\t')
		return (TRUE);
	if (ch=='=')
		return (TRUE);
	if (ch=='?')
		return (TRUE);
	if (ch=='\'')
		return (TRUE);
	if (ch=='\"')
		return (TRUE);
	/*basic input*/
	if ((ch>='0'&&ch<='9')||(ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch=='.')||(ch==' ')||(ch==':')||(ch=='\\')||(ch=='-')||(ch=='/')||(ch==0xd)||(ch=='\b')||(ch==','))
		return (TRUE);
	return (FALSE);
}

void shelltask(void *pParam)
{
	INT8U i=0;		/*i is the pointer of commandbuf */
	char ch;
	char CommandBuf[MaxLenComBuf+1];	/*store '\0'*/
	InitCommands();
	CommandBuf[0] = '\0';
	
	//to do: add some lib functions for ucosII ,like clear screen .
	//clrscr();
	ShowVer();
	/*To be done: Login & Password*/
	InitRamDisk();
    ShowHelp();
	ShowPwd();

	semshell=OSSemCreate(0);
	for(;;){
		do
		{							//only accept a-z,0-9,A-Z,.,space,/,-
			PC_GetKeyWait((INT16S *)&ch);
		}while(!inputlegal(ch));
		switch(ch)
		{
		case '\t':
			bkparam='2';
			OSSemPost(semstarter);
			OSSemPend(semshell,0,&ch);
			ShowPwd();
			i=0;
			CommandBuf[i]='\0';
			break;
		case 27:
			bkparam='\0';
			OSSemPost(semstarter);
			OSSemPend(semshell,0,&ch);
			ShowPwd();
			i=0;
			CommandBuf[i]='\0';
			break;
		case 13:				//enter
			if (i==0)
			{      						//commandbuf is null,begin a new line
				ShowPwd();
			}
			else{
				if(CommandBuf[i-1]==' ') 
					i--;			//get rid of the end space
				CommandBuf[i] = '\0';
				genieshell(CommandBuf);
				i = 0;
				CommandBuf[i] = '\0';
				ShowPwd();
			}
			break;
			
		case '\b':				//backspace
			if ( i==0 ){		//has backed to first one
				//do nothing
			}
			else{
				i--;			//pointer back once
				OS_ENTER_CRITICAL();
				putchar('\b');		//cursor back once
				putchar(' ');	      //earse last char in screen
				putchar('\b');		//cursor back again
				OS_ENTER_CRITICAL();
			}
			break;
			
		case ' ':               //don't allow continuous or begin space(' ')
			if((CommandBuf[i-1] == ' ')||(i==0)||(i>MaxLenComBuf)){
				//do nothing
			}
			else
			{
				CommandBuf[i] = ch;
				i++;
				OS_ENTER_CRITICAL();
				putchar(ch);  //display and store ch
				OS_ENTER_CRITICAL();
			}
			break;
			
		default:				//normal key
			if (i>MaxLenComBuf){	//the buf reached MAX 
				//do nothing
			}			
			else{
				CommandBuf[i] = ch;
				i++;
				OS_ENTER_CRITICAL();
				putchar(ch);  //display and store ch
				OS_ENTER_CRITICAL();
			}
			break;
		}  //switch
	}//for(;;)
}

INT8U CommandAnalys(char *Buf)
{
	INT8U i,j;
	INT8U pointer;
	INT8U num;
	char name[MAX_CMD_NAME];		//command name length
	
	argc = 0;              //argc is global
	pointer = 0;
	num = 0;
	_log("\n\r");
	
	//'eat' white spaces at the front
	if (Buf[0]==' ')
		pointer++;
	
	while((Buf[pointer]!=' ') && (Buf[pointer]!='\0') && pointer<MAX_CMD_NAME ){
		name[pointer]=Buf[pointer];
		pointer++;
	}
	name[pointer] = '\0';	//now got the command name, and pointer is to the first space in the Buf
	
	for(i=0;i<MAX_COMMAND_NUM;i++){
		if(aliasname(name,ShellComms[i].name)){
			num = i;
			break;
		}				//find the command number
	}					
	//not find it
	if (i==MAX_COMMAND_NUM) return ERRORCOMMAND;
	
	j=0;
	i=1;
	while(Buf[pointer]!='\0')
	{
		if(Buf[pointer]==' ')
		{
			if (j==0)	//if space is not in '\'' or '\"'
			{
				if(argc>0)
				{
					Buf[pointer] = '\0';			//end of last argv
				}
				pointer++;
				argv[argc] = &Buf[pointer];			//add a parameter for every space
				argc++;
			}
			else
			{
				i=0;
				pointer++;
			}
		}
		else
		{
			if (Buf[pointer]=='\''||Buf[pointer]=='\"')
			{
				if (i)
					j++;
				else
					j--;
				if (j==0)
					i=1;
			}
			else
			{
				if (j)
					i=0;
			}
			pointer++;
		}
	}//while

	//eat '\'' or '\"' at the two ends of each argv
	for (i=0;i<argc;i++)
	{
		for (pointer=0;argv[i][pointer]!='\0';pointer++);
		if ((argv[i][0]=='\''||argv[i][0]=='\"')&&(argv[i][pointer-1]=='\''||argv[i][pointer-1]=='\"'))
		{
			argv[i][pointer-1]='\0';
			argv[i]++;
		}
	}
	return num;
}
