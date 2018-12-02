#include "/c/cs323/Hwk5/process-stub.h"

int debugPrint=1;
typedef struct output{
int value;
token *tok;
}output;

char * concatenateArguments(token *tok)
{

int argLength=0;
token *origTok=tok;
while(tok[0].type==ARG)
{
	// the plus one account for the space between each argument
	argLength+=strlen(tok[0].text)+1;
	tok++;
}
char *args=0;
if(argLength!=0)
{
	char *args=malloc(sizeof(char)*(argLength)); args[0]='\0';
	while(origTok[0].type==ARG)
	{
		sprintf(args+strlen(args), origTok[0].text);
		if(origTok!=tok-1){
		sprintf(args+strlen(args), " ");
		}
		origTok++;
	}
	args[argLength-1]='\0';
}
return args;
}

struct output process_pipeline(token *tok)
{
struct output out;
while(tok[0].type==PIPE || tok[0].type==ARG)
{
	char *args = concatenateArguments(tok);
	if(debugPrint){printf("Concatenated Arguments: %s\n", args);}
	while(tok[0].type==ARG){tok++;}
	while(tok[0].type==LOCAL)
	{
		tok++;
	}
	while(RED_OP(tok[0].type))
	{
		tok++;
	}
	if(args){free(args);}
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
