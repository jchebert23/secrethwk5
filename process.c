#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=0;
int debugPrintChild=0;
typedef struct output{
int value;
token *tok;
}output;

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

struct output process_pipeline(token *tok, int background)
{
	struct output out;
	while(!(tok[0].type==SEP_END || tok[0].type==SEP_BG || tok[0].type==SEP_AND || tok[0].type==SEP_OR || tok[0].type==PAR_RIGHT))
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
			    execvp(argArr[0], argArr);	
			}
			else
			{
			    while(RED_OP(tok[0].type)){tok++;}
			    int status;
			    if(background==0)
			    {
				    waitpid(pid, &status, 0);
				    out.value=status;
			    }
			    else
			    {
				    //IMPORTANT HOW TO PRINT BACKGROUNDED
				    fprintf(stderr, "Backgrounded: %d\n", pid);
				    out.value=0;
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
			    process(tok);
			    exit(0);
		    }
		    else
		    {	    
			    while(RED_OP(tok[0].type)){tok++;}

			    int status;
			    if(background==0)
			    {
				    waitpid(pid, &status, 0);
				    out.value=status;
			    }
			    else
			    {

				    fprintf(stderr, "Backgrounded: %d\n", pid);
				    out.value=0;
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
		out.tok=tok;
	}
	return out;
}

int process_and_or(token *tok, int background)
{
	process_pipeline(tok, background);
	return 0;
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
