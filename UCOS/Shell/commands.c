/*	 
 *	commands.c
 *	the genie shell command part for ucosII
 *	under skyeye
 *
 *	Bugs report:	 ganganwen@163.com
 *	Last modified:	 2005-04-25 
 *  changed by: wenjia
 *	changed by: stophin 2014/06/01
 */


#include	"includes.h"



command ShellComms[MAX_COMMAND_NUM];

FS_FILE *myfile;
char mybuffer[0x100];

char CurDir[64]="\\";		//show current dir
char TargetVal[64] = "\\";  
//TargetVal数组第一功能功能：用于组合当前目录与输入数据如 
//CurDir = "\mydir"   cat 1.txt 时组合成cat \mydir\1.txt
//第二功能：保留CurDir目录，因为如果cd参数第一个字符为\时就从&CurDir[0] 
//开始复制，但若输入目录出错则将原来的CurDir目录还原。

INT8U DirExist(const char *DirName);
void RewindRoot(void);


void CreateTargetVal(const char *arg);

//判断目录名是否存在
//1:目录存在  0:目录不存在
INT8U DirExist(const char *DirName)
{
    FS_DIR *dirp;
    dirp = FS_OpenDir(DirName);
    if (dirp)
    {
        FS_CloseDir(dirp);
       // _log("Open %s Exist!\n", DirName);
        return 1;
    } else {
       // _log("The %s Dir is not exist!\n", DirName);
        return 0;
    }
} 
//--------------------------------------------
//#define CUR_DRI_MAX_LEN 32
//FS_DIR CurDir[CUR_DRI_MAX_LEN 32];
 // struct FS_DIRENT *direntp;
/*********************************************************************
*
*             _write_file
*
  This routine demonstrates, how to create and write to a file
  using the file system.
*/

//
static void _write_to_file(const char *name, const char *txt)
{
	int x;

	if (myfile) {
    /* write to file */
    x = FS_FWrite(txt,1,strlen(txt),myfile);
    /* all data written ? */
    if (x!=(int)strlen(txt)) {
      /* check, why not all data was written */
      x = FS_FError(myfile);
      sprintf(mybuffer,"Not all bytes written because of error %d.\n\n",x);
      _error(mybuffer);
    }
    /* close file */
    FS_FClose(myfile);
//	_log("\n\n");
  }
  else {
    sprintf(mybuffer,"Unable to create file %s\n\n",name);
    _error(mybuffer);
  }
}

static void _write_file(const char *name, const char *txt) {
  /* create file */
  myfile = FS_FOpen(name,"w");
   /* really write to file*/
  _write_to_file(name,txt);
}

//
static void _write_file_append(const char *name, const char *txt) {
  /* create file */
  myfile = FS_FOpen(name,"a");
  /* really write to file*/
  _write_to_file(name,txt);
}


/*********************************************************************
*
*             _dump_file
*
  This routine demonstrates, how to open and read from a file using 
  the file system.
*/

static void _dump_file(const char *name) {
  int x;

  /* open file */
  myfile = FS_FOpen(name,"r");
  if (myfile) {
    /* read until EOF has been reached */
    do {
      x = FS_FRead(mybuffer,1,sizeof(mybuffer)-1,myfile);
      mybuffer[x]=0;
      if (x) {
        _log(mybuffer);
      }
    } while (x);
    /* check, if there is no more data, because of EOF */
    x = FS_FError(myfile);
    if (x!=FS_ERR_EOF) {
      /* there was a problem during read operation */
		sprintf(mybuffer,"Error %d during read operation.\n",x);
		_error(mybuffer);
    }
    /* close file */
    FS_FClose(myfile);
//	_log("\n\n");
  }
  else {
    sprintf(mybuffer,"Unable to open file %s.\n",name);
    _error(mybuffer);
  }
}

