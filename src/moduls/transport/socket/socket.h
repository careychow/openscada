#ifndef SOCKET_H
#define SOCKET_H

#include <pthread.h>

#include <ttransports.h>

#define S_NM_TCP  "TCP"
#define S_NM_UDP  "UDP"
#define S_NM_UNIX "UNIX"

#define SOCK_TCP  0
#define SOCK_UDP  1
#define SOCK_UNIX 2

namespace Sockets
{
    class TSocketIn;

    struct SSockIn 
    {
	TSocketIn *s_in;
	int       cl_sock;
	string    sender;
    };

    struct SSockCl
    {
	pid_t  cl_pid;    // Client's pids
	int    cl_sock;
    };

    class TSocketIn: public TTransportIn
    {
	public:
	    /*
	     * Open input socket <name> for locale <address>
	     * address : <type:<specific>>
	     * 	type: 
	     * 	  TCP  - TCP socket with  "UDP:<host>:<port>"
	     * 	  UDP  - UDP socket with  "TCP:<host>:<port>"
	     * 	  UNIX - UNIX socket with "UNIX:<path>"
	     */
	    TSocketIn(string name, TTipTransport *owner);
	    ~TSocketIn();
	    
	    void start();
	    void stop();
	public:
	    
	private:
	    static void *Task(void *);
	    static void *ClTask(void *);
	
	    void ClSock( SSockIn &s_in );
	    int  PutMess( int sock, string &request, string &answer, string sender, int hds = -1 );

	    void RegClient(pid_t pid, int i_sock);
	    void UnregClient(pid_t pid);
	private:
	    pthread_t pthr_tsk;
	    int       sock_fd;
	    unsigned  sock_res;
	
	    bool      endrun;      // Command for stop task	    
	    bool      endrun_cl;   // Command for stop client tasks
	    //params
	    int       &max_queue;   // max queue for TCP, UNIX sockets
	    int       &max_fork;    // maximum forking (opened sockets)
	    int       &buf_len;     // input buffer length	
    
    	    int       type;        // socket's types 
    	    string    path;        // path to file socket for UNIX socket
    	    string    host;        // host for TCP/UDP sockets
    	    string    port;        // port for TCP/UDP sockets
    	    int       mode;        // mode for TCP/UNIX sockets (0 - no hand; 1 - hand connect)

	    bool            cl_free;  // Clients stoped
    	    vector<SSockCl> cl_id;    // Client's pids
    };

    class TSocketOut: public TTransportOut
    {
	public:
	    /*
	     * Open output socket <name> for locale <address>
	     * address : <type:<specific>>
	     * 	type: 
	     * 	  TCP  - TCP socket with  "UDP:<host>:<port>"
	     * 	  UDP  - UDP socket with  "TCP:<host>:<port>"
	     * 	  UNIX - UNIX socket with "UNIX:<path>"
	     */
	    TSocketOut(string name, TTipTransport *owner);
	    ~TSocketOut();

	    void start();
	    void stop();

	    int IOMess(char *obuf, int len_ob, char *ibuf = NULL, int len_ib = 0, int time = 0 );

	private:
	    int       sock_fd;
    
    	    int       type;        // socket's types 
    	    struct sockaddr_in  name_in;
    	    struct sockaddr_un  name_un;
    };

    class TTransSock: public TTipTransport
    {
	public:
	    TTransSock( string name );
	    ~TTransSock();
	    
	    TTransportIn  *In( string name );
	    TTransportOut *Out( string name );	    
	
	    void mod_CheckCommandLine( );
	    void mod_UpdateOpt();	
	public:
	    int       max_queue;   // max queue for TCP, UNIX sockets
	    int       max_fork;    // maximum forking (opened sockets)
	    int       buf_len;     // input buffer length	    
	private:
	
	    string opt_descr( );
	    //================== Controll functions ========================
	    void ctr_fill_info( XMLNode *inf );
	    void ctr_din_get_( string a_path, XMLNode *opt );
	    void ctr_din_set_( string a_path, XMLNode *opt );
	private:
	    static const char *i_cntr; 
    };
}

#endif //SOCKET_H
