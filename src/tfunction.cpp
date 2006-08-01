
//OpenSCADA system file: tfunction.cpp
/***************************************************************************
 *   Copyright (C) 2003-2006 by Roman Savochenko                           *
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

#include <math.h>

#include <tsys.h>
#include <tmess.h>
#include "tfunction.h"

//Function abstract object
TFunction::TFunction( const string &iid ) : m_id(iid), m_tval(NULL), run_st(false)
{

}

TFunction::~TFunction()
{
    for( int i_io = 0; i_io < m_io.size(); i_io++ )
	delete m_io[i_io];
}

void TFunction::preDisable(int flag)
{
    if( m_tval ) { delete m_tval; m_tval = NULL; }
    if( used.size() )
    {
	string mess;
	for( int i=0; i < used.size(); i++ )
	    mess+=used[i]->name()+", ";
	throw TError(nodePath().c_str(),Mess->I18N("Function used by: %s"),mess.c_str());
    }
}

int TFunction::ioSize()
{
    return m_io.size();
}

IO *TFunction::io( int iid )
{    
    if( iid >= m_io.size() ) throw TError(nodePath().c_str(),Mess->I18N("Index %d broken!"),iid);
    return m_io[iid];
}

int TFunction::ioId( const string &id )
{    
    for( int i_io = 0; i_io < m_io.size(); i_io++ )
	if( m_io[i_io]->id() == id ) return i_io;
    return -1;	
}

void TFunction::ioList( vector<string> &list )
{
    for( int i_io = 0; i_io < m_io.size(); i_io++ )
	list.push_back( m_io[i_io]->id() );
}

void TFunction::ioAdd( IO *io )
{
    preIOCfgChange();    
    m_io.push_back(io);
    io->owner = this;    
    postIOCfgChange();
}

void TFunction::ioIns( IO *io, int pos )
{
    if( pos < 0 || pos > m_io.size() )	
	pos = m_io.size();
	
    preIOCfgChange();	
    m_io.insert(m_io.begin()+pos,io);
    io->owner = this;    
    postIOCfgChange();
}

void TFunction::ioDel( int pos )
{
    if( pos < 0 || pos >= m_io.size() )
        throw TError(nodePath().c_str(),Mess->I18N("Delete IO <%d> error."),pos);
	
    preIOCfgChange();    	
    m_io.erase(m_io.begin()+pos);
    postIOCfgChange();
}

void TFunction::ioMove( int pos, int to )
{
    if( pos < 0 || pos >= m_io.size() || to < 0 || to >= m_io.size() )
	throw TError(nodePath().c_str(),Mess->I18N("Move IO from %d to %d error."),pos,to);
	
    preIOCfgChange();    	
    IO *io = m_io[to];
    m_io[to] = m_io[pos];
    m_io[pos] = io;  
    postIOCfgChange();  	
}    

void TFunction::preIOCfgChange()
{
    string blk_lst;
    for(unsigned i=0; i < used.size(); i++)
	if( used[i]->blk() )	blk_lst+=used[i]->name()+",";
    if( blk_lst.size() )
	throw TError(nodePath().c_str(),Mess->I18N("Change no permit by function used: %s"),blk_lst.c_str());
    
    for(unsigned i=0; i < used.size(); i++)
	used[i]->preIOCfgChange();
}

void TFunction::postIOCfgChange()
{
    for(unsigned i=0; i < used.size(); i++)
        used[i]->postIOCfgChange();
}

void TFunction::valAtt( TValFunc *vfnc )
{
    for(unsigned i=0; i < used.size() ;i++)
	if(used[i] == vfnc) 
	    throw TError(nodePath().c_str(),Mess->I18N("Value <%s> already attached!"),vfnc->name().c_str());
    used.push_back(vfnc);
}

void TFunction::valDet( TValFunc *vfnc )
{
    for(unsigned i=0; i < used.size() ;i++)
	if(used[i] == vfnc)
        {
            used.erase(used.begin()+i);
    	    break;
        }
}

void TFunction::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
	ctrMkNode("oscada_cntr",opt,-1,"/",Mess->I18N("Function: ")+name());
	ctrMkNode("area",opt,-1,"/func",Mess->I18N("Function"));
	ctrMkNode("area",opt,-1,"/func/st",Mess->I18N("State"));
        ctrMkNode("fld",opt,-1,"/func/st/st",Mess->I18N("Accessing"),0664,"root","root",1,"tp","bool");
        ctrMkNode("area",opt,-1,"/func/cfg",Mess->I18N("Config"));
	ctrMkNode("fld",opt,-1,"/func/cfg/id",Mess->I18N("Id"),0444,"root","root",1,"tp","str");
	ctrMkNode("fld",opt,-1,"/func/cfg/name",Mess->I18N("Name"),0444,"root","root",1,"tp","str");
	ctrMkNode("fld",opt,-1,"/func/cfg/descr",Mess->I18N("Description"),0444,"root","root",3,"tp","str","cols","70","rows","4");
	ctrMkNode("area",opt,-1,"/io",Mess->I18N("IO"));	
	ctrMkNode("table",opt,-1,"/io/io",Mess->I18N("IO"),0440,"root","root");
	ctrMkNode("list",opt,-1,"/io/io/0",Mess->I18N("Id"),0444,"root","root",1,"tp","str");
	ctrMkNode("list",opt,-1,"/io/io/1",Mess->I18N("Name"),0444,"root","root",1,"tp","str");
	ctrMkNode("list",opt,-1,"/io/io/2",Mess->I18N("Type"),0444,"root","root",1,"tp","str");
        ctrMkNode("list",opt,-1,"/io/io/3",Mess->I18N("Mode"),0444,"root","root",1,"tp","str");
	ctrMkNode("list",opt,-1,"/io/io/4",Mess->I18N("Hide"),0444,"root","root",1,"tp","bool");
	ctrMkNode("list",opt,-1,"/io/io/5",Mess->I18N("Default"),0444,"root","root",1,"tp","str");	
	ctrMkNode("area",opt,-1,"/test",Mess->I18N("Test"));
	ctrMkNode("fld",opt,-1,"/test/en",Mess->I18N("Enable"),0660,"root","root",1,"tp","bool");
	//Add test form
	if( m_tval )
	{
	    ctrMkNode("area",opt,-1,"/test/io",Mess->I18N("IO"));
    	    //Put io
    	    for( int i_io = 0; i_io < ioSize(); i_io++ )
    	    {
		if( m_io[i_io]->hide() ) continue;
	    
		char *tp = "";
		switch(io(i_io)->type())
		{
		    case IO::String:	tp = "str";	break;
		    case IO::Integer:	tp = "dec";	break;
		    case IO::Real:	tp = "real";	break;
		    case IO::Boolean:	tp = "bool";	break;
		}		
		ctrMkNode("fld",opt,-1,("/test/io/"+io(i_io)->id()).c_str(),io(i_io)->name(),0664,"root","root",1,"tp",tp);
	    }
	    //Add Calc button and Calc time
	    ctrMkNode("fld",opt,-1,"/test/n_clc",Mess->I18N("Number calcs"),0664,"root","root",1,"tp","dec");
	    ctrMkNode("fld",opt,-1,"/test/tm",Mess->I18N("Calc time (mks)"),0444,"root","root",1,"tp","real");
	    ctrMkNode("comm",opt,-1,"/test/calc",Mess->I18N("Calc"));
	}
	opt->attr("rez","0");
        return;
    }
    //Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/func/st/st" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->text(run_st?"1":"0");
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	start(atoi(opt->text().c_str()));
    }
    else if( a_path == "/func/cfg/id" && ctrChkNode(opt) )	opt->text(id());
    else if( a_path == "/func/cfg/name" && ctrChkNode(opt) )	opt->text(name());
    else if( a_path == "/func/cfg/descr" && ctrChkNode(opt) )	opt->text(descr());
    else if( a_path == "/io/io" && ctrChkNode(opt,"get",0440,"root","root",SEQ_RD) )
    {
	XMLNode *n_id	= ctrMkNode("list",opt,-1,"/io/io/0","");
	XMLNode *n_nm  	= ctrMkNode("list",opt,-1,"/io/io/1","");
	XMLNode *n_type	= ctrMkNode("list",opt,-1,"/io/io/2","");
        XMLNode *n_mode = ctrMkNode("list",opt,-1,"/io/io/3","");
	XMLNode *n_hide = ctrMkNode("list",opt,-1,"/io/io/4","");
	XMLNode *n_def 	= ctrMkNode("list",opt,-1,"/io/io/5","");
	//XMLNode *n_vect	= ctrId(opt,"6");
	for( int i_io = 0; i_io < ioSize(); i_io++ )
	{ 
	    string tmp_str;
	    if(n_id)	n_id->childAdd("el")->text(io(i_io)->id());
	    if(n_nm)	n_nm->childAdd("el")->text(io(i_io)->name());
	    //Make type
	    switch(io(i_io)->type())
	    {
		case IO::String:	tmp_str = Mess->I18N("String");	break;
		case IO::Integer:	tmp_str = Mess->I18N("Integer");	break;
		case IO::Real:		tmp_str = Mess->I18N("Real");	break;
		case IO::Boolean:	tmp_str = Mess->I18N("Bool");	break;
		case IO::Vector:	tmp_str = Mess->I18N("Vector");	break;
	    }
	    if(n_type)	n_type->childAdd("el")->text(tmp_str);
	    //Make mode
	    switch(io(i_io)->mode())
	    {
		case IO::Output:	tmp_str = Mess->I18N("Output");	break;
		case IO::Return:	tmp_str = Mess->I18N("Return");	break;
		case IO::Input:		tmp_str = Mess->I18N("Input");	break;
	    }
	    if(n_mode)	n_mode->childAdd("el")->text(tmp_str);
		
	    if(n_hide)	n_hide->childAdd("el")->text(io(i_io)->hide()?"1":"0");
	    if(n_def)	n_def->childAdd("el")->text(io(i_io)->def());
	}	
    }
    else if( a_path == "/test/en" )
    {
	if( ctrChkNode(opt,"get",0660,"root","root",SEQ_RD) )	opt->text(m_tval?"1":"0");
	if( ctrChkNode(opt,"set",0660,"root","root",SEQ_WR) )
	{
	    bool to_en_test = atoi(opt->text().c_str());
	    if( to_en_test && !m_tval )	{ m_tval = new TValFunc(id()+"_test",this); m_tval->dimens(true); }
	    if( !to_en_test && m_tval ) { delete m_tval; m_tval = NULL; }
	}
    }
    else if( a_path == "/test/n_clc" && m_tval )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->text(TBDS::genDBGet(nodePath()+"ntCalc","10",opt->attr("user")));
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	TBDS::genDBSet(nodePath()+"ntCalc",opt->text(),opt->attr("user"));
    }	
    else if( a_path == "/test/tm" && m_tval && ctrChkNode(opt) )opt->text(TSYS::real2str(m_tval->calcTm()));	
    else if( a_path.substr(0,8) == "/test/io" && m_tval )
    {
	string io_id = TSYS::pathLev(a_path,2);
        for( int i_io = 0; i_io < m_io.size(); i_io++ )
    	    if( io_id == io(i_io)->id() )
	    {
		if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->text(m_tval->getS(i_io));
		if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	m_tval->setS(i_io,opt->text());
		break;
	    }
    }
    else if( a_path == "/test/calc" && m_tval && ctrChkNode(opt,"set") )	
    { 
        double c_rez = 0;
	int n_tcalc = atoi(TBDS::genDBGet(nodePath()+"ntCalc","10",opt->attr("user")).c_str());
        for(int i_c = 0; i_c < n_tcalc; i_c++ )
	{
	    m_tval->calc();
	    c_rez += m_tval->calcTm();
	}
        m_tval->calcTm(c_rez);
    }
}		


//**** IO ****
IO::IO( const char *iid, const char *iname, IO::Type itype, IO::Mode imode, const char *idef, bool ihide, const char *ivect )
{
    m_id = iid;
    m_name = iname;
    m_type = itype;
    m_mode = imode;
    m_hide = ihide;
    m_def  = idef;
    m_vect = ivect;
}	
	
void IO::id( const string &val )
{ 
    owner->preIOCfgChange();
    m_id = val; 
    owner->postIOCfgChange();
}

void IO::name( const string &val ) 	
{ 
    //owner->preIOCfgChange();
    m_name = val; 
    //owner->postIOCfgChange();
}

void IO::type( Type val ) 	
{
    owner->preIOCfgChange();
    m_type = val;
    owner->postIOCfgChange();
}

void IO::mode( Mode val ) 	
{ 
    owner->preIOCfgChange();
    m_mode = val; 
    owner->postIOCfgChange();
}

void IO::def( const string &val )
{ 
    //owner->preIOCfgChange();
    m_def = val; 
    //owner->postIOCfgChange();
}

void IO::vector( const string &val )
{ 
    owner->preIOCfgChange();
    m_vect = val; 
    owner->postIOCfgChange();
}

void IO::hide( bool val )	
{ 
    //owner->preIOCfgChange();
    m_hide = val; 
    //owner->postIOCfgChange();
}

//===================================================
//========== TValFunc ===============================
//===================================================
TValFunc::TValFunc( const string &iname, TFunction *ifunc, bool iblk ) : 
    m_name(iname), m_func(NULL), m_dimens(false), tm_calc(0.0), m_blk(iblk)
{   
    func(ifunc);    
}

TValFunc::~TValFunc( )
{
    if( m_func ) funcDisConnect();
}

void TValFunc::func( TFunction *ifunc, bool att_det )
{
    if( m_func ) funcDisConnect(att_det);
    if( ifunc ) 
    {
	m_func = ifunc;
	if(att_det) m_func->valAtt(this);
	for( int i_vl = 0; i_vl < m_func->ioSize(); i_vl++ )
	{
	    SVl val;
	    val.tp = m_func->io(i_vl)->type();
	    if( val.tp == IO::String ) 		val.vl = new string(m_func->io(i_vl)->def());
	    else if( val.tp == IO::Integer )	val.vl = new int(atoi(m_func->io(i_vl)->def().c_str()));
	    else if( val.tp == IO::Real ) 	val.vl = new double(atof(m_func->io(i_vl)->def().c_str()));
	    else if( val.tp == IO::Boolean )	val.vl = new bool(atoi(m_func->io(i_vl)->def().c_str()));
	    m_val.push_back(val);
	}
    }
}

void TValFunc::funcDisConnect( bool det )
{
    if( m_func )
    {
	for( int i_vl = 0; i_vl < m_val.size(); i_vl++ )
	    if( m_val[i_vl].tp == IO::String )		delete (string *)m_val[i_vl].vl;
	    else if( m_val[i_vl].tp == IO::Integer )	delete (int *)m_val[i_vl].vl;
	    else if( m_val[i_vl].tp == IO::Real )	delete (double *)m_val[i_vl].vl;
	    else if( m_val[i_vl].tp == IO::Boolean )	delete (bool *)m_val[i_vl].vl;
	m_val.clear();    
	if(det)
	{ 
	    m_func->valDet(this);
	    m_func = NULL;
	}
    }
}

int TValFunc::ioId( const string &iid )
{
    if( !m_func )	throw TError("ValFnc",Mess->I18N("IO <%s> no present!"),iid.c_str());
    return m_func->ioId(iid);
}

void TValFunc::ioList( vector<string> &list )
{
    if( !m_func )       throw TError("ValFnc",Mess->I18N("Function no attached!"));
    return m_func->ioList(list);
}

int TValFunc::ioSize( )
{
    if( !m_func )       throw TError("ValFnc",Mess->I18N("Function no attached!"));
    return m_func->ioSize();
}

string TValFunc::getS( unsigned id )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	return *(string *)m_val[id].vl;
	case IO::Integer:	return TSYS::int2str(*(int *)m_val[id].vl);
	case IO::Real:		return TSYS::real2str(*(double *)m_val[id].vl);
	case IO::Boolean:	return TSYS::int2str(*(bool *)m_val[id].vl);
    }
    return "";
}	    
	
int TValFunc::getI( unsigned id )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	return atoi(((string *)m_val[id].vl)->c_str());
	case IO::Integer:	return *(int *)m_val[id].vl;
	case IO::Real:		return (int)(*(double *)m_val[id].vl);
	case IO::Boolean:	return *(bool *)m_val[id].vl;
    }
    return 0;
}	
	
double TValFunc::getR( unsigned id )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	return atof(((string *)m_val[id].vl)->c_str());
	case IO::Integer:	return *(int *)m_val[id].vl;
	case IO::Real:		return *(double *)m_val[id].vl;
	case IO::Boolean:	return *(bool *)m_val[id].vl;
    }
    return 0.0;
}
	
bool TValFunc::getB( unsigned id )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	return atoi(((string *)m_val[id].vl)->c_str());
	case IO::Integer:	return *(int *)m_val[id].vl;
	case IO::Real:		return *(double *)m_val[id].vl;
	case IO::Boolean:	return *(bool *)m_val[id].vl;
    }
    return false;
}
	
void TValFunc::setS( unsigned id, const string &val )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	*(string *)m_val[id].vl = val;	break;					
	case IO::Integer:	*(int *)m_val[id].vl = atoi(val.c_str());	break;
	case IO::Real:		*(double *)m_val[id].vl = atof(val.c_str());	break;
	case IO::Boolean:	*(bool *)m_val[id].vl = atoi(val.c_str());	break;
    }
}
	
void TValFunc::setI( unsigned id, int val )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	*(string *)m_val[id].vl = TSYS::int2str(val);	break;
	case IO::Integer:	*(int *)m_val[id].vl = val;	break;
	case IO::Real:		*(double *)m_val[id].vl = val;	break;
	case IO::Boolean:	*(bool *)m_val[id].vl = val;	break;
    }
}
	
void TValFunc::setR( unsigned id, double val )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    if( isnan(val) ) val = 0.;	//Check for 'Not a Number'
    switch(m_val[id].tp)
    {
	case IO::String:	*(string *)m_val[id].vl = TSYS::real2str(val);	break;
	case IO::Integer:	*(int *)m_val[id].vl = (int)val;break;
	case IO::Real:		*(double *)m_val[id].vl = val;	break;
	case IO::Boolean:	*(bool *)m_val[id].vl = val;	break;
    }
}	
	
void TValFunc::setB( unsigned id, bool val )
{
    if( id >= m_val.size() )    throw TError("ValFnc",Mess->I18N("Id or IO %d error!"),id);
    switch(m_val[id].tp)
    {
	case IO::String:	*(string *)m_val[id].vl = TSYS::int2str(val);	break;
	case IO::Integer:	*(int *)m_val[id].vl = val;	break;
	case IO::Real:		*(double *)m_val[id].vl = val;	break;
	case IO::Boolean:	*(bool *)m_val[id].vl = val;	break;
    }
}

void TValFunc::calc( )
{ 
    if( !m_func || !m_func->startStat() ) return;    
    if( !m_dimens ) m_func->calc(this);
    else
    {
	unsigned long long t_cnt = SYS->shrtCnt();
	m_func->calc(this); 
	tm_calc = 1.0e6*((double)(SYS->shrtCnt()-t_cnt))/((double)SYS->sysClk());
    }
}

void TValFunc::preIOCfgChange()
{    
    func( NULL, false );
}

void TValFunc::postIOCfgChange()
{
    func( m_func, false );
}
