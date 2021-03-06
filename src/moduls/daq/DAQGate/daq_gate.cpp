
//OpenSCADA system module DAQ.DAQGate file: daq_gate.cpp
/***************************************************************************
 *   Copyright (C) 2007-2013 by Roman Savochenko                           *
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

#include <signal.h>

#include <tsys.h>
#include <ttiparam.h>

#include "daq_gate.h"

//******************************************************
//* Modul info!                                        *
#define MOD_ID		"DAQGate"
#define MOD_NAME	_("Data sources gate")
#define MOD_TYPE	SDAQ_ID
#define VER_TYPE	SDAQ_VER
#define MOD_VER		"0.9.5"
#define AUTHORS		_("Roman Savochenko")
#define DESCRIPTION	_("Allow to make gate data sources of remote OpenSCADA station to local OpenSCADA station.")
#define LICENSE		"GPL2"
//******************************************************

DAQGate::TTpContr *DAQGate::mod;  //Pointer for direct access to main module object

extern "C"
{
#ifdef MOD_INCL
    TModule::SAt daq_DAQGate_module( int n_mod )
#else
    TModule::SAt module( int n_mod )
#endif
    {
	if(n_mod == 0) return TModule::SAt(MOD_ID, MOD_TYPE, VER_TYPE);
	return TModule::SAt("");
    }

#ifdef MOD_INCL
    TModule *daq_DAQGate_attach( const TModule::SAt &AtMod, const string &source )
#else
    TModule *attach( const TModule::SAt &AtMod, const string &source )
#endif
    {
	if(AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE)) return new DAQGate::TTpContr(source);
	return NULL;
    }
}

using namespace DAQGate;

//******************************************************
//* TTpContr                                           *
//******************************************************
TTpContr::TTpContr( string name ) : TTipDAQ(MOD_ID)
{
    mod		= this;

    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VER;
    mAuthor	= AUTHORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;
}

TTpContr::~TTpContr( )
{

}

void TTpContr::load_( )
{
    //> Load parameters from command line

}

void TTpContr::postEnable( int flag )
{
    TTipDAQ::postEnable(flag);

    //> Controler's DB structure
    fldAdd(new TFld("PRM_BD",_("Parameters cache table"),TFld::String,TFld::NoFlag,"30",""));
    fldAdd(new TFld("PERIOD",_("Gather data period (s)"),TFld::Integer,TFld::NoFlag,"6","0","0;100"));	//!!!! Remove at further
    fldAdd(new TFld("SCHEDULE",_("Acquisition schedule"),TFld::String,TFld::NoFlag,"100","1"));
    fldAdd(new TFld("PRIOR",_("Gather task priority"),TFld::Integer,TFld::NoFlag,"2","0","-1;99"));
    fldAdd(new TFld("TM_REST",_("Restore timeout (s)"),TFld::Integer,TFld::NoFlag,"3","30","1;1000"));
    fldAdd(new TFld("TM_REST_DT",_("Restore data depth time (hour)"),TFld::Real,TFld::NoFlag,"6.2","1","0;12"));
    fldAdd(new TFld("GATH_MESS_LEV",_("Gather messages level"),TFld::Integer,TFld::Selected,"1","1",
	"0;1;2;3;4;5;6;7",_("Debug (0);Information (1);Notice (2);Warning (3);Error (4);Critical (5);Alert (6);Emergency (7)")));
    fldAdd(new TFld("SYNCPER",_("Sync inter remote station period (s)"),TFld::Real,TFld::NoFlag,"6.2","0","0;1000"));
    fldAdd(new TFld("STATIONS",_("Remote stations list"),TFld::String,TFld::FullText,"100"));
    fldAdd(new TFld("CNTRPRM",_("Remote cotrollers and parameters list"),TFld::String,TFld::FullText,"200"));

    //> Parameter type bd structure
    int t_prm = tpParmAdd("std","PRM_BD",_("Standard"));
    tpPrmAt(t_prm).fldAdd(new TFld("ATTRS",_("Attributes configuration cache"),TFld::String,TFld::FullText|TCfg::NoVal,"100000",""));
    //> Set to read only
    for(unsigned i_sz = 0; i_sz < tpPrmAt(t_prm).fldSize(); i_sz++)
	tpPrmAt(t_prm).fldAt(i_sz).setFlg(tpPrmAt(t_prm).fldAt(i_sz).flg()|TFld::NoWrite);
}

TController *TTpContr::ContrAttach( const string &name, const string &daq_db )
{
    return new TMdContr(name, daq_db, this);
}

//******************************************************
//* TMdContr                                           *
//******************************************************
TMdContr::TMdContr( string name_c, const string &daq_db, ::TElem *cfgelem) :
    ::TController(name_c,daq_db,cfgelem),
    mSched(cfg("SCHEDULE")), mMessLev(cfg("GATH_MESS_LEV")), mSync(cfg("SYNCPER").getRd()), mRestDtTm(cfg("TM_REST_DT").getRd()), mPerOld(cfg("PERIOD").getId()),
    mRestTm(cfg("TM_REST").getId()), mPrior(cfg("PRIOR").getId()),
    prcSt(false), call_st(false), endrunReq(false), mPer(1e9), tmGath(0)
{
    cfg("PRM_BD").setS(MOD_ID"Prm_"+name_c);
}

TMdContr::~TMdContr( )
{
    if(run_st) stop();
}

string TMdContr::getStatus( )
{
    string val = TController::getStatus();

    if(startStat() && !redntUse())
    {
	if(call_st)	val += TSYS::strMess(_("Call now. "));
	if(period())	val += TSYS::strMess(_("Call by period: %s. "),TSYS::time2str(1e-3*period()).c_str());
        else val += TSYS::strMess(_("Call next by cron '%s'. "),TSYS::time2str(TSYS::cron(cron()),"%d-%m-%Y %R").c_str());
	val += TSYS::strMess(_("Spent time: %s. "),TSYS::time2str(tmGath).c_str());
	bool isWork = false;
	for(unsigned i_st = 0; i_st < mStatWork.size(); i_st++)
	    if(mStatWork[i_st].second.cntr > -1)
		val += TSYS::strMess(_("Station '%s' error, restoring in %.3g s."),mStatWork[i_st].first.c_str(),mStatWork[i_st].second.cntr);
	    else
	    {
		val += TSYS::strMess(_("Requests to station '%s': %.6g."),mStatWork[i_st].first.c_str(),-mStatWork[i_st].second.cntr);
		isWork = true;
	    }
	if(!isWork) val.replace(0,1,"10");
    }

    return val;
}

TParamContr *TMdContr::ParamAttach( const string &name, int type )
{
    return new TMdPrm(name,&owner().tpPrmAt(type));
}

void TMdContr::load_( )
{
    TController::load_( );

    //> Check for get old period method value
    if(mPerOld) { cfg("SCHEDULE").setS(TSYS::int2str(mPerOld)); mPerOld = 0; }
}

void TMdContr::enable_( )
{
    string statv, cp_el, daqtp, cntrnm, prmnm, cntrpath, gPrmLs;
    vector<string> prm_ls;
    XMLNode req("list");

    //> Clear present parameters configuration
    list(prm_ls);
    for(unsigned i_p = 0; i_p < prm_ls.size(); i_p++) at(prm_ls[i_p]).at().setCntrAdr("");

    //> Station list update
    if(!mStatWork.size())
	for(int st_off = 0; (statv=TSYS::strSepParse(cfg("STATIONS").getS(),0,'\n',&st_off)).size(); )
	    mStatWork.push_back(pair<string,StHd>(statv,StHd()));

    //> Remote station scaning. Controllers and parameters scaning
    for(int i_st = 0; i_st < mStatWork.size(); i_st++)
	for(int cp_off = 0; (cp_el=TSYS::strSepParse(cfg("CNTRPRM").getS(),0,'\n',&cp_off)).size(); )
	    try
	    {
		//>> Parse parameter template
		daqtp  = TSYS::strSepParse(cp_el,0,'.');
		cntrnm = TSYS::strSepParse(cp_el,1,'.');
		prmnm  = TSYS::strSepParse(cp_el,2,'.');
		if(daqtp.empty() || cntrnm.empty()) continue;
		cntrpath = "/"+mStatWork[i_st].first+"/DAQ/"+daqtp+"/"+cntrnm+"/";
		//>> Get parameters list
		prm_ls.clear();
		if(prmnm.empty() || prmnm == "*")
		{
		    req.clear()->setName("get")->setAttr("path",cntrpath+"%2fprm%2fprm");
		    if(cntrIfCmd(req)) throw TError(req.attr("mcat").c_str(),"%s",req.text().c_str());
		    else for(unsigned i_ch = 0; i_ch < req.childSize(); i_ch++)
			prm_ls.push_back(req.childGet(i_ch)->attr("id"));
		}
		else prm_ls.push_back(prmnm);

		//>> Process remote parameters
		for(unsigned i_p = 0; i_p < prm_ls.size(); i_p++)
		{
		    if(!present(prm_ls[i_p]))
		    {
			//>>> Parameter name request and make new parameter object
			req.clear()->setName("get")->setAttr("path",cntrpath+prm_ls[i_p]+"/%2fprm%2fcfg%2fNAME");
			if(cntrIfCmd(req)) throw TError(req.attr("mcat").c_str(),"%s",req.text().c_str());
			add(prm_ls[i_p],owner().tpPrmToId("std"));
			at(prm_ls[i_p]).at().setName(req.text());
		    }
		    if(!at(prm_ls[i_p]).at().enableStat())
		    {
			at(prm_ls[i_p]).at().enable();
			if(enableStat()) at(prm_ls[i_p]).at().load();
		    }
		    at(prm_ls[i_p]).at().setCntrAdr(cntrpath);
		    gPrmLs += prm_ls[i_p]+";";
		}
	    }catch(TError err){ mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }

    //> Removing remotely missed parameters in case all remote stations active status by actual list
    bool prmChkToDel = true;
    for(int i_st = 0; prmChkToDel && i_st < mStatWork.size(); i_st++)
	if(mStatWork[i_st].second.cntr >= 0) prmChkToDel = false;
    if(prmChkToDel && enableStat())
    {
	list(prm_ls);
	for(unsigned i_p = 0; i_p < prm_ls.size(); i_p++)
	    if(gPrmLs.find(prm_ls[i_p]+";",0) == string::npos)
		try{ del(prm_ls[i_p],true); }
		catch(TError err)
		{
		    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
		    mess_err(nodePath().c_str(),_("Deletion parameter '%s' is error but it no present on configuration or remote station."),prm_ls[i_p].c_str());
		}
    }
}

void TMdContr::disable_( )
{
    mStatWork.clear();
}

void TMdContr::start_( )
{
    if(prcSt) return;

    mStatWork.clear();
    enable_();

    //> Schedule process
    mPer = TSYS::strSepParse(cron(),1,' ').empty() ? vmax(0,(int64_t)(1e9*atof(cron().c_str()))) : 0;

    //> Clear stations request counter
    for(unsigned i_st = 0; i_st < mStatWork.size(); i_st++) mStatWork[i_st].second.cntr = -1;

    //> Start the gathering data task
    SYS->taskCreate(nodePath('.',true), mPrior, TMdContr::Task, this);
}

void TMdContr::stop_( )
{
    if(!prcSt) return;

    //> Stop the request and calc data task
    SYS->taskDestroy(nodePath('.',true), &endrunReq);

    //> Connection alarm clear
    for(unsigned i_st = 0; i_st < mStatWork.size(); i_st++)
    {
	if(mStatWork[i_st].second.cntr < 0)	continue;
	alarmSet(TSYS::strMess(_("DAQ.%s: connect to data source: %s."),id().c_str(),_("STOP")),TMess::Info);
	break;
    }
}

bool TMdContr::cfgChange( TCfg &icfg )
{
    TController::cfgChange(icfg);

    if(icfg.fld().name() == "SCHEDULE" && startStat())
	mPer = TSYS::strSepParse(cron(),1,' ').empty() ? vmax(0,(int64_t)(1e9*atof(cron().c_str()))) : 0;

    return true;
}

void *TMdContr::Task( void *icntr )
{
    map<string,float>::iterator sti;
    TMdContr &cntr = *(TMdContr *)icntr;
    int64_t t_cnt, t_prev = TSYS::curTime();
    double syncCnt = 0;
    unsigned int div = 0;

    cntr.endrunReq = false;
    cntr.prcSt = true;
    bool isFirst = true;

    for(unsigned int it_cnt = 0; !cntr.endrunReq; it_cnt++)
    {
	if(cntr.redntUse()) { TSYS::sysSleep(STD_WAIT_DELAY*1e-3); continue; }

	cntr.call_st = true;
	t_cnt = TSYS::curTime();

	try
	{
	    ResAlloc res(cntr.enRes, false);

	    //> Allow stations presenting
	    bool isAccess = false, needEnable = false;
	    for(unsigned i_st = 0; i_st < cntr.mStatWork.size(); i_st++)
	    {
		if(isFirst)	cntr.mStatWork[i_st].second.cntr = 0;	//Reset counter for connection alarm state update
		if(cntr.mStatWork[i_st].second.cntr > 0)
		    cntr.mStatWork[i_st].second.cntr = vmax(0,cntr.mStatWork[i_st].second.cntr-1e-6*(t_cnt-t_prev));
		if(cntr.mStatWork[i_st].second.cntr <= 0)	isAccess = true;
		if(cntr.mStatWork[i_st].second.cntr == 0)	needEnable = true;	//!!!! May be only for all == 0 stations
	    }
	    if(!isAccess) { t_prev = t_cnt; TSYS::sysSleep(1); continue; }
	    else
	    {
		if(cntr.syncPer() > 0.1)	//Enable sync
		{
		    div = cntr.period() ? vmax(2,(unsigned int)(cntr.syncPer()/(1e-9*cntr.period()))) : 0;
		    if(syncCnt <= 0) syncCnt = cntr.syncPer();
		    syncCnt = vmax(0, syncCnt-1e-6*(t_cnt-t_prev));
		}
		else { div = 0; syncCnt = 1; }	//Disable sync
		vector<string> pLS;
		cntr.list(pLS);
		AutoHD<TMdPrm> prm;
		string scntr;

		//> Parameters list update
		if(isFirst || needEnable || (!div && syncCnt <= 0) || (div && it_cnt > div && (it_cnt%div) == 0))
		    try { res.release(); cntr.enable_(); res.request(false); }
		    catch(TError err) { }

		//> Mark no process
		for(unsigned i_p = 0; i_p < pLS.size(); i_p++)
		{
		    AutoHD<TMdPrm> pO = cntr.at(pLS[i_p]);
		    if(!pO.at().isSynced || (!div && syncCnt <= 0) || (div && it_cnt > div && (((it_cnt+i_p)%div) == 0)))
			pO.at().sync();
		    pO.at().isPrcOK = false;
		}

		//> Station's cycle
		for(unsigned i_st = 0; i_st < cntr.mStatWork.size(); i_st++)
		{
		    if(cntr.mStatWork[i_st].second.cntr > 0) continue;
		    XMLNode req("CntrReqs"); req.setAttr("path", "/"+cntr.mStatWork[i_st].first+"/DAQ/");
		    map<string, bool> cntrLstMA;

		    //> Put attributes requests
		    for(unsigned i_p = 0; i_p < pLS.size(); i_p++)
		    {
			prm = cntr.at(pLS[i_p]);
			if(prm.at().isPrcOK) continue;
			for(int c_off = 0; (scntr=TSYS::strSepParse(prm.at().cntrAdr(),0,';',&c_off)).size(); )
			{
			    if(TSYS::pathLev(scntr,0) != cntr.mStatWork[i_st].first) continue;
			    string aMod	= TSYS::pathLev(scntr, 2);
			    string aCntr = TSYS::pathLev(scntr, 3);
			    cntrLstMA[aMod+"/"+aCntr] = true;

			    XMLNode *prmNd = req.childAdd("get")->setAttr("path","/"+aMod+"/"+aCntr+"/"+prm.at().id()+"/%2fserv%2fattr");
			    prmNd->setAttr("hostTm", !cntr.restDtTm() ? "1" : "0");

			    //>> Prepare individual attributes list
			    bool sepReq = !prm.at().isEVAL && ((!div && syncCnt > 0) || (div && ((it_cnt+i_p)%div)));
			    prmNd->setAttr("sepReq", sepReq ? "1" : "0");
			    if(!cntr.restDtTm() && !sepReq) continue;

			    vector<string> listV;
			    prm.at().vlList(listV);
			    unsigned rC = 0;
			    for(unsigned iV = 0; iV < listV.size(); iV++)
			    {
				AutoHD<TVal> vl = prm.at().vlAt(listV[iV]);
				if(sepReq && (!vl.at().arch().freeStat() || vl.at().reqFlg()))
				{
				    prmNd->childAdd("el")->setAttr("id",listV[iV]);
				    rC++;
				}
				if(!vl.at().arch().freeStat())
				    prmNd->childAdd("ael")->setAttr("id",listV[iV])->
					setAttr("tm",TSYS::ll2str(vmax(vl.at().arch().at().end(""),TSYS::curTime()-(int64_t)(3.6e9*cntr.restDtTm()))));
			    }
			    if(sepReq && rC > listV.size()/2)
			    {
				prmNd->childClear("el");
				prmNd->setAttr("sepReq", "0");
			    }
			}
		    }
		    //> Requests to the controllers messages prepare
		    for(map<string,bool>::iterator i_c = cntrLstMA.begin(); i_c != cntrLstMA.end(); ++i_c)
		    {
			int tm_grnd = cntr.mStatWork[i_st].second.lstMess[i_c->first];
			if(!tm_grnd) tm_grnd = SYS->sysTm() - 3600*cntr.restDtTm();
			req.childAdd("get")->setAttr("path", "/"+i_c->first+"/%2fserv%2fmess")->
					     setAttr("tm_grnd", TSYS::int2str(tm_grnd))->
					     setAttr("lev", cntr.mMessLev.getS());
		    }

		    if(!req.childSize()) continue;

		    //> Same request
		    if(cntr.cntrIfCmd(req)) { mess_err(req.attr("mcat").c_str(),"%s",req.text().c_str()); continue; }

		    //> Result process
		    for(unsigned i_r = 0; i_r < req.childSize(); ++i_r)
		    {
			XMLNode *prmNd = req.childGet(i_r);
			if(atoi(prmNd->attr("err").c_str())) continue;
			string aMod	= TSYS::pathLev(prmNd->attr("path"), 0);
                        string aCntr	= TSYS::pathLev(prmNd->attr("path"), 1);
			string pId	= TSYS::pathLev(prmNd->attr("path"), 2);
			if(pId == "/serv/mess")
			{
			    for(unsigned i_m = 0; i_m < prmNd->childSize(); i_m++)
			    {
				XMLNode *m = prmNd->childGet(i_m);
				SYS->archive().at().messPut(atoi(m->attr("time").c_str()), atoi(m->attr("utime").c_str()),
				    cntr.mStatWork[i_st].first+":"+m->attr("cat"), atoi(m->attr("lev").c_str()), m->text());
			    }
			    cntr.mStatWork[i_st].second.lstMess[aMod+"/"+aCntr] = atoi(prmNd->attr("tm").c_str());
			}
			else
			{
			    prm = cntr.at(pId);
			    if(prm.at().isPrcOK) continue;
			    prm.at().isPrcOK = true;
			    prm.at().isEVAL = false;

			    for(unsigned i_a = 0; i_a < prmNd->childSize(); i_a++)
			    {
				XMLNode *aNd = prmNd->childGet(i_a);
				if(!prm.at().vlPresent(aNd->attr("id"))) continue;
				AutoHD<TVal> vl = prm.at().vlAt(aNd->attr("id"));

				if(aNd->name() == "el")
				{ vl.at().setS(aNd->text(),cntr.restDtTm()?atoll(aNd->attr("tm").c_str()):0,true); vl.at().setReqFlg(false); }
				else if(aNd->name() == "ael" && !vl.at().arch().freeStat() && aNd->childSize())
				{
				    int64_t btm = atoll(aNd->attr("tm").c_str());
				    int64_t per = atoll(aNd->attr("per").c_str());
				    TValBuf buf(vl.at().arch().at().valType(),0,per,false,true);
				    for(unsigned i_v = 0; i_v < aNd->childSize(); i_v++)
					buf.setS(aNd->childGet(i_v)->text(),btm+per*i_v);
				    vl.at().arch().at().setVals(buf,buf.begin(),buf.end(),"");
				}
			    }
			}
		    }
		}

		//> Mark no processed parameters to EVAL
		for(unsigned i_p = 0; i_p < pLS.size(); i_p++)
		{
		    prm = cntr.at(pLS[i_p]);
		    if(prm.at().isPrcOK || prm.at().isEVAL) continue;
		    vector<string> vLs;
		    prm.at().elem().fldList(vLs);
		    for(unsigned i_v = 0; i_v < vLs.size(); i_v++)
		    {
			if(vLs[i_v] == "SHIFR" || vLs[i_v] == "NAME" || vLs[i_v] == "DESCR") continue;
			AutoHD<TVal> vl = prm.at().vlAt(vLs[i_v]);
			if(vl.at().getS() == EVAL_STR) continue;
			vl.at().setS(EVAL_STR, vl.at().arch().freeStat() ? 0 :
			    vmin(TSYS::curTime(),vmax(vl.at().arch().at().end(""),TSYS::curTime()-(int64_t)(3.6e9*cntr.restDtTm()))), true);
		    }
		    prm.at().vlAt("err").at().setS(_("10:Data not allow."),0,true);
		    prm.at().isEVAL = true;
		}
	    }
	    isFirst = false;
	    //res.release( );
	}catch(TError err)	{ mess_err(err.cat.c_str(),err.mess.c_str()); }

	//> Calc acquisition process time
	t_prev = t_cnt;
	cntr.tmGath = TSYS::curTime()-t_cnt;
	cntr.call_st = false;

	TSYS::taskSleep(cntr.period(), (cntr.period()?0:TSYS::cron(cntr.cron())));
    }

    cntr.prcSt = false;

    return NULL;
}

string TMdContr::catsPat( )
{
    string curPat = TController::catsPat();

    string statv;
    for(int st_off = 0; (statv=TSYS::strSepParse(cfg("STATIONS").getS(),0,'\n',&st_off)).size(); )
	curPat += "|^"+statv+":";

    return curPat;
}

int TMdContr::cntrIfCmd( XMLNode &node )
{
    string reqStat = TSYS::pathLev(node.attr("path"),0);

    for(unsigned i_st = 0; i_st < mStatWork.size(); i_st++)
	if(mStatWork[i_st].first == reqStat)
	{
	    if(mStatWork[i_st].second.cntr > 0) break;
	    try
	    {
		int rez = SYS->transport().at().cntrIfCmd(node,MOD_ID+id());
		//> Clear alarm for gone successful connect
		if(startStat() && mStatWork[i_st].second.cntr == 0)
		{
		    unsigned i_st1;
		    for(i_st1 = 0; i_st1 < mStatWork.size(); i_st1++)
			if(mStatWork[i_st1].second.cntr > 0) break;
		    if(i_st1 >= mStatWork.size())
			alarmSet(TSYS::strMess(_("DAQ.%s: connect to data source: %s."), id().c_str(), _("OK")), TMess::Info);
		}
		mStatWork[i_st].second.cntr -= 1;
		return rez;
	    }
	    catch(TError err)
	    {
		if(call_st)
		{
		    //> Set alarm for station
		    if(mStatWork[i_st].second.cntr < 0)
			alarmSet(TSYS::strMess(_("DAQ.%s: connect to data source '%s': %s."),
			    id().c_str(), mStatWork[i_st].first.c_str(), TRegExp(":","g").replace(err.mess,"=").c_str()));
		    mStatWork[i_st].second.cntr = mRestTm;
		}
		throw;
	    }
	}

    return atoi(node.attr("err").c_str());
}

void TMdContr::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if(opt->name() == "info")
    {
	TController::cntrCmdProc(opt);
	ctrRemoveNode(opt,"/cntr/cfg/PERIOD");
        ctrMkNode("fld",opt,-1,"/cntr/cfg/SCHEDULE",mSched.fld().descr(),/*startStat()?R_R_R_:*/RWRWR_,"root",SDAQ_ID,4,
            "tp","str","dest","sel_ed","sel_list",TMess::labSecCRONsel(),"help",TMess::labSecCRON());
	ctrMkNode("fld",opt,-1,"/cntr/cfg/PRIOR",cfg("PRIOR").fld().descr(),startStat()?R_R_R_:RWRWR_,"root",SDAQ_ID,1,"help",TMess::labTaskPrior());
	ctrMkNode("fld",opt,-1,"/cntr/cfg/TM_REST_DT",cfg("TM_REST_DT").fld().descr(),RWRWR_,"root",SDAQ_ID,1,
	    "help",_("Zero for disable archive access."));
	ctrMkNode("fld",opt,-1,"/cntr/cfg/SYNCPER",cfg("SYNCPER").fld().descr(),RWRWR_,"root",SDAQ_ID,1,"help",_("Zero for disable periodic sync."));
	ctrMkNode("fld",opt,-1,"/cntr/cfg/STATIONS",cfg("STATIONS").fld().descr(),enableStat()?R_R_R_:RWRWR_,"root",SDAQ_ID,4,"tp","str","cols","100","rows","4",
	    "help",_("Remote OpenSCADA stations' identifiers list used into it controller."));
	ctrMkNode("fld",opt,-1,"/cntr/cfg/CNTRPRM",cfg("CNTRPRM").fld().descr(),enableStat()?R_R_R_:RWRWR_,"root",SDAQ_ID,4,"tp","str","cols","100","rows","4",
	    "help",_("Remote OpenSCADA full controller's or separated controller's parameters list. Address example:\n"
		     "  System.AutoDA - for controller;\n"
		     "  System.AutoDA.UpTimeStation - for controller's parameter."));
	ctrMkNode("comm",opt,-1,"/cntr/cfg/host_lnk",_("Go to remote stations list configuration"),RWRW__,"root",SDAQ_ID,1,"tp","lnk");
	return;
    }

    //> Process command to page
    string a_path = opt->attr("path");
    if(a_path == "/cntr/cfg/host_lnk" && ctrChkNode(opt,"get",RWRW__,"root",SDAQ_ID,SEC_RD))
    {
	SYS->transport().at().setSysHost(true);
	opt->setText("/Transport");
    }
    else TController::cntrCmdProc(opt);
}

