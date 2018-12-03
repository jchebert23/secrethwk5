#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=0;
int debugPrintChild=0;
int debugPrintExitStatus=0;
#define ERROREXIT(arg, status)  (perror(arg), exit(status))
typedef struct redirections{
int input;
int output;
}redirections;

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
	    size_t nbytes=strlen(path)*2;
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
	    fd=open(path, O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR);
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

int process_stage(token *tok, int background)
{
	token *origTok=tok;
	int numArgs=0;
	int status=0;
	int fakeStatus=0;
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
	//printing out argument list
	if(debugPrint)
	{
		printf("Args: ");
		for(int x=0; x<numArgs; x++)
		{
			printf("%s ", argArr[x]);
		}
		printf("\n");
	}
	//setting any locals
	while(tok[0].type==LOCAL)
	{
		local(tok[0].text);
		tok++;
	}
	//setting any redirections
	pid_t pid;
	//if their were any arguments
	if(numArgs!=0)
	{
		//have to add terminating 0 to list of arguments
		argArr[numArgs]=0;
		pid=fork();
		if(pid==0)
		{
			
		    if(debugPrintChild)
		    {
			    printf("In child process: %d\n", getpid());
		    }
		    
			while(RED_OP(tok[0].type))
			{
				redirection(tok[0].type, tok[0].text);
				tok++;
			}
		    //need shit here
		    int i=execvp(argArr[0], argArr);
		    ERROREXIT(argArr[0],i); 
		}
		else
		{
		    while(RED_OP(tok[0].type)){tok++;}
		    if(background==0)
		    {
			    waitpid(pid, &fakeStatus, 0);
			    status = STATUS(fakeStatus);
			    if(debugPrintExitStatus)
			    {
				    printf("Stage just ended with status:%d , fake status: %d\n", status, fakeStatus);
			    }
		    }
		    else
		    {
			    //IMPORTANT HOW TO PRINT BACKGROUNDED
			    fprintf(stderr, "Backgrounded: %d\n", pid);
			    status=0;
		    }
		}
		    
	}
	//if not we know that there has to be a subcommand
	else
	{
	    
	    pid=fork();
	    if(pid==0)
	    {
		    if(debugPrintChild)
		    {
			    printf("In child process: %d\n", getpid());
		    }
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
	    else
	    {	    
		    while(RED_OP(tok[0].type)){tok++;}
		    if(background==0)
		    {
			    waitpid(pid, &fakeStatus, 0);
			    status=STATUS(fakeStatus);
		    }
		    else
		    {
			    fprintf(stderr, "Backgrounded: %d\n", pid);
			    status=0;
		    }
		    if(tok[0].type!=PAR_LEFT)
		    {
			printf("CODE SHOULD NOT BE HERE, ERROR IN LOGIC\n");
			exit(0);
		    }
	    //incrementing it to get past left parenthesis
		    tok++;
		    while(tok[0].type!=PAR_RIGHT)
		    {
			if(tok[0].type==PAR_LEFT)
			{
			tok++;
			tok=traverseParenthesis(tok);
			}
			else
			{
			tok++;
			}
		    }
		    //increment it one more time so we get past the right parenthesis
		    tok++;
		    }
	}
	return status;
}

int process_pipeline(token *tok, int background)
{

    int pipeExist=0;
    int fd[2];
    int pid;
    int fdin=-1;
    int status=0;
    int fakeStatus=0;
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
		    exit(process_stage(tok, background));
	    }
	    else
	    {
		if(fdin>=0)
		{
			close(fdin);
		}
		waitpid(pid, &fakeStatus, 0);
		status=STATUS(fakeStatus);
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
	int temp= process_stage(tok, background);
	//IMPORTANT WHICH EXIT FUNCTION TO USE
	if(debugPrintExitStatus)
	{
		printf("Exiting Wit Status: %d\n", temp);
	}
	exit(temp);
    }
    else
    {
	waitpid(pid, &fakeStatus, 0);
	status=STATUS(fakeStatus);
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


int process_and_or(token *tok, int background)
{

	int first=1;
	int and=0;
	int or=0;
	int status=0;
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
		    status=process_pipeline(tok, background);
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
	int background=0;
	token *origTok=tok;
	while(tok[0].type!=SEP_END)
	{
		if(tok[0].type==PAR_LEFT)
		{
			tok++;
			//if we find left paren, disregard all thats in between, until you find the right parenthesis
			//traverse parenthesis returns spot after closing parenthesis
			tok=traverseParenthesis(tok);
		}
		//if we find this seperator stuff is going to background
		else if(tok[0].type==SEP_BG)
		{
			background=1;
			break;
		}
		else
		{
		tok++;
		}
	}
	tok=origTok;
	if(debugPrintChild)
	{
		printf("Background: %d\n", background);
	}
	process_and_or(tok, background);
	return 0;
}

int process(token *tok){
    process_list(tok);
    return 0;
}
