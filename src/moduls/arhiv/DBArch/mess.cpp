
//OpenSCADA system module Archive.DBArch file: mess.cpp
/***************************************************************************
 *   Copyright (C) 2007 by Roman Savochenko                                *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
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

#include <sys/time.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <tsys.h>
#include "arch.h"
#include "mess.h"


using namespace DBArch;

//************************************************
//* DBArch::ModMArch - Messages archivator       *
//************************************************
ModMArch::ModMArch( const string &iid, const string &idb, TElem *cf_el ) : 
    TMArchivator(iid,idb,cf_el), m_beg(0), m_end(0), tm_calc(0.0), 
    m_max_size(cfg("DBArchSize").getRd())
{
    setAddr("*.*");
}

ModMArch::~ModMArch( )
{
    try{ stop(); }catch(...){}
}

void ModMArch::postDisable( int flag )
{
    TMArchivator::postDisable( flag );
    try
    {
	if( flag )
	{
	    //- Remove info record -
    	    TConfig cfg(&mod->archEl());
	    cfg.cfg("TBL").setS(archTbl());
	    SYS->db().at().dataDel(addr()+"."+mod->mainTbl(),"",cfg);
		
	    //- Remove archive's DB table -
    	    SYS->db().at().open( addr()+"."+archTbl() );
	    SYS->db().at().close( addr()+"."+archTbl(), true );
	}
    }
    catch(TError err)	{ mess_warning(err.cat.c_str(),"%s",err.mess.c_str()); }
}

void ModMArch::load( )
{
    TMArchivator::load();

    //- Init address to DB -
    if( addr().empty() ) setAddr("*.*");
    
    //- Load message archive parameters -
    TConfig cfg(&mod->archEl());
    cfg.cfg("TBL").setS(archTbl());
    if(SYS->db().at().dataGet(addr()+"."+mod->mainTbl(),"",cfg))
    {
	m_beg = atoi(cfg.cfg("BEGIN").getS().c_str());
	m_end = atoi(cfg.cfg("END").getS().c_str());
    }
}

void ModMArch::start( )
{    
    run_st = true;
}

void ModMArch::stop( )
{
    run_st = false;
}

time_t ModMArch::begin( )
{
    return m_beg;
}

time_t ModMArch::end()
{
    return m_end;
}

void ModMArch::put( vector<TMess::SRec> &mess )
{
    if(!run_st) throw TError(nodePath().c_str(),_("Archive no started!"));
    
    TConfig cfg(&mod->messEl());
    unsigned long long t_cnt = SYS->shrtCnt();
    for( unsigned i_m = 0; i_m < mess.size(); i_m++)
    {
	if( !chkMessOK(mess[i_m].categ,mess[i_m].level) ) continue;	
	
	//- Put record to DB -
	cfg.cfg("TM").setI(mess[i_m].time);
	cfg.cfg("CATEG").setS(mess[i_m].categ);
	cfg.cfg("MESS").setS(mess[i_m].mess);
	cfg.cfg("LEV").setI(mess[i_m].level);
	SYS->db().at().dataSet(addr()+"."+archTbl(),"",cfg);
	//- Archive time border update -
	m_beg=m_beg?vmin(m_beg,mess[i_m].time):mess[i_m].time;
	m_end=m_end?vmax(m_end,mess[i_m].time):mess[i_m].time;
    }
    
    //- Archive size limit process -
    if( (m_end-m_beg) > (time_t)(maxSize()*3600.) )
    {
	time_t n_end = m_end-(time_t)(maxSize()*3600.);
	for( time_t t_c = m_beg; t_c < n_end; t_c++ )
	{
	    cfg.cfg("TM").setI(t_c);
	    SYS->db().at().dataDel(addr()+"."+archTbl(),"",cfg);
	}
	m_beg=n_end;
    }
    
    //- Update archive info -
    cfg.setElem(&mod->archEl());
    cfg.cfgViewAll(false);
    cfg.cfg("TBL").setS(archTbl(),true);
    cfg.cfg("BEGIN").setS(TSYS::int2str(m_beg),true);
    cfg.cfg("END").setS(TSYS::int2str(m_end),true);
    SYS->db().at().dataSet(addr()+"."+mod->mainTbl(),"",cfg);
    
    tm_calc = 1.0e3*((double)(SYS->shrtCnt()-t_cnt))/((double)SYS->sysClk());
}

void ModMArch::get( time_t b_tm, time_t e_tm, vector<TMess::SRec> &mess, const string &category, char level )
{
    if(!run_st) throw TError(nodePath().c_str(),_("Archive no started!"));

    b_tm = vmax(b_tm,begin());
    e_tm = vmin(e_tm,end());
    if( e_tm <= b_tm ) return;
    
    TConfig cfg(&mod->messEl());
    //- Get values from DB -    
    for( time_t t_c = b_tm; t_c <= e_tm; t_c++ )
    {
	cfg.cfg("TM").setI(t_c);
	for( int e_c = 0; SYS->db().at().dataSeek(addr()+"."+archTbl(),"",e_c++,cfg); )
	{
	    TMess::SRec rc(t_c,cfg.cfg("CATEG").getS(),(TMess::Type)cfg.cfg("LEV").getI(),cfg.cfg("MESS").getS());
	    if( rc.level >= level && TMess::chkPattern(rc.categ,category) )
	    {
		bool equal = false;
	        int i_p = mess.size();
	        for( int i_m = mess.size()-1; i_m >= 0; i_m-- )
	        {
	            if( mess[i_m].time > rc.time )   i_p = i_m;
	            else if( rc.time == mess[i_m].time && rc.level == mess[i_m].level && rc.mess == mess[i_m].mess )
                    { equal = true; break; }
                    else if( mess[i_m].time < rc.time ) break;
        	}
                if( !equal )
		{ 
		    mess.insert(mess.begin()+i_p,rc);    
		    if( mess.size() >= TArchiveS::max_req_mess ) return;
		}
	    }
	    cfg.cfg("CATEG").setS(""); cfg.cfg("MESS").setS("");
	}
    }
}

void ModMArch::cntrCmdProc( XMLNode *opt )
{
    string grp = owner().owner().subId();    
    //- Get page info -
    if( opt->name() == "info" )
    {
        TMArchivator::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/prm/st/tarch",_("Archiving time (msek)"),0444,"root",grp.c_str(),1,"tp","real");
	if( ctrMkNode("area",opt,1,"/bs",_("Additional options"),0444,"root",grp.c_str()) )
	    ctrMkNode("fld",opt,-1,"/bs/sz",cfg("DBArchSize").fld().descr(),0664,"root",grp.c_str(),1,"tp","real");
        return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/prm/st/tarch" && ctrChkNode(opt) ) 	opt->setText(TSYS::real2str(tm_calc,6));
    else if( a_path == "/bs/sz" )
    {
	if( ctrChkNode(opt,"get",0664,"root",grp.c_str(),SEQ_RD) ) opt->setText(TSYS::real2str(m_max_size));
	if( ctrChkNode(opt,"set",0664,"root",grp.c_str(),SEQ_WR) ) m_max_size = atof(opt->text().c_str());
    }
    else TMArchivator::cntrCmdProc(opt);
}