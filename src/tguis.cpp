#include <getopt.h>

#include "tkernel.h"
#include "tmessage.h"
#include "tguis.h"

TGUIS::TGUIS( TKernel *app ) : TGRPModule(app,"GUI")
{

}

void TGUIS::Start(  )
{

}

void TGUIS::pr_opt_descr( FILE * stream )
{
    fprintf(stream,
    "========================= Protocol options ================================\n"
    "    --GUIModPath=<path>  Set moduls <path>;\n"
    "\n");
}

void TGUIS::CheckCommandLine( char **argv, int argc )
{
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"GUIModPath" ,1,NULL,'m'},
	{NULL         ,0,NULL,0  }
    };

    optind=opterr=0;	
    do
    {
	next_opt=getopt_long(argc,argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': pr_opt_descr(stdout); break;
	    case 'm': DirPath = optarg;     break;
	    case -1 : break;
	}
    } while(next_opt != -1);
//    if(optind < App->argc) pr_opt_descr(stdout);
}

void TGUIS::UpdateOpt()
{

}
