#include <getopt.h>

#include "tsys.h"
#include "tkernel.h"
#include "tmessage.h"
#include "tbds.h"
#include "tcontroller.h"
#include "ttipcontroller.h"
#include "tmodule.h"
#include "tvalue.h"
#include "tcontrollers.h"


SCfgFld TControllerS::gen_elem[] =
{
    {"NAME"  ,"Controller's name."               ,CFG_T_STRING              ,"","",""           ,"20",""          ,"%s"},
    {"MODUL" ,"Module(plugin) of type controler.",CFG_T_STRING              ,"","",""           ,"20",""          ,"%s"},
    {"BDTYPE","Type controller's BD."            ,CFG_T_STRING              ,"","","direct_dbf" ,"20",""          ,"%s"},
    {"BDNAME","Name controller's BD."            ,CFG_T_STRING              ,"","","./DATA"     ,"50",""          ,"%s"},
    {"TABLE" ,"Name controller's Table."         ,CFG_T_STRING              ,"","","contr.dbf"  ,"20",""          ,"%s"},
    {"STAT"  ,"Controller's stat."               ,CFG_T_BOOLEAN|CFG_T_SELECT,"","","false"      ,"1" ,"false;true","%s","Disable;Enable"}
};

const char *TControllerS::o_name = "TControllerS";
const char *TControllerS::n_opt  = "Controller";

TControllerS::TControllerS( TKernel *app ) : TGRPModule(app,"Controller"), TConfig(NULL), 
	t_bd("direct_dbf"), n_bd("./DATA"), n_tb("generic.dbf")
{
    TConfigElem *gen_ecfg = new TConfigElem;   
    for(unsigned i = 0; i < sizeof(gen_elem)/sizeof(SCfgFld); i++) gen_ecfg->cfe_Add(&gen_elem[i]);    
    cf_ConfElem(gen_ecfg);
}

TControllerS::~TControllerS(  )
{
    vector<string> List;
    gmd_StopAll();
    for(unsigned i_ctr = 0; i_ctr < Contr.size(); i_ctr++)
	if( Contr[i_ctr].use ) DelContr( i_ctr );	
    for(unsigned i_m = 0; i_m < gmd_Size(); i_m++) gmd_DelM(i_m);
    TConfigElem *gen_ecfg = cf_ConfElem();
    cf_ConfElem(NULL);
    delete gen_ecfg;
}

void TControllerS::gmd_InitAll( )
{
    TGRPModule::gmd_InitAll();
    LoadBD();
}

int TControllerS::gmd_StartAll(  )         
{
    for(unsigned i=0; i< Contr.size(); i++)
	if( Contr[i].id_mod >= 0 ) 
	{
	    try{ at_tp(Contr[i].id_mod)->at(Contr[i].id_contr)->Start( ); }
	    catch(TError err) {  Mess->put(1,"%s",err.what().c_str()); }
	}

    return(0);
}

int TControllerS::gmd_StopAll(  )
{
//    LoadBD();
    for(unsigned i=0; i< Contr.size(); i++)
	if( Contr[i].id_mod >= 0 )
	{
	    try{ at_tp(Contr[i].id_mod)->at(Contr[i].id_contr)->Stop( ); }
	    catch(TError err) {  Mess->put(1,"%s",err.what().c_str()); }
	}

    return(0);
}

void TControllerS::ContrList( vector<string> & List )
{
    List.clear();
    for(unsigned i=0;i < Contr.size(); i++)
	if(Contr[i].id_mod >= 0 ) 
	    List.push_back(at_tp(Contr[i].id_mod)->at(Contr[i].id_contr)->Name());
}


void TControllerS::pr_opt_descr( FILE * stream )
{
    fprintf(stream,
    "========================= TipController options ===========================\n"
    "    --TCModPath=<path>   Set moduls <path>;\n"
    "    --TCTypeGenBD=<name> Set a name of type generic BD (default \"direct_dbf\");\n"
    "    --TCNameGenBD=<name> Set a name of generic BD (default \"./DATA\");\n"
    "    --TCNameGenTB=<name> Set a name of generic table (default \"generic.dbf\");\n"
    "------------------ Fields <%s> sections of config file ----------------\n"
    " modules_path = <path>     path to modules;\n"
    " TypeGenBD    = <name>     type generic bd (modules name);\n"
    " NameGenBD    = <name>     name generic bd;\n"    
    " NameGenTB    = <name>     name generic table;\n"
    " GenBD        = <fullname> generic bd recorded: \"<TypeBD>:<NameBD>:<NameTable>\";\n"
    "\n",n_opt);
}

void TControllerS::gmd_CheckCommandLine( )
{
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"TCModPath"  ,1,NULL,'m'},
	{"TCTypeGenBD",1,NULL,'t'},
	{"TCNameGenBD",1,NULL,'b'},
	{"TCNameGenTB",1,NULL,'l'},	
	{NULL         ,0,NULL,0  }
    };

    optind=opterr=0;	
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': pr_opt_descr(stdout); break;
	    case 'm': DirPath  = optarg;    break;
	    case 't': t_bd     = optarg;    break;
	    case 'b': n_bd     = optarg;    break;
	    case 'l': n_tb     = optarg;    break;
	    case -1 : break;
	}
    } while(next_opt != -1);
}