static void _dump_file_to_buffer(const char *name,char *buffer)
{
	int x,i,l;
	if (buffer==NULL)
		return;
	
  /* open file */
  myfile = FS_FOpen(name,"r");
  if (myfile) {
    /* read until EOF has been reached */
	  l=0;
    do {
      x = FS_FRead(mybuffer,1,sizeof(mybuffer)-1,myfile);
      mybuffer[x]=0;
      if (x) {
      //  _log(mybuffer);
		for (i=0;i<x;i++)
		{
			buffer[l++]=mybuffer[i];
			if (l>=MAX_FILE_BLOCK)
				break;
		}
		if (l>=MAX_FILE_BLOCK)
				break;
		buffer[l++]='\n';
      }
    } while (x);
	buffer[l]='\0';
    /* check, if there is no more data, because of EOF */
    x = FS_FError(myfile);
    if (x!=FS_ERR_EOF) {
      /* there was a problem during read operation */
		sprintf(mybuffer,"Error %d during read operation.\n",x);
		_error(mybuffer);
		buffer[0]='\0';
    }
    /* close file */
    FS_FClose(myfile);
//	_log("\n\n");
  }
  else
  {
	  sprintf(mybuffer,"Unable to open file %s.\n",name);
	  _error(mybuffer); 
	  buffer[0]='\0';
  }
}


/*********************************************************************
*
*             _show_directory
*
  This routine demonstrates, how to read a directory.
*/

#if FS_POSIX_DIR_SUPPORT

static void _show_directory(const char *name) {
  FS_DIR *dirp;
  struct FS_DIRENT *direntp;

  dirp = FS_OpenDir(name);
  if (dirp) {
    do {
      direntp = FS_ReadDir(dirp);
      if (direntp) {
        sprintf(mybuffer,"%s\n",direntp->d_name);
        _log(mybuffer);
      }
    } while (direntp);
    FS_CloseDir(dirp);
	_log("\n");
  }
  else {
    _error("Unable to open directory\n");
  }
}

/*static INT8U _cd_directory(const char *name)
{
    FS_DIR *dirp;
    struct FS_DIRENT *direntp;   
	
}*/
#endif /* FS_POSIX_DIR_SUPPORT */


/*********************************************************************
*
*             _show_free
*
  This routine demonstrates, how to read disk space information.
*/

static void _show_free(const char *device) {
  FS_DISKFREE_T disk_data;
  int x;

  _log("Disk information of ");
  _log(device);
  _log("\n");
  x = FS_IoCtl(device,FS_CMD_GET_DISKFREE,0,(void*) &disk_data);
  if (x==0) {
    sprintf(mybuffer,"total clusters     : %lu\navailable clusters : %lu\nsectors/cluster    : %u\nbytes per sector   : %u\n",
          disk_data.total_clusters, disk_data.avail_clusters, disk_data.sectors_per_cluster, disk_data.bytes_per_sector);
    _log(mybuffer);
  } 
  else {
    _error("Invalid drive specified\n\n");
  }
}

/*********************************************************************
*
*             InitCommands
*
  This routine init shell
*/
void InitCmd(void)
{
    INT8U TmpCnt = MAX_COMMAND_NUM;
    while (TmpCnt--)
    {
        ShellComms[TmpCnt].CommandFunc = NULL;
        ShellComms[TmpCnt].name[0]='\0';
        ShellComms[TmpCnt].num  = 0;
    }
}

INT8U InitCommands()
{
    InitCmd();
	ShellComms[0].num = 0;
	strcpy(ShellComms[0].name,"ls");
	ShellComms[0].CommandFunc = lsFunc;

	ShellComms[1].num = 1;
	strcpy(ShellComms[1].name,"rm");
	ShellComms[1].CommandFunc = rmFunc;

	ShellComms[2].num = 2;
	strcpy(ShellComms[2].name,"write|nano");
	ShellComms[2].CommandFunc = writeFunc;

	ShellComms[3].num = 3;
	strcpy(ShellComms[3].name,"cat");
	ShellComms[3].CommandFunc = catFunc;

	ShellComms[4].num = 4;
	strcpy(ShellComms[4].name,"format");
	ShellComms[4].CommandFunc = formatFunc;

	ShellComms[5].num = 5;
	strcpy(ShellComms[5].name,"mkdir");
	ShellComms[5].CommandFunc = mkdirFunc;

	ShellComms[6].num = 6;
	strcpy(ShellComms[6].name,"rmdir");
	ShellComms[6].CommandFunc = rmdirFunc;

	ShellComms[7].num = 7;
	strcpy(ShellComms[7].name,"free");
	ShellComms[7].CommandFunc = freeFunc;

	ShellComms[8].num = 8;
	strcpy(ShellComms[8].name,"help");
	ShellComms[8].CommandFunc = helpFunc;

	ShellComms[9].num = 9;
	strcpy(ShellComms[9].name,"cd");
	ShellComms[9].CommandFunc = cdFunc;

	ShellComms[10].num = 10;
	strcpy(ShellComms[10].name,"pwd");
	ShellComms[10].CommandFunc = pwdFunc;

    ShellComms[11].num = 11;
	strcpy(ShellComms[11].name,"exit|:x|:X");
	ShellComms[11].CommandFunc = ExitFunc;
        
    ShellComms[12].num = 12;
	strcpy(ShellComms[12].name,"show");
	ShellComms[12].CommandFunc = ShowFunc;

	ShellComms[13].num = 13;
	strcpy(ShellComms[13].name,"cls|clear");
	ShellComms[13].CommandFunc = clrscrFunc;

	ShellComms[14].num = 14;
	strcpy(ShellComms[14].name,"ver");
	ShellComms[14].CommandFunc = versionFunc;

	ShellComms[15].num = 15;
	strcpy(ShellComms[15].name,"shell");
	ShellComms[15].CommandFunc = shellFunc;

	ShellComms[16].num = 16;
	strcpy(ShellComms[16].name,"alias");
	ShellComms[16].CommandFunc = aliasFunc;

	return 0;
}

