
//OpenSCADA system module DAQ.System file: da_smart.cpp
/***************************************************************************
 *   Copyright (C) 2005-2008 by Roman Savochenko                           *
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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <tsys.h>

#include "os_contr.h"
#include "da_smart.h"

using namespace SystemCntr;

const char *HddSmart::smartval_cmd = "smartctl -A -v N,raw48 /dev/";

//*************************************************
//* HddSmart                                      *
//*************************************************
HddSmart::HddSmart( )
{

}

HddSmart::~HddSmart( )
{

}

void HddSmart::init( TMdPrm *prm )
{
    TCfg &c_subt = prm->cfg("SUBT");

    //Create Configuration
    c_subt.fld().setDescr(_("Disk"));

    vector<string> list;
    dList(list);
    string dls;
    for(unsigned i_l = 0; i_l < list.size(); i_l++)
	dls = dls+list[i_l]+";";
    c_subt.fld().setValues(dls);
    c_subt.fld().setSelNames(dls);

    try{ c_subt.getSEL(); }
    catch(...)
    {
	if( list.size() ) c_subt.setS(list[0]);
    }
}

void HddSmart::dList( vector<string> &list, bool part )
{
    int major, minor;
    char name[11];
    char buf[256];

    FILE *f = fopen("/proc/partitions","r");
    if( f == NULL ) return;

    while( fgets(buf,sizeof(buf),f) != NULL )
    {
	if( sscanf(buf,"%d %d %*d %10s",&major,&minor,name) != 3 ) continue;
	//if( strncmp(name,"hd",2) )	continue;
	if( !part && minor != 0 )	continue;

	string cmd = string(smartval_cmd)+name+((name[0]=='s')?" -d ata":"");
	FILE *fp = popen(cmd.c_str(),"r");
	if( fp )
	{
	    int val;
	    bool access_true = false;
	    while( fgets(buf,sizeof(buf),fp) != NULL )
	    {
		if( sscanf(buf,"%*d %*s %*x %*d %*d %*d %*s %*s %*s %d\n",&val) != 1 ) continue;
		access_true = true;
		break;
	    }
	    pclose(fp);
	    if( access_true )	list.push_back(name);
	}
    }
    fclose(f);
}

void HddSmart::getVal( TMdPrm *prm )
{
    int id;
    unsigned long val;
    char buf[256];
    char name[31];

    string dev = prm->cfg("SUBT").getS();

    //> SMART attributes
    string cmd = string(smartval_cmd)+dev+((dev.size()&&dev[0]=='s')?" -d ata":"");
    FILE *fp = popen(cmd.c_str(),"r");
    if( fp )
    {
	while( fgets(buf,sizeof(buf),fp) != NULL )
	{
	    if( sscanf(buf,"%d %30s %*x %*d %*d %*d %*s %*s %*s %lu\n",&id,name,&val) != 3 ) continue;
	    string s_id = TSYS::int2str(id);
	    if( !prm->vlPresent(s_id) )	fldAdd( new TFld(s_id.c_str(),name,TFld::Integer,TFld::NoWrite) );
	    prm->vlAt(s_id).at().setI(val,0,true);
	}
	fclose(fp);
    }
}

void HddSmart::makeActiveDA( TMdContr *a_cntr )
{
    string ap_nm = "Smart_";

    vector<string> list;
    dList(list);
    for(unsigned i_hd = 0; i_hd < list.size(); i_hd++)
    {
	string hddprm = ap_nm+list[i_hd];
	if(!a_cntr->present(hddprm))
	{
	    a_cntr->add(hddprm,0);
	    a_cntr->at(hddprm).at().setName(_("HD smart: ")+list[i_hd]);
	    a_cntr->at(hddprm).at().autoC(true);
	    a_cntr->at(hddprm).at().cfg("TYPE").setS(id());
	    a_cntr->at(hddprm).at().cfg("SUBT").setS(list[i_hd]);
	    a_cntr->at(hddprm).at().cfg("EN").setB(true);
	}
    }
}
