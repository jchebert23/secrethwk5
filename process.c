#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=0;
int debugPrintChild=0;
int debugPrintExitStatus=0;
#define ERROREXIT(arg, status)  (perror(arg), exit(status))

int process(token *tok);

//function to break down local string, and set environment variable
void local(char *s)
{
	//IMPORTANT, deal with case where $PATH=$PATH
	char *equalsSign=strchr(s, '=');
	int lengthOfFirstHalf=equalsSign-s;
	char firstHalf[lengthOfFirstHalf+1];
	memcpy(firstHalf, s, lengthOfFirstHalf);
	if(debugPrint)
	{
	    printf("First half of local equality: %s\n", firstHalf);
	    printf("Second half of local equality: %s\n", s+lengthOfFirstHalf+1);
	}
	setenv( firstHalf, s+lengthOfFirstHalf+1, 1);
}

//function to change file descriptors based on the redirection
void redirection(int type, char *path)
{
    int fd;
    if(type==RED_IN)
    {
	    fd=open(path, O_RDONLY);
	    dup2(fd, STDIN_FILENO);
	    close(fd);
    }
    else if(type==RED_IN_HERE)
    {
	    char template[16]="tempInputXXXXXX";
	    template[15]='\0';
	    fd=mkstemp(template);
	    size_t nbytes=strlen(path);
	    write(fd, path, nbytes);
	    close(fd);
	    fd=open(template, O_RDONLY);
	    dup2(fd, STDIN_FILENO); 
	    if(remove(template)!=0)
	    {
		    printf("%s not deleted successfully\n", template);
	    } 
	    close(fd);
    }
    else if(type==RED_OUT)
    {
	    //IMPORTANT PERMISSIONS ON OPENING FILE
	    fd=open(path, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR|S_IRUSR);
	    dup2(fd, STDOUT_FILENO);
	    close(fd);
    }
    else if(type==RED_OUT_APP)
    {
	    //IMPORTANT PERMISSIONS ON OPENING FILE
	    fd=open(path, O_WRONLY|O_APPEND|O_CREAT, S_IWUSR|S_IRUSR);
	    dup2(fd, STDOUT_FILENO);
	    close(fd);
    }
    else if(type==RED_ERR_OUT)
    {
	    dup2(1, 2);
    }
    else
    {
	    close(2);
    }
}

token *traverseParenthesis(token *tok)
{
    while(1)
    {
	    if(tok[0].type==PAR_RIGHT)
	    {
		tok++;
		return tok;
	    }
	    else if(tok[0].type==PAR_LEFT)
	    {
		tok++;
		tok=traverseParenthesis(tok);
	    }
	    else
	    {
		tok++;
	    }
    }
}

token *traverseList(token *tok)
{
    if(tok==0)
    {
	    return 0;
    }
    while(1)
    {
	if(tok[0].type==NONE || tok[0].type==PAR_RIGHT)
	{
		return 0;
	}
	else if(tok[0].type==PAR_LEFT)
	{
		tok++;
		tok=traverseParenthesis(tok);
	}
	else if(tok[0].type==SEP_END || tok[0].type==SEP_BG)
	{
		return tok;
	}
	else
	{
		tok++;
	}
    }
}

token *traverseAndOr(token *tok)
{
    while(tok[0].type!=SEP_END && tok[0].type!=SEP_BG)
    {
	if(tok[0].type==PAR_LEFT)
	{
	    tok++;
	    tok=traverseParenthesis(tok);
	}
	else if(tok[0].type==SEP_AND || tok[0].type==SEP_OR)
	{
	    return tok;
	}
	else
	{
	    tok++;
	}
    }
    return 0;
}

token * pipeExists(token *tok)
{
    while(tok[0].type!=SEP_AND && tok[0].type!=SEP_OR && tok[0].type!=SEP_END && tok[0].type!=SEP_BG)
    {
	if(tok[0].type==PAR_LEFT)
	{
		tok++;
		tok=traverseParenthesis(tok);
	}
	else if(tok[0].type==PIPE)
	{
		tok++;
		return tok;
	}
	else
	{
		tok++;
	}
    }
    return 0;
}

void process_stage(token *tok)
{
	token *origTok=tok;
	int numArgs=0;
	while(tok[0].type==ARG){
		numArgs++;
		tok++;}
	char *argArr[numArgs+1];
	tok=origTok;
	int i=0;
	while(tok[0].type==ARG){
	    argArr[i]=tok[0].text;
	    tok++;
	    i++;
	}
	//setting any locals
	while(tok[0].type==LOCAL)
	{
		local(tok[0].text);
		tok++;
	}
	//if their were any arguments
	if(numArgs!=0)
	{
		//have to add terminating 0 to list of arguments
		argArr[numArgs]=0;	
		while(RED_OP(tok[0].type))
		{
		    redirection(tok[0].type, tok[0].text);
		    tok++;
		}
		//need shit here
		int i=execvp(argArr[0], argArr);
		ERROREXIT(argArr[0],i); 
		    
	}
	//if not we know that there has to be a subcommand
	else
	{
		    while(RED_OP(tok[0].type))
		    {
			redirection(tok[0].type, tok[0].text);
			tok++;
		    }
		    if(tok[0].type!=PAR_LEFT)
		    {
			printf("CODE SHOULD NOT BE HERE, ERROR IN LOGIC\n");
			exit(0);
		    }
	    //incrementing it to get past left parenthesis
		    tok++;
		    exit(process(tok));
	}
}