//alias 
INT8U aliasFunc(INT8U argc,char **argv)
{
	char cmd[MAX_CMD_NAME];
	char alias[MAX_CMD_NAME];
	char *tmp=NULL;
	BOOLEAN unalias=FALSE;
	int i=0,k,j,f;
	if (argc!=1)
	{
		_error("Usage: alias command[=/-][aliascommand]\n");
		_error("option =    alias\n");
		_error("       -    unalias\n");
		return 1;
	}
	else
	{
		k=0;j=0;f=0;
		for (i=0;argv[0][i]!='\0';i++)
		{
			if (f==0&&argv[0][i]=='='||argv[0][i]=='-')
			{
				f=1;
				if (argv[0][i]=='-')
					unalias=TRUE;
				continue;
			}
			if (f)
				alias[j++]=argv[0][i];
			else
				cmd[k++]=argv[0][i];
			if (k>=MAX_CMD_NAME||j>=MAX_CMD_NAME)
				break;
			
		}
		alias[j]='\0';
		cmd[k]='\0';
		if (cmd[0]=='\0')
		{
			_error("command cannot be empty!\n");
			return 1;
		}
		if (alias[0]=='\0') //if aliascommand is null, then show the alias of command
		{
			for(i=0;i<MAX_COMMAND_NUM;i++)	//find the command number
			{
				if(aliasname(cmd,ShellComms[i].name))	//found command
				{
					_log(ShellComms[i].name);
					_log("\n");
					break;
				}				
			}
			if (i==MAX_COMMAND_NUM)
			{
				_error("command ");
				_error(cmd);
				_error(" does not exist!\n");
			}	
			return 1;
		}
	//	printf("\n_%s_%s_\n",cmd,alias);
		for(i=0;i<MAX_COMMAND_NUM;i++)	//check if alias is used by other command
		{
			aliasget(alias,ShellComms[i].name,&k,&j);
			if (k&&!aliasname(cmd,ShellComms[i].name))
			{
				_error(alias);
				_error(" has duplicated command alias:");
				_error(ShellComms[i].name);
				_error("\n\n");
				return 1;
			}
		}
		f=-1;
		for(i=0;i<MAX_COMMAND_NUM;i++)	//find the command number
		{
			if(aliasname(cmd,ShellComms[i].name))	//found command
			{
				aliasget(alias,ShellComms[i].name,&k,&j);	
				if (!k)	//check if clias already exists,if does not exist
				{
					f= i;	//record the command number and exit loop
					break;
				}
				else  //already exists
				{
					f=-2;	//flag and exit loop
					break;
				}
			}				
		}
		if (f==-1)
		{
			_error("command ");
			_error(cmd);
			_error(" does not exist!\n");
			return 1;
		}
		else if (f==-2)	//if command already exists and there're not one command,then unalias it
		{
			if (k>1)
			{
				if (unalias)
				{
					j--;k--;
					for (f=0;ShellComms[i].name[f]!='\0';f++)
					{
						if (ShellComms[i].name[f]=='|')
						{
							j--;
							k--;
						}
						if (j==0)
						{
							break;
						}
					}
					ShellComms[i].name[f]='\0';
					if (k)
					{
						for (f++;ShellComms[i].name[f]!='\0';f++)
						{
							if (ShellComms[i].name[f]=='|')
							{
								break;
							}
						}
						tmp=ShellComms[i].name+f;
						if (ShellComms[i].name[0]=='\0')
							tmp++;
						strcat(ShellComms[i].name,tmp);
					}
				//	printf("\n%s\n",ShellComms[i].name);
					_log("command ");
					_log(cmd);
					_log(" unaliasd: ");
					_log(ShellComms[i].name);
					_log("\n");
				}
				else
				{
					_error("command ");
					_error(cmd);
					_error(" cannot be aliased with ");
					_error(alias);
					_error(", because it is already existed.\n");
					return 1;
				}
			}
			else if (unalias)
			{
				_log("command ");
				_log(cmd);
				_log(" cannot be aliased, otherwise there's no command left.\n");
				return 1;
			}
			else
			{
				_error("command ");
				_error(cmd);
				_error(" cannot be aliased with");
				_error(alias);
				_error(", because it is already existed.\n");
				return 1;
			}
		}
		else 	//alias
		{
			if (unalias)
			{
				_log("command ");
				_log(cmd);
				_log(" not aliasd: ");
				_log(ShellComms[i].name);
				_log("\n");
			}
			else
			{
				strcat(ShellComms[i].name,"|");
				strcat(ShellComms[i].name,alias);
			//	printf("\n%s\n",ShellComms[i].name);
				_log("command ");
				_log(cmd);
				_log(" aliasd: ");
				_log(ShellComms[i].name);
				_log("\n");
			}
		}
	}
	return 0;
}