//******************************************************
//* TMdPrm                                             *
//******************************************************
TMdPrm::TMdPrm( string name, TTipParam *tp_prm ) : TParamContr(name,tp_prm), isPrcOK(false), isEVAL(true), isSynced(false), p_el("w_attr")
{
    setToEnable(true);
}

TMdPrm::~TMdPrm( )
{
    nodeDelAll();
}

void TMdPrm::postEnable( int flag )
{
    TParamContr::postEnable(flag);
    if(vlCfg())	setVlCfg(NULL);
    if(!vlElemPresent(&p_el))	vlElemAtt(&p_el);
}

TMdContr &TMdPrm::owner( )	{ return (TMdContr&)TParamContr::owner(); }

void TMdPrm::enable( )
{
    if(enableStat())	return;

    TParamContr::enable();
}

void TMdPrm::disable( )
{
    if(!enableStat())	return;

    TParamContr::disable();

    //> Set EVAL to parameter attributes
    vector<string> ls;
    elem().fldList(ls);
    for(unsigned i_el = 0; i_el < ls.size(); i_el++)
	vlAt(ls[i_el]).at().setS(EVAL_STR,0,true);
}

void TMdPrm::setCntrAdr( const string &vl )
{
    if(vl.empty()) { mCntrAdr = ""; return; }

    string scntr;
    for(int off = 0; (scntr=TSYS::strSepParse(mCntrAdr,0,';',&off)).size(); )
	if(scntr == vl) return;
    mCntrAdr += vl+";";
}