int getStatusFromPipeline(int *arr, int length)
{
    
	int actual;
	actual=0;
	int fakeStatus;
	for(int i=0; i<=length; i++)
	{
		waitpid(arr[i], &fakeStatus, 0);
		if(STATUS(fakeStatus)!=0)
		{
			actual=STATUS(fakeStatus);
		}
		else
		{
			actual=actual;
		}
	}
	free(arr);
	return actual;
}

int process_pipeline(token *tok)
{

    int pipeExist=0;
    int fd[2];
    int pid;
    int fdin=-1;
    int status=0;
    int *pidArr=malloc(sizeof(int));
    int index=0;
    //int fakeStatus;
    if(pipeExists(tok))
    {
	    pipeExist=1;
    }
    while(pipeExist)
    {
	    //IMPORTANT, ERRORS FOR IF PIPE FAILS
	    if(pipe(fd) || (pid=fork()) < 0)
	    {
		    printf("Pipe or fork doesn't work\n");
	    }
	    else if(pid==0)
	    {
		    close(fd[0]);
		    //this tests whether it is first thing in pipe chain
		    if(fdin>=0)
		    {
			  //makes stdin the stdout of last call to pipe  
			    dup2(fdin,0);
			    close(fdin);
		    }
		    if(fd[1] != 1)
		    {
			    //makes stdout this file
			    dup2(fd[1], 1);
			    close(fd[1]);
		    }
		    //this will turn into an exec call
		    process_stage(tok);
	    }
	    else
	    {
		pidArr[index]=pid;
		index++;
		pidArr=realloc(pidArr, sizeof(int)*(index+1));
		if(fdin>=0)
		{
			close(fdin);
		}
		if(debugPrintExitStatus)
		{
			printf("DONE PIPE WAIT\n");
		}
		tok=pipeExists(tok);
		fdin=fd[0];
		close(fd[1]);
	    }
	    if(pipeExists(tok)==0)
	    {
		pipeExist=0;
	    }
    }
    //last stage, where there are no pipes afterwards
    //IMPORTANT ERROR ON THIS FORK
    if((pid = fork())<0)
    {
	    printf("Fork does not work on last fork\n");
    }
    else if(pid==0)
    {
	if(fdin>=0)
	{
	    dup2(fdin,0);
	    close(fdin);
	}
	//this will turn into an exec call
	process_stage(tok);
    }
    else
    {
	pidArr[index]=pid;
	status=getStatusFromPipeline(pidArr, index);
	//while((wait(&fakeStatus))>0);	
	//status = STATUS(fakeStatus);
	if(debugPrintExitStatus)
	{
		printf("Child process just ended with status: %d\n", status);
	}
	if(fdin >=0)
	{
		close(fdin);
	}
    }
    return status;
}


int process_and_or(token *tok)
{

	int first=1;
	int and=0;
	int or=0;
	int status=0;
	int pid =0;
	int fakeStatus;
	while(1)
	{
	    and=0;
	    or=0;
	    if(tok[0].type==SEP_AND)
	    {
		    and=1;
	    }
	    else
	    {
		    or=1;
	    }
	    if(!first)
	    {
	    tok++;
	    }
	    if((or && (status!=0)) || (and && (status==0)) || first)
	    {
		    pid=fork();
		    if(pid==0)
		    {
			    exit(process_pipeline(tok));
		    }
		    else
		    {
			    waitpid(pid,&fakeStatus,0);
			    status=STATUS(fakeStatus);
			    
			char buff[30];
			sprintf(buff,"%d", status);
			setenv("?",buff , 1);
		    }
	    }
	    if(first){first=0;}
	    if(traverseAndOr(tok))
	    {
		    tok=traverseAndOr(tok);
	    }
	    else
	    {
		    break;
	    }
	}
	return status;
}
int process_list(token *tok)
{
	int pid;
	int status=0;
	while(1)
	{
	    token *tempTok=traverseList(tok);
	    if(tempTok==0)
	    {
		    break;
	    }
	    else if(tempTok[0].type==SEP_END)
	    {
		//IMPORTANT, fork error here    
		   status = process_and_or(tok);
	    }
	     else
	     {
		//IMPORTANT, fork error here
		pid=fork();
		if(pid==0)
		{
			exit(process_and_or(tok));
		}
		else
		{
		    fprintf(stderr, "Backgrounded: %d\n", pid);
		    status=0;
		}
	     }
	     tok=traverseList(tok);
	     tok++;
	}
	return status;
}

int process(token *tok){
    int status;
    int pid;
    if((pid=waitpid((pid_t) -1, &status, WNOHANG))>0)
    {
	    fprintf(stderr, "Completed: %d (%d)\n", pid, status);
    }
    return process_list(tok);
}
