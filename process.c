#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=0;
typedef struct output{
int value;
token *tok;
}output;


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
			tok++;
		}
		//setting any redirections
		while(RED_OP(tok[0].type))
		{
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
			    out.value=status;
			}
		}
		//if not we know that there has to be a subcommand
		if(tok[0].type==PAR_LEFT)
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