void TMdPrm::load_( )
{
    //> Load from cache
    TParamContr::load_();
    //> Restore attributes from cache
    try
    {
	XMLNode attrsNd;
	attrsNd.load(cfg("ATTRS").getS());
	for(unsigned i_el = 0; i_el < attrsNd.childSize(); i_el++)
	{
	    XMLNode *aEl = attrsNd.childGet(i_el);
    	    if(vlPresent(aEl->attr("id"))) continue;
	    p_el.fldAdd(new TFld(aEl->attr("id").c_str(),aEl->attr("nm").c_str(),(TFld::Type)atoi(aEl->attr("tp").c_str()),
		atoi(aEl->attr("flg").c_str()),"","",aEl->attr("vals").c_str(),aEl->attr("names").c_str()));
	    //vlAt(aEl->attr("id")).at().setS(aEl->text());
	}
    } catch(TError err) { }

    //> Sync attributes list
    sync();
}

void TMdPrm::save_( )
{
    //> Prepare attributes cache configuration
    XMLNode attrsNd("Attrs");
    vector<string> ls;
    elem().fldList(ls);
    for(unsigned i_el = 0; i_el < ls.size(); i_el++)
    {
	AutoHD<TVal> vl = vlAt(ls[i_el]);
	attrsNd.childAdd("a")->setAttr("id",ls[i_el])->
			       setAttr("nm",vl.at().fld().descr())->
			       setAttr("tp",TSYS::int2str(vl.at().fld().type()))->
			       setAttr("flg",TSYS::int2str(vl.at().fld().flg()))->
			       setAttr("vals",vl.at().fld().values())->
			       setAttr("names",vl.at().fld().selNames());
			       //setText(vl.at().getS());
    }
    cfg("ATTRS").setS(attrsNd.save(XMLNode::BrAllPast));

    //> Save to cache
    TParamContr::save_();
}