//run shell command
INT8U shellFunc(INT8U argc,char **argv)
{
	int i,j;
	char cmd[MAX_FILE_BLOCK];
	if (argc<1)
	{
		_error("Usage: shell [option] filename\n\n");
		return 1;
	}
	else if (argc<2)
	{
		if (argv[0][0]=='-')
		{
			if (argv[0][1]=='h')
			{
				_error("Usage: shell [option] filename\n");
				_error("option: -h      show this help\n\n");
				return 1;
			}
			else
			{
				_error("Wrong option, use -h to help yourself\n\n");
				return 1;
			}
		}
		CreateTargetVal(argv[0]);
		_dump_file_to_buffer(TargetVal,cmd);
		//cut every '\n' with '\0'
		j=0;
		for (i=0;cmd[i]!='\0';i++)
		{
			if (cmd[i]=='\n')
			{
				j++;
				cmd[i]='\0';
			}
		}
		i=0;
		while (j--)	//execute shell command with every '\0'
		{
			genieshell(cmd+i);
			if (j)
			{
				for (;cmd[i++]!='\0';);
			}
		}
	}
	else
	{
		_error("Too many parameters, use -h to help yourself\n\n");
		return 1;
	}
	return 0;
}

//
INT8U versionFunc(INT8U argc,char **argv)
{
	ShowVer();
	return 0;
}

//differs in different platform
INT8U clrscrFunc(INT8U argc,char **argv)
{
	system("cls");
	return 0;
}


INT8U ShowFunc(INT8U argc,char **argv)
{

    _log("\n=========  TargetVal =  ");
	_log(TargetVal);
	_log("========\n");
    return 0;
}

INT8U ExitFunc(INT8U argc,char **argv)
{
    exit(0);    
    return 0;
}


INT8U pwdFunc(INT8U argc,char **argv)
{
	if (argc != 0)
	{
		_error("PWD is single arg\n");
		return 1;
	}
	_log(CurDir);
	_log("\n");
	return 0;
}
//功能：设置成根目录
void RewindRoot(void)
{
    CurDir[0] = '\\';
    CurDir[1] = '\0';
}


