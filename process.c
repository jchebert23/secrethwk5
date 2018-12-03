#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=1;
typedef struct output{
int value;
token *tok;
}output;

typedef struct redirections{
int input;
int output;
}redirections;
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
    if(type==RED_IN)
    {
	    int fd=open(path, O_RDONLY);
	    dup2(fd, STDIN_FILENO);
    }
}

struct output process_pipeline(token *tok)
{
	struct output out;
	while(tok[0].type==PIPE || tok[0].type==ARG)
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
		while(RED_OP(tok[0].type))
		{
			redirection(tok[0].type, tok[0].text);
			tok++;
		}
		pid_t pid;
		//if their were any arguments
		if(numArgs!=0)
		{
			//have to add terminating 0 to list of arguments
			argArr[numArgs]=0;
			pid=fork();
			if(pid==0)
			{
			    execvp(argArr[0], argArr);	
			}
			else
			{
			    int status;
			    waitpid(pid, &status, 0);
			    close(STDIN_FILENO)
			    out.value=status;
			}
		}
		//if not we know that there has to be a subcommand
		else if(tok[0].type==PAR_LEFT)
		{
		    tok++;
		    out.value=process(tok);
		    int leftParen=0;
		    while(!(tok[0].type==PAR_RIGHT  && leftParen==0))
		    {
			if(tok[0].type==PAR_LEFT)
			{
			leftParen++;
			}
			else if(tok[0].type==PAR_RIGHT)
			{
			leftParen--;
			}
			tok++;
		    }
		    //increment it one more time so we get past the right parenthesis
		    tok++;
		}
		else
		{
			//IMPORTANT
			printf("THIS IS BAD, CODE SHOULD NEVER REACH THIS POINT. NO ARGS AND NO LEFT PARENTHESIS\n");
	
		}
		out.tok=tok;
	}
	return out;
}

int process_and_or(token *tok)
{
process_pipeline(tok);
return 0;
}

int process_list(token *tok)
{
process_and_or(tok);
return 0;
}

int process(token *tok){
    int status;

    if(strcmp(tok[0].text, "wait")==0)
    {
	    int returnedVal;
	    while((returnedVal=waitpid(-1, &status, WNOHANG))==0)
	    {
	    }
    }
    process_list(tok);
    return 0;

}