void TMdPrm::sync( )
{
    //> Request and update attributes list
    string scntr;
    XMLNode req("CntrReqs");
    for(int c_off = 0; (scntr=TSYS::strSepParse(cntrAdr(),0,';',&c_off)).size(); )
	try
	{
	    vector<string> als;
	    req.clear()->setAttr("path",scntr+id());
	    req.childAdd("get")->setAttr("path","/%2fprm%2fcfg%2fNAME");
	    req.childAdd("get")->setAttr("path","/%2fprm%2fcfg%2fDESCR");
	    req.childAdd("list")->setAttr("path","/%2fserv%2fattr");
	    if(owner().cntrIfCmd(req))	throw TError(req.attr("mcat").c_str(),req.text().c_str());

	    setName(req.childGet(0)->text());
	    setDescr(req.childGet(1)->text());
	    //>> Check and create new attributes
	    for(unsigned i_a = 0; i_a < req.childGet(2)->childSize(); i_a++)
	    {
		XMLNode *ael = req.childGet(2)->childGet(i_a);
		als.push_back(ael->attr("id"));
		if(vlPresent(ael->attr("id")))	continue;
		TFld::Type tp = (TFld::Type)atoi(ael->attr("tp").c_str());
		p_el.fldAdd( new TFld( ael->attr("id").c_str(),ael->attr("nm").c_str(),tp,
		    (atoi(ael->attr("flg").c_str())&(TFld::Selected|TFld::NoWrite|TFld::HexDec|TFld::OctDec|TFld::FullText))|TVal::DirWrite|TVal::DirRead,
		    "","",ael->attr("vals").c_str(),ael->attr("names").c_str()) );
		modif(true);
	    }
	    //>> Check for remove attributes
	    for(int i_p = 0; i_p < (int)p_el.fldSize(); i_p++)
	    {
    		unsigned i_l;
    		for(i_l = 0; i_l < als.size(); i_l++)
        	    if(p_el.fldAt(i_p).name() == als[i_l])
            		break;
    		if(i_l >= als.size())
        	    try{ p_el.fldDel(i_p); i_p--; modif(true); }
        	    catch(TError err){ mess_warning(err.cat.c_str(),err.mess.c_str()); }
	    }
	    isSynced = true;
	    return;
	}catch(TError err) { continue; }
}