//更改当前目录命令
INT8U cdFunc(INT8U argc,char **argv)
{
	INT8U DirCurPos = 0;
    INT8U CmdLen    = 0;
	INT8U TmpVal    = 0;
    INT8U IsRoot    = 0;
    INT8U IsBackup  = 0;
    INT8U BackupLen = 0;
	DirCurPos = strlen(CurDir);   //取得当前目录的字符长度
    if (argc != 1)
	{
		_error("please input command as:cd directoryname ");
        _error("\n cd \\ change to root\n cd .. \n cd \\MyDir\\Dir1\n");
		_error("\n\n");
		return 1;
	}
	if (strcmp(argv[0], "..") == 0)  //回到上一级
	{
		if (1 == DirCurPos)         //设定长度为1的为根目录
		{
#if EN_DEBUG == 1
			_error("This is root dir\n");
#endif
			return 1;
		} else {
			TmpVal = DirCurPos;   //取得当前目录的长度并从最后开始查找\
		   //从最后开始查找将该级目录清除 
           //例：\my\dir1  从1开始找到r最后找到\dir的\清除
            while (TmpVal--) 
		   {
			   if (CurDir[TmpVal] == '\\')   //是否找到上一级目录
			   {
				   if (TmpVal == 0)         //根目录
				   {
					   RewindRoot();
				   } else {
				      CurDir[TmpVal] = '\0';   //将最后的\换成结束符
				   }
#if EN_DEBUG  == 1
                   _log("\n CurDir Len = %d\n", strlen(CurDir));
#endif
				   break;    //退出循环
               } else {
                   CurDir[TmpVal] = '\0';  //清除字符
               }
		   }
		}
    } else	if (strcmp(argv[0], "\\") == 0)	{   //
		RewindRoot();   //回到根目录
	} else {
	    if (strcmp(CurDir, "\\") == 0)   //比较方法用于比较是否是当前目录
		{
            IsRoot = 1;
            if ('\\' == argv[0][0])   //如果当输入参数是带\开始的，如cd \dir\dir1
            {
                DirCurPos = 0;   
            }
#if EN_DEBUG == 1
            _log("\ncd start at root! DirCurPos = %d\n", DirCurPos);
#endif
		} else {
            if ('\\' == argv[0][0])   //如果当输入参数是带\开始的，如cd \dir\dir1
            {
                //因为当前目录不是根目录，所以为了防止出错保留原目录
                IsBackup  = 1;
                BackupLen = DirCurPos + 1;
                FS__CLIB_strncpy(&TargetVal[0], &CurDir[0], BackupLen);
                DirCurPos = 0;     //从头开始复制命令
            } else {
		        CurDir[DirCurPos] = '\\';
			    DirCurPos++;
            }
		}
        CmdLen    = strlen(argv[0]);
        FS__CLIB_strncpy(&CurDir[DirCurPos], argv[0], CmdLen + 1);  //确保字符是以'\0'结束请看FS__CLIB_strncpy源码
#if EN_DEBUG == 1
        _log("\nafter FS__CLIB_strncpy() CurDir= %s  DirCurPos = %d CmdLen = %d\n", CurDir, DirCurPos, CmdLen);
#endif
        if (DirExist(CurDir) == 0)  //目录不存在
        {
			_log(argv[0]);
            _log("  Dir is not exist!\n", );
            if (1 == IsRoot)   //是否是根目录
            {
                RewindRoot();               
            } else {
                if (1 == IsBackup)
                {
                    FS__CLIB_strncpy(&CurDir[0], &TargetVal[0], BackupLen);
                } else {
                    CurDir[DirCurPos - 1] = '\0';   //将加入的目录清除
                }
            }
        } else {
#if EN_DEBUG == 1
            _log("Open %s Exist!\n", CurDir);  //目录存在
#endif
        }
	}
#if EN_DEBUG == 1
    _log("\nCurDir = %s \n", CurDir);
#endif
	return 0;
}

INT8U lsFunc(INT8U argc,char **argv)
{			

	if(argc == 0)
    {
        _show_directory(CurDir);//_show_directory("");
    } else {
        _show_directory(argv[0]);
    }
	return 0;
}
		

INT8U rmFunc(INT8U argc,char **argv)
{

	if(argc != 1)
	{
		_error("please input command as:rm filename");
		_error("\n\n");
		return 1;
	}
    CreateTargetVal(argv[0]);
	//if(!FS_Remove(argv[0]))
    if(!FS_Remove(TargetVal))
	{
		_log("remove ");
		_log(argv[0]);
		_log(" successful\n\n");
	}
	else
	{
		_error("remove failure!\n\n");
	}
	return 0;
}