void TControllerS::gmd_UpdateOpt()
{
    string opt;
    
    if( SYS->GetOpt(n_opt,"modules_path",opt) ) DirPath = opt;
    if( SYS->GetOpt(n_opt,"TypeGenBD",opt) )    t_bd = opt;
    if( SYS->GetOpt(n_opt,"NameGenBD",opt) )    n_bd = opt;
    if( SYS->GetOpt(n_opt,"NameGenTB",opt) )    n_tb = opt;
    if( SYS->GetOpt(n_opt,"GenBD",opt) )
    {
    	int pos = 0;
	t_bd = opt.substr(pos,opt.find(":",pos)-pos); pos = opt.find(":",pos)+1;
	n_bd = opt.substr(pos,opt.find(":",pos)-pos); pos = opt.find(":",pos)+1;
	n_tb = opt.substr(pos,opt.find(":",pos)-pos); pos = opt.find(":",pos)+1;
	if( !t_bd.size() ) t_bd = owner->DefBDType;
	if( !n_bd.size() ) n_bd = owner->DefBDName;
    }    
}

void TControllerS::LoadBD()
{
    string cell;
    //bool   reload = false;
    //---- NEW ----
    try
    {
    	int b_hd = owner->BD->OpenTable(t_bd,n_bd,n_tb);
	cf_LoadAllValBD( owner->BD->at_tbl(b_hd) );
	cf_FreeDubl("NAME",true);
	owner->BD->CloseTable(b_hd);
    }catch(TError err) { Mess->put(1,"%s: %s",o_name,err.what().c_str()); }
    //Create controller 
    for(unsigned i_cfg = 0; i_cfg < cf_Size(); i_cfg++)
	if( cf_Get_SEL("STAT", i_cfg) == "Enable" )
	{
	    //Check avoid controller
	    try{ NameCntrToId( cf_Get_S("NAME", i_cfg) ); }
	    catch(...)
    	    {
		try
		{
		    int hd = AddContr(  cf_Get_S("NAME", i_cfg), 
					cf_Get_S("MODUL", i_cfg), 
					cf_Get_S("BDTYPE", i_cfg), 
					cf_Get_S("BDNAME", i_cfg), 
					cf_Get_S("TABLE", i_cfg) );
		    at(hd)->Enable();
		}
		catch(TError err) { Mess->put(2,"%s: %s",o_name,err.what().c_str()); }
	    }
	}									    
}


void TControllerS::UpdateBD(  )
{
    int b_hd;
    string cell, stat;
    try { b_hd = owner->BD->OpenTable(t_bd,n_bd,n_tb); }
    catch(...) { b_hd = owner->BD->OpenTable(t_bd,n_bd,n_tb,true); }
    cf_ConfElem()->cfe_UpdateBDAttr( owner->BD->at_tbl(b_hd) );
    cf_SaveAllValBD(owner->BD->at_tbl(b_hd));
    owner->BD->at_tbl(b_hd)->Save();
    owner->BD->CloseTable(b_hd);
    for(unsigned i=0;i < Contr.size(); i++) if( Contr[i].use ) at(i)->Save();    
}

unsigned TControllerS::AddContr( string name, string tip, string t_bd, string n_bd, string n_tb )
{
    unsigned i;
    //!!! Want request resource 
    try{ NameCntrToId( name ); }
    catch(...)
    {
#if OSC_DEBUG
	Mess->put(0, "Add controller <%s>!",name.c_str());
#endif
	
    	for(i=0;i < Contr.size(); i++)
	    if( !Contr[i].use ) break;
	if(i == Contr.size() ) Contr.push_back();
	Contr[i].use      = true;
	Contr[i].id_mod   = gmd_NameToId(tip);
	Contr[i].id_contr = at_tp(Contr[i].id_mod)->Add(name,t_bd,n_bd,n_tb);
    	return(i);
    }
    throw TError("%s: Controller %s already avoid!",o_name,name.c_str());
}

void TControllerS::DelContr( unsigned id )
{
    //!!! Want request resource 
    if(id >= Contr.size() || !Contr[id].use ) 
	throw TError("%s: Controller %d error!",o_name,id);
#if OSC_DEBUG
    Mess->put(0, "Del controller <%s>!",at(id)->Name().c_str());
#endif
    at_tp(Contr[id].id_mod)->Del(Contr[id].id_contr);
    Contr[id].use = false;
}

int TControllerS::gmd_AddM( TModule *modul )
{
    int hd=TGRPModule::gmd_AddM(modul);
    at_tp(hd)->owner = this;
    
    return(hd);
}

TController *TControllerS::at( unsigned id)
{ 
    if(id >= Contr.size() || !Contr[id].use ) 
	throw TError("%s: Controller %d error!",o_name,id);
    return(at_tp(Contr[id].id_mod)->at(Contr[id].id_contr));
}

unsigned TControllerS::NameCntrToId( string Name )
{
    for(unsigned i_id = 0; i_id < Contr.size(); i_id++)
        if( Contr[i_id].use && at_tp(Contr[i_id].id_mod)->at(Contr[i_id].id_contr)->Name() == Name ) 
	    return(i_id);
    throw TError("%s: %s controller no avoid!",o_name,Name.c_str());
}