void TMdPrm::vlGet( TVal &val )
{
    if(val.name() == "err" && (!enableStat() || !owner().startStat())) TParamContr::vlGet(val);
}

void TMdPrm::vlSet( TVal &valo, const TVariant &pvl )
{
    if(!enableStat() || !owner().startStat())	valo.setI(EVAL_INT,0,true);
    if(valo.getS() == EVAL_STR || valo.getS() == pvl.getS()) return;

    XMLNode req("set");

    //> Send to active reserve station
    if(owner().redntUse())
    {
	req.setAttr("path",nodePath(0,true)+"/%2fserv%2fattr")->childAdd("el")->setAttr("id",valo.name())->setText(valo.getS());
	SYS->daq().at().rdStRequest(owner().workId(),req);
	return;
    }
    //> Direct write
    string scntr;
    for(int c_off = 0; (scntr=TSYS::strSepParse(cntrAdr(),0,';',&c_off)).size(); )
	try
	{
	    req.clear()->setAttr("path",scntr+id()+"/%2fserv%2fattr")->
		childAdd("el")->setAttr("id",valo.name())->setText(valo.getS());
	    if(owner().cntrIfCmd(req))	throw TError(req.attr("mcat").c_str(),req.text().c_str());
	}catch(TError err) { continue; }
}