INT8U writeFunc(INT8U argc,char **argv)
{
	int i,j,k;	
	char *content=NULL;
	char tx[MAX_FILE_BLOCK];
	INT8U err;
	BOOLEAN append;

	if (EditorMbox==NULL)
		EditorMbox=OSMboxCreate((void *)0);
	if (EditorMbox==NULL)
		_warning("\nMailbox not created!\n");
	else
		printf("Mail box successfully created: %p\n",EditorMbox);

	if (argc<1)
	{
		_error("please input command as:write [option] filename [data]");
		_error("\n\n");
		return 1;
	}
	else if (argc<2)
	{
		//write with option only, will consider filename start with '-' later
		if (argv[0][0]=='-')
		{
			if (argv[0][1]=='h')
			{
				_error("Uasge: write [option] filename [data]\n");
				_error("option:	-a      append to file\n");
				_error("        -w      write and cover the file\n");
				_error("        -h      show this help\n");
				_error("\n\n");
				return 1;
			}
			else
			{
				_error("please input command as:write [option] filename [data]");
				_error("\n\n");
				return 1;
			}
		}
		CreateTargetVal(argv[0]);

		//call editor
		bkparam='2';	//set parameter for taskstart,so that taskstart can switch to editor
		OSSemPost(semstarter);
		OSSemPend(semshell,0,&err);
		content=OSMboxPend(EditorMbox,1,&err);	//get information from mailbox

		if (!(content))
		{
			_error("\ninput error!");
			_error("\n\n");
			return 1;
		}
	}
	else
	{
		if (argv[0][0]=='-')
		{
			if (argv[0][1]=='a')
				append=TRUE;
			else if (argv[0][1]=='w')
				append=FALSE;
			else
			{
				_error("option error, use -h to help yourself.");
				_error("\n\n");
				return 1;
			}
			CreateTargetVal(argv[1]);
		}
		else
		{
			CreateTargetVal(argv[0]);
		}
		if (argc==2)
		{
			if (argv[0][0]=='-')
			{
				//call editor
				bkparam='2';	//set parameter for taskstart,so that taskstart can switch to editor
				OSSemPost(semstarter);
				OSSemPend(semshell,0,&err);
				content=OSMboxPend(EditorMbox,0,&err);	//get information from mailbox

				if (!(content))
				{
					_error("\ninput error!");
					_error("\n\n");
					return 1;
				}
			}
			else
			{
				k=0;
				for (j=0;argv[1][j]!='\0';j++)
				{
					tx[k++]=argv[1][j];
					if (k>=MAX_FILE_BLOCK)
						break;
				}
				tx[k]='\0';
				content=tx;
			}
		}
		else
		{
			k=0;
			for (i=argv[0][0]=='-'?2:1;i<argc;i++)
			{
				for (j=0;argv[i][j]!='\0';j++)
				{
					tx[k++]=argv[i][j];
					if (k>=MAX_FILE_BLOCK)
						break;
				}
				if (k>=MAX_FILE_BLOCK)
					break;
			}
			tx[k]='\0';
			content=tx;
		}
	}

	EditorMbox=OSMboxDel(EditorMbox,OS_DEL_ALWAYS,&err);
	if ((OS_EVENT *)0!=EditorMbox)
	{
		_warning("\nMailbox not delected!");
	}

	if (append)
		_write_file_append(TargetVal,content);
	else
		_write_file(TargetVal,content);

	_log("\nwrite success!\n");

	PC_GetDateTime(content);
	printf("%s @%s",TargetVal,content);

	return 0;
}


//用于将arg参数构造成CurDir + arg = TargetParam
void CreateTargetVal(const char *arg)
{
    INT8U CurDirLen = 0;
    //INT8U ArgIsStartRoot = 0;

    CurDirLen = strlen(CurDir);
    if ('\\' == arg[0])
    {
     //   ArgIsStartRoot  = 1;
        FS__CLIB_strncpy(&TargetVal[0], arg, strlen(arg) + 1);
    } else {
        if (1 == CurDirLen)  //判断是否当前目录
        {
           //if (1 == ArgIsStartRoot)
        //{
        //    FS__CLIB_strncpy(&TargetVal[0], arg, strlen(arg) + 1);
        //} else{
            FS__CLIB_strncpy(&TargetVal[CurDirLen], arg, strlen(arg) + 1);
        //}
    //} else {
      //  if (1 == ArgIsStartRoot)
       // {
       //      FS__CLIB_strncpy(&TargetVal[0], arg, strlen(arg) + 1);
        } else {
            FS__CLIB_strncpy(TargetVal, CurDir, CurDirLen + 1);
            TargetVal[CurDirLen++] = '\\';
#if EN_DEBUG == 1
            _log("\n %s\n", TargetVal);
#endif
            FS__CLIB_strncpy(&TargetVal[CurDirLen], arg, strlen(arg) + 1);
        }
    }
#if EN_DEBUG == 1
    _log("\nTargetVal = %s\n", TargetVal);
#endif
}

