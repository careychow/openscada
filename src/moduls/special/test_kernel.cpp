#include <getopt.h>
#include <string>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../../tsys.h"
#include "../../tkernel.h"
#include "../../tmessage.h"
#include "../../tparams.h"
#include "../../tparam.h"
#include "../../tparamcontr.h"
#include "test_kernel.h"

//============ Modul info! =====================================================
#define NAME_MODUL  "test_kernel"
#define NAME_TYPE   "Special"
#define SUB_TYPE    "TEST"
#define VERSION     "0.0.3"
#define AUTORS      "Roman Savochenko"
#define DESCRIPTION "OpenScada Kernel test module: Configs, Values ... ."
#define LICENSE     "GPL"
//==============================================================================

extern "C"
{
    TModule *attach( char *FName, int n_mod )
    {
	KernelTest::TTest *self_addr;
	if(n_mod==0) self_addr = new KernelTest::TTest( FName );
	else         self_addr = NULL;
	return ( self_addr );
    }
}

using namespace KernelTest;

//==============================================================================
//================= BDTest::TTest ==============================================
//==============================================================================
TTest::TTest(char *name)
{
    NameModul = NAME_MODUL;
    NameType  = NAME_TYPE;
    Vers      = VERSION;
    Autors    = AUTORS;
    DescrMod  = DESCRIPTION;
    License   = LICENSE;
    FileName  = strdup(name);
}

TTest::~TTest()
{
    free(FileName);	
}

string TTest::mod_info( const string name )
{
    if( name == "SubType" ) return(SUB_TYPE);
    else return( TModule::mod_info( name) );
}

void TTest::mod_info( vector<string> &list )
{
    TModule::mod_info(list);
    list.push_back("SubType");
}

void TTest::pr_opt_descr( FILE * stream )
{
    fprintf(stream,
    "============== Module %s command line options =======================\n"    
    "------------------ Fields <%s> sections of config file --------------\n"
    "XML=<1>                enable XML parsed and create test (1 - on; 0 - off);\n"
    "  XML_pars_fl=<name>   set <name> file for XML parsing;\n"
    "\n",NAME_MODUL,NAME_MODUL);
}

void TTest::mod_CheckCommandLine(  )
{
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{NULL        ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': pr_opt_descr(stdout); break;
	    case -1 : break;
	}
    } while(next_opt != -1);
}

void TTest::mod_UpdateOpt( )
{

}