void TMdPrm::vlArchMake( TVal &val )
{
    TParamContr::vlArchMake(val);

    if(val.arch().freeStat())	return;
    val.arch().at().setSrcMode(TVArchive::PassiveAttr);
    val.arch().at().setPeriod(owner().period() ? owner().period()/1000 : 1000000);
    val.arch().at().setHardGrid(true);
    val.arch().at().setHighResTm(true);
}

TVal* TMdPrm::vlNew( )	{ return new TMdVl(); }

void TMdPrm::cntrCmdProc( XMLNode *opt )
{
    string a_path = opt->attr("path");

    //> Service commands process
    if(a_path.substr(0,6) == "/serv/") { TParamContr::cntrCmdProc(opt); return; }

    //> Get page info
    if(opt->name() == "info")
    {
	TValue::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Parameter: ")+name());
	if(ctrMkNode("area",opt,0,"/prm",_("Parameter")))
	{
	    if(ctrMkNode("area",opt,-1,"/prm/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/prm/st/type",_("Type"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		if(owner().enableStat())
		    ctrMkNode("fld",opt,-1,"/prm/st/en",_("Enable"),RWRWR_,"root",SDAQ_ID,1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/prm/st/id",_("Id"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
		ctrMkNode("fld",opt,-1,"/prm/st/nm",_("Name"),R_R_R_,"root",SDAQ_ID,1,"tp","str");
	    }
	    XMLNode *cfgN = ctrMkNode("area",opt,-1,"/prm/cfg",_("Configuration"));
	    if(cfgN)
	    {
		//>> Get remote parameter's config section
		string scntr;
		XMLNode req("info");
		for(int c_off = 0; (scntr=TSYS::strSepParse(cntrAdr(),0,';',&c_off)).size(); )
		    try
		    {
			req.clear()->setAttr("path",scntr+id()+"/%2fprm%2fcfg");
			if(owner().cntrIfCmd(req)) throw TError(req.attr("mcat").c_str(),req.text().c_str());
			break;
		    }catch(TError err) { continue; }
		if(req.childSize())
		{
		    *cfgN = *req.childGet(0);
		    cfgN->setAttr("dscr",_("Remote station configuration"));
		}
	    }
	}
	return;
    }
    //> Process command to page
    if(a_path == "/prm/st/type" && ctrChkNode(opt))		opt->setText(type().descr);
    else if(a_path == "/prm/st/en")
    {
	if(ctrChkNode(opt,"get",RWRWR_,"root",SDAQ_ID,SEC_RD))	opt->setText(enableStat()?"1":"0");
	if(ctrChkNode(opt,"set",RWRWR_,"root",SDAQ_ID,SEC_WR))
	{
	    if(!owner().enableStat()) throw TError(nodePath().c_str(),_("Controller is not started!"));
	    else atoi(opt->text().c_str())?enable():disable();
	}
    }
    else if(a_path == "/prm/st/id" && ctrChkNode(opt))	opt->setText(id());
    else if(a_path == "/prm/st/nm" && ctrChkNode(opt))	opt->setText(name());
    else if(a_path.substr(0,8) == "/prm/cfg")
    {
	//> Request to remote host
	string scntr;
	for(int c_off = 0; (scntr=TSYS::strSepParse(cntrAdr(),0,';',&c_off)).size(); )
	    try
	    {
		opt->setAttr("path",scntr+id()+"/"+TSYS::strEncode(a_path,TSYS::PathEl));
		if(owner().cntrIfCmd(*opt)) throw TError(opt->attr("mcat").c_str(),opt->text().c_str());
	    }catch(TError err) { continue; }
	opt->setAttr("path",a_path);
    }
    else TValue::cntrCmdProc(opt);
}

//******************************************************
//* TMdVl                                              *
//******************************************************
TMdPrm &TMdVl::owner( )	{ return *(dynamic_cast<TMdPrm*>(nodePrev())); }

void TMdVl::cntrCmdProc( XMLNode *opt )
{
    if(!arch().freeStat()) { TVal::cntrCmdProc(opt); return; }

    string a_path = opt->attr("path");
    //> Service commands process
    if(a_path == "/serv/val" && owner().owner().restDtTm())	//Values access
    {
	//>> Request to remote station
	string scntr;
	for(int c_off = 0; (scntr=TSYS::strSepParse(owner().cntrAdr(),0,';',&c_off)).size(); )
	    try
	    {
		opt->setAttr("path",scntr+owner().id()+"/"+name()+"/"+TSYS::strEncode(a_path,TSYS::PathEl));
		if(!owner().owner().cntrIfCmd(*opt)) break;
	    }catch(TError err) { continue; }
	opt->setAttr("path",a_path);
	return;
    }

    TVal::cntrCmdProc(opt);
}