INT8U catFunc(INT8U argc,char **argv)
{
    if(argc != 1)
	{
		_error("please input command as:cat filename");
		_error("\n\n");
		return 1;
	}
    CreateTargetVal(argv[0]);
    _dump_file(TargetVal);
	return 0;
}

INT8U formatFunc(INT8U argc,char **argv)
{
	int x;
	char ch='\0';
	_warning("DO YOU REALLY WANT TO FORMAT?\n");
	PC_GetKeyWait((INT16S *)&ch);
	if (ch!='y'&&ch!='Y')
	{
		return -1;
	}
	RewindRoot();

    /* format your RAM disk */
    x = FS_IoCtl("ram:0:",FS_CMD_FORMAT_MEDIA,FS_MEDIA_RAM_16KB,0);
    if (x!=0) {
      _error("Cannot format RAM disk.\n\n");
	  return 1;
	}
	else{
		_log("format successful\n\n");
		return 0;
	}
}

INT8U mkdirFunc(INT8U argc,char **argv)
{
	if(argc != 1)
	{
		_error("please input command as:mkdir filename");
		_error("\n\n");
		return 1;
	}
    CreateTargetVal(argv[0]);
	//if(!FS_MkDir(argv[0]))
    if(!FS_MkDir(TargetVal))
	{
		_log("make directory ");
		_log(argv[0]);
		_log(" successful\n\n");
	}
	else
	{
		_error("mkdir failure! please check disk full or unformated\n");
	}
	return 0;
}

INT8U rmdirFunc(INT8U argc,char **argv)
{
	if(argc != 1)
	{
		_error("please input command as:mkdir filename");
		_error("\n\n");
		return 1;
	}
    CreateTargetVal(argv[0]);
	//if(!FS_RmDir(argv[0]))
    if(!FS_RmDir(TargetVal))
	{
		_log("remove ");
		_log(argv[0]);
		_log(" successful\n\n");
	}
	else
	{
		_error("remove failure! please check the directory name\n");
	}
	return 0;
}


INT8U freeFunc(INT8U argc,char **argv)
{
	_show_free("");
	_log("\n");
	return 0;
}

INT8U helpFunc(INT8U argc,char **argv)
{
    ShowHelp();
	return 0;
}

void ShowHelp(void)
{
	_log("\nUsage:\n");
	_log("format\t\t- 格式化\n");
	_log("mkdir\t\t- 建立目录\n");
	_log("write\t\t- 创建文件\n");
	_log("rmdir\t\t- 删除目录\n");
	_log("ls\t\t- 显示目录\n");
	_log("cat\t\t- 打开文件\n");
	_log("rm\t\t- 删除文件\n");
	_log("free\t\t- 显示空间\n");
	_log("cd\t\t- 更改当前目录\n");
	_log("show\t\t- 显示TargetVal值\n");
	_log("pwd\t\t- 显示当前目录\n");
	_log("cls\t\t- 清除屏幕\n");
	_log("ver\t\t- 显示版本信息\n");
	_log("exit\t\t- 退出\n");
	_log("shell\t\t- 运行shell脚本\n");
	_log("alias\t\t- 命名重名\n");
}

void ShowVer(void)
{
	_log("\n\r**************************************************************\n");
	_log("\r*		 Welcom to genie shell	            \n");
	_log("\r*		   Author:YangYe 20021102   	    \n");
	_log("\r*		VC version by 文佳 20050409         \n");
	_log("\r*		   changed by stophin  20140601     \n");
	_log("\r*	  use \"help\" command to help yourself \n");
	_log("\r*		bug report: ganganwen@163.com		\n");
	_log("\r*		  last edited by:stophin			\n");
	_log("\r*	use ESC or TAB to switch among tasks	\n");
	_log("\r**************************************************************\n\n");
}

void ShowPwd(void)
{
	_log("\nRAM:");
	_log(CurDir);
	_log(">");
}

void ClearScr(void)
{
	system("cls");
}