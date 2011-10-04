#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/PluginLoader.h"
#include "SKIR/SKIRRuntime.h"

#include "events.pb.h"

#include <tbb/task_scheduler_init.h>
#include <cerrno>
#include <iostream>
#include <ctime>
#include <string>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

using namespace llvm;

namespace {
    cl::opt<std::string>
    InputFile(cl::desc("<input bitcode>"), cl::Positional, cl::init("-"));

    cl::list<std::string>
    InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

#if 0
    // Determine optimization level.
    cl::opt<char>
    OptLevel("O",
	     cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
		      "(default = '-O2')"),
	     cl::Prefix,
	     cl::ZeroOrMore,
           cl::init(' '));
#endif
    
    cl::opt<std::string>
    EntryFunc("entry-function",
	      cl::desc("Specify the entry function (default = 'main') "
		       "of the executable"),
	      cl::value_desc("function"),
	      cl::init("main"));
  
    cl::opt<std::string>
    FakeArgv0("fake-argv0",
	      cl::desc("Override the 'argv[0]' value passed into the executing"
		       " program"), cl::value_desc("executable"));
  
    cl::opt<std::string>
    Echo("echo", cl::desc("echo"), cl::value_desc("string"));
  
    cl::opt<bool>
    DisableCoreFiles("disable-core-files", cl::Hidden,
		     cl::desc("Disable emission of core files if possible"));

    cl::opt<bool>
    NoLazyCompilation("disable-lazy-compilation",
		      cl::desc("Disable JIT lazy compilation"),
		      cl::init(false));

    cl::opt<int>
    NThreads("n", cl::desc("Number of threads to use"), cl::init(1));
    
    cl::opt<int>
    RunMode("t", cl::desc("Type of scheduler to use"), cl::init(0));
    
    cl::opt<bool>
    Verbose("v", cl::desc("verbose"), cl::init(false));

    cl::opt<int>
    Port("p", cl::desc("event server port"), cl::init(7547));
}

SKIRRuntime skirrt;
bool die = false;

using boost::asio::ip::tcp;

const int max_length = 4096;

typedef boost::shared_ptr<tcp::socket> socket_ptr;

void session(socket_ptr sock)
{
    std::string data;
    
    while (1) {
	char d[max_length+1];
	boost::system::error_code error;
	size_t length = sock->read_some(boost::asio::buffer(d,max_length), error);
	d[length] = 0;
	std::string new_data(d, length);
	data += new_data;
	
	if (error == boost::asio::error::eof)
	    break; // Connection closed cleanly by peer.
	else if (error)
	    std::cerr << "Exception in thread: " << error << "\n";
    }

    std::stringstream ss;
    ss.str(data);
    skirrt.onEvent(&ss);

    if (size_t n = ss.str().length())
	boost::asio::write(*sock, boost::asio::buffer(ss.str().c_str(), n));
}

void server(boost::asio::io_service& io_service, short port)
{
  tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    socket_ptr sock(new tcp::socket(io_service));
    a.accept(*sock);
    //boost::thread t(boost::bind(session, sock));
    session(sock);
  }
}

void skirrt_event_server(void)
{
    boost::asio::io_service io_service;
    
    server(io_service, Port);
}

int
main(int argc, char **argv, char * const *envp)
{
    cl::ParseCommandLineOptions(argc, argv, "skir\n");
    tbb::task_scheduler_init tbb_init(NThreads);

    skirrt.setVerbose(Verbose);
    skirrt.start(NThreads);
    boost::thread t(skirrt_event_server);

    if (!Echo.empty())
    {
	EchoRequest echo;
	echo.set_str(Echo);
	std::stringstream event;
	event << "EchoRequest\n";
	echo.SerializeToOstream(&event);
	skirrt.onEvent(&event);
    }
    
    if (InputFile.compare("-")) {
	RunModuleRequest runModule;
	runModule.set_input_module(InputFile);
	for (int i=0, j=InputArgv.size(); i<j; i++)
	    runModule.add_input_argv()->assign(InputArgv[i]);
	runModule.set_request_id(1);
	runModule.set_fake_argv0(FakeArgv0);
	runModule.set_entry_func(EntryFunc);
	runModule.set_nthreads(NThreads);
	runModule.set_run_mode(RunMode);
	std::stringstream event;
	event << "RunModuleRequest\n";
	runModule.SerializeToOstream(&event);
	skirrt.onEvent(&event);
	//skirrt.run(InputFile, InputArgv, FakeArgv0, EntryFunc, envp, NThreads, RunMode);
    }

    t.join();

    return 0;
}