void TTest::Start(  )
{
    Mess->put(MESS_NOTICE,"***** Begin <%s> test block *****",NAME_MODUL);
    TParamS &param = Owner().Owner().Param();
    //Owner().Controller->AddContr("test3","virtual_v1","virt_c");
    //Owner().Controller->at("test3")->Add("ANALOG","TEST_VirtualC",-1);
    //Owner().Controller->at("test3")->Del("ANALOG","TEST_VirtualC");
    //Owner().Controller->DelContr("test3");
    //Owner().Controller->UpdateBD();
    
    /*
    vector<string> list_ct,list_c,list_pt,list_pc;
    Owner().Controller->List(list_ct);
    Mess->put(1,"Controller types: %d",list_ct.size());
    for(int i=0; i < list_ct.size(); i++)
    {
	try
	{
	    Mess->put(1,"Controller type: <%s>",list_ct[i].c_str());

	    Owner().Controller->at_tp(list_ct[i])->ListTpPrm(list_pt);
	    Mess->put(1,"Types param's: %d",list_pt.size());
	    for(int ii=0; ii < list_pt.size(); ii++)
	    Mess->put(1,"Type: <%s>",list_pt[ii].c_str());

	    Owner().Controller->at_tp(list_ct[i])->List(list_c);
	    Mess->put(1,"Controllers: %d",list_c.size());
	    for(int ii=0; ii < list_c.size(); ii++)
	    {
		Mess->put(1,"Controller: <%s>",list_c[ii].c_str());
		for(int i_pt=0; i_pt < list_pt.size(); i_pt++)
		{
		    Owner().Controller->at(list_c[ii])->List(list_pt[i_pt],list_pc);
		    Mess->put(1,"%s Parameters: %d",list_pt[i_pt].c_str(),list_pc.size());
		    for(int iii=0; iii < list_pc.size(); iii++)
		    Mess->put(1,"Parameter: <%s>",list_pc[iii].c_str());
		}
	    }
	}
	catch(TError err){ }
    }
    //---------------- All parameter's list ----------------
    Owner().Param->List(list_pc);
    Mess->put(1,"Params: %d",list_pc.size());
    for(unsigned i=0; i < list_pc.size(); i++)
    Mess->put(1,"Param: <%s>",list_pc[i].c_str());
    */
			    
    //---------------- Configs element's test ----------------
    try
    {
	vector<string> list_el;
	param[param.NameToHd("TEST_VirtualC")].at().cf_ListEl(list_el);
	Mess->put(MESS_NOTICE,"%s: <%s> config elements: %d",NAME_MODUL,"TEST_VirtualC",list_el.size());
	for(unsigned i=0; i< list_el.size(); i++)
	Mess->put(MESS_NOTICE,"%s: Element: %s",NAME_MODUL,list_el[i].c_str());
    } catch(TError error)
    { Mess->put(MESS_NOTICE,"%s: %s",NAME_MODUL,error.what().c_str()); }
    //---------------- Values element's test ----------------
    try
    {
	STime tm = {0,0};
	vector<string> list_el;
	param[param.NameToHd("TEST_VirtualC")].at().vl_Elem().vle_List(list_el);
	Mess->put(MESS_NOTICE,"%s: <%s> value elements: %d",NAME_MODUL,"TEST_VirtualC",list_el.size());
	param[param.NameToHd("TEST_VirtualC")].at().vl_SetI(0,30,tm);
	for(unsigned i=0; i< list_el.size(); i++)
    	    Mess->put(MESS_NOTICE,"%s: Element: %s: %f (%f-%f)",NAME_MODUL,list_el[i].c_str(),
		param[param.NameToHd("TEST_VirtualC")].at().vl_GetR(i,tm),
		param[param.NameToHd("TEST_VirtualC")].at().vl_GetR(i,tm,V_MIN),
		param[param.NameToHd("TEST_VirtualC")].at().vl_GetR(i,tm,V_MAX) );
    } catch(TError error)
    { Mess->put(MESS_NOTICE,"%s: %s",NAME_MODUL,error.what().c_str()); }

    //=============== Test XML =====================
    try
    {
	XMLNode *t_n = mod_XMLCfgNode()->get_child("XML");
	if( atoi(t_n->get_text().c_str()) == 1 )
    	{
    	    int hd = open(t_n->get_attr("file").c_str(),O_RDONLY);
    	    if(hd > 0)
    	    {
		Mess->put(MESS_NOTICE,"-------- Start TEST XML parsing ----------");
	    	int cf_sz = lseek(hd,0,SEEK_END);
		lseek(hd,0,SEEK_SET);
		char *buf = (char *)malloc(cf_sz);
		read(hd,buf,cf_sz);
		close(hd);
		string s_buf = buf;
		free(buf);
		XMLNode node;
		node.load_xml(s_buf);
		pr_XMLNode( &node, 0 );
		Mess->put(MESS_NOTICE,"-------- End TEST XML parsing ----------");
	    }
	}
    }
    catch(...) {  }
    //=============== Test XML =====================
    
    Mess->put(MESS_NOTICE,"***** End <%s> test block *****",NAME_MODUL);
}

void TTest::pr_XMLNode( XMLNode *node, int level )
{
    char *buf = (char *)malloc(level+1);
    for(int i_c = 0; i_c < level; i_c++) buf[i_c] = ' ';
    buf[level] = 0;
	
    vector<string> list;
    Mess->put(MESS_NOTICE,"%s{%d <%s>, text <%s>, childs - %d!",
	buf, level, node->get_name().c_str(),node->get_text().c_str(),node->get_child_count());
    node->get_attr_list(list);
    for(unsigned i_att = 0; i_att < list.size(); i_att++)
	Mess->put(MESS_NOTICE,"        Attr <%s> = <%s>!",list[i_att].c_str(),node->get_attr(list[i_att]).c_str());	
    for(int i_ch = 0; i_ch < node->get_child_count(); i_ch++)
	pr_XMLNode( node->get_child(i_ch), level+1 ); 
    Mess->put(MESS_NOTICE,"%s}%d <%s>", buf, level, node->get_name().c_str());
    free(buf);
}

