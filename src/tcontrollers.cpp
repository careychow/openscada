/***************************************************************************
 *   Copyright (C) 2004 by Roman Savochenko                                *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <getopt.h>

#include "tsys.h"
#include "tkernel.h"
#include "tmessage.h"
#include "tcontroller.h"
#include "ttipcontroller.h"
#include "tmodule.h"
#include "tvalue.h"
#include "tcontrollers.h"


const char *TControllerS::o_name = "TControllerS";

TControllerS::TControllerS( TKernel *app ) 
	: TGRPModule(app,"Controller","Controllers"), m_bd("", "", "generic") 
{
    fldAdd( new TFld("NAME","Controller's name.",T_STRING|F_KEY,"20") );
    fldAdd( new TFld("MODUL","Module(plugin) of type controler.",T_STRING|F_KEY,"20") );
    fldAdd( new TFld("BDTYPE","Type controller's BD.",T_STRING,"20","direct_dbf") );
    fldAdd( new TFld("BDNAME","Name controller's BD.",T_STRING,"50","./DATA") );
    fldAdd( new TFld("TABLE","Name controller's Table.",T_STRING,"20","contr.dbf") );    
}

TControllerS::~TControllerS(  )
{
    gmdStop();    
}

void TControllerS::gmdLoad( )
{
    //========== Load parameters from command line ============
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"       ,0,NULL,'h'},
	{NULL         ,0,NULL,0  }
    };

    optind=opterr=0;	
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,optDescr().c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);    
    
    //========== Load parameters from config file =============
    string opt;  
    try
    { 
	string opt = gmdCfgNode()->childGet("id","GenBD")->text(); 
	m_bd.tp	= TSYS::strSepParse(opt,0,':');
	m_bd.bd = TSYS::strSepParse(opt,1,':');
	m_bd.tbl= TSYS::strSepParse(opt,2,':');
    }
    catch(...) {  }
    
    //Load DB
    loadBD();
    
    //Load modules
    TGRPModule::gmdLoad( );
}

TBDS::SName TControllerS::BD() 
{ 
    return owner().nameDBPrep(m_bd); 
}

void TControllerS::gmdStart(  )         
{
    vector<string> m_l;
    gmdList(m_l);
    for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
    {
	vector<string> c_l;
	((TTipController &)gmdAt(m_l[i_m]).at()).list(c_l);
	for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	{
	    AutoHD<TController> cntr = ((TTipController &)gmdAt(m_l[i_m]).at()).at(c_l[i_c]);
	    if( !cntr.at().startStat() && cntr.at().toStart() )
		try{ cntr.at().start( ); }
		catch(TError err) { mPutS("SYS",TMess::Error,err.what()); }
	}
    }							    
}

void TControllerS::gmdStop( )
{
    vector<string> m_l;
    gmdList(m_l);
    for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
    {
	vector<string> c_l;
	((TTipController &)gmdAt(m_l[i_m]).at()).list(c_l);
	for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	{
	    AutoHD<TController> cntr = ((TTipController &)gmdAt(m_l[i_m]).at()).at(c_l[i_c]);
	    if( cntr.at().startStat() )
		try{ cntr.at().stop( ); }
		catch(TError err) { mPutS("SYS",TMess::Error,err.what()); }
	}
    }							    
}

string TControllerS::optDescr( )
{
    char buf[STR_BUF_LEN];
    snprintf(buf,sizeof(buf),Mess->I18N(
	"======================== The controller subsystem options =================\n"
	"------------ Parameters of section <%s> in config file -----------\n"
    	"GenBD     <fullname>       generic bd recorded: \"<TypeBD>:<NameBD>:<NameTable>\";\n\n"
	),gmdId().c_str());

    return(buf);
}

void TControllerS::loadBD()
{
    vector<string> list_el;
    TConfig g_cfg(this);        
    
    try
    {
	AutoHD<TTable> tbl = owner().db().open(BD());
	int fld_cnt=0;
        while( tbl.at().fieldSeek(fld_cnt++,g_cfg) )
	{
	    try
	    {
		SName CntrS(g_cfg.cfg("MODUL").getS().c_str(), g_cfg.cfg("NAME").getS().c_str());
		TBDS::SName n_bd(g_cfg.cfg("BDTYPE").getS().c_str(), g_cfg.cfg("BDNAME").getS().c_str(), g_cfg.cfg("TABLE").getS().c_str());
		
		((TTipController &)gmdAt(CntrS.tp).at()).add(CntrS.obj,n_bd);
		AutoHD<TController> ctr = ((TTipController &)gmdAt(CntrS.tp).at()).at(CntrS.obj);
		ctr.at().load();
		if( !ctr.at().enableStat() && ctr.at().toEnable() ) 
		    ctr.at().enable(); 
	    }
	    catch(TError err) { mPutS("SYS",TMess::Error,err.what()); }
	}
	tbl.free();	
	owner().db().close(BD());
    }catch(TError err) { mPutS("SYS",TMess::Error,err.what()); }    
}

void TControllerS::saveBD(  )
{	
    //Save all controllers    
    vector<string> m_l;
    gmdList(m_l);
    for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
    {
	vector<string> c_l;
	((TTipController &)gmdAt(m_l[i_m]).at()).list(c_l);
	for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	{
	    try{ ((TTipController &)gmdAt(m_l[i_m]).at()).at(c_l[i_c]).at().save( ); }
	    catch(TError err) { mPutS("SYS",TMess::Error,err.what()); }
	}
    }							    
}

//================== Controll functions ========================
void TControllerS::cntrCmd_( const string &a_path, XMLNode *opt, int cmd )
{
    if( cmd==TCntrNode::Info )
    {
	TGRPModule::cntrCmd_( a_path, opt, cmd );       //Call parent

	ctrInsNode("area",0,opt,a_path.c_str(),"/bd",Mess->I18N("Subsystem"),0440);
	if( owner().genDB( ) )
	    ctrMkNode("fld",opt,a_path.c_str(),"/bd/tbl",Mess->I18N("Table"),0660,0,0,"str");	    
	else
	{	    
	    ctrMkNode("fld",opt,a_path.c_str(),"/bd/t_bd",Mess->I18N("BD (module:bd:table)"),0660,0,0,"str")->
		attr_("dest","select")->attr_("select","/bd/b_mod");
	    ctrMkNode("fld",opt,a_path.c_str(),"/bd/bd","",0660,0,0,"str");	    
	    ctrMkNode("fld",opt,a_path.c_str(),"/bd/tbl","",0660,0,0,"str");
	}
	ctrMkNode("comm",opt,a_path.c_str(),"/bd/load_bd",Mess->I18N("Load from BD"));
	ctrMkNode("comm",opt,a_path.c_str(),"/bd/upd_bd",Mess->I18N("Save to BD"));
	ctrMkNode("fld",opt,a_path.c_str(),"/help/g_help",Mess->I18N("Options help"),0440,0,0,"str")->
	    attr_("cols","90")->attr_("rows","5");
    }
    else if( cmd==TCntrNode::Get )
    {
	if( a_path == "/bd/t_bd" )     ctrSetS( opt, m_bd.tp );
	else if( a_path == "/bd/bd" )  ctrSetS( opt, m_bd.bd );
	else if( a_path == "/bd/tbl" ) ctrSetS( opt, m_bd.tbl );
	else if( a_path == "/bd/b_mod" )
	{
	    vector<string> list;	
	    owner().db().gmdList(list);
	    opt->childClean();
	    ctrSetS( opt, "" );
	    for( unsigned i_a=0; i_a < list.size(); i_a++ )
		ctrSetS( opt, list[i_a] );
	}
	else if( a_path == "/help/g_help" ) ctrSetS( opt, optDescr() );       
	else TGRPModule::cntrCmd_( a_path, opt, cmd );
    }
    else if( cmd==TCntrNode::Set )
    {
	if( a_path == "/bd/t_bd" )       	m_bd.tp    = ctrGetS( opt );
	else if( a_path == "/bd/bd" )  		m_bd.bd    = ctrGetS( opt );
	else if( a_path == "/bd/tbl" )		m_bd.tbl   = ctrGetS( opt );
	else if( a_path == "/bd/load_bd" )	loadBD();
	else if( a_path == "/bd/upd_bd" )	saveBD();
	else TGRPModule::cntrCmd_( a_path, opt, cmd );	
    }
}

