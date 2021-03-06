// Main program.

// Copyright (C) 2009, 2013, 2017  Embecosm Limited <info@embecosm.com>

// Contributor Jeremy Bennett <jeremy.bennett@embecosm.com>
// Contributor Ian Bolton <ian.bolton@embecosm.com>

// This file is part of the RISC-V GDB server

// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include "config.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <getopt.h>

// RISC-V headers in general and for each target

#include "ITarget.h"

#ifdef BUILD_GDBSIM_MODEL
#include "GdbSim.h"
#endif /* BUILD_GDBSIM_MODEL */

#ifdef BUILD_PICORV32_MODEL
#include "Picorv32.h"
#endif /* BUILD_PICORV32_MODEL */

#ifdef BUILD_RI5CY_MODEL
#include "Ri5cy.h"
#endif /* BUILD_RI5CY_MODEL */


// Class headers

#include "GdbServer.h"
#include "TraceFlags.h"

#include "RspConnection.h"
#include "StreamConnection.h"

using std::atoi;
using std::cerr;
using std::cout;
using std::endl;
using std::ostream;
using std::strcmp;


//! The RISC-V model

static ITarget *globalCpu = nullptr;

static const std::string gdbserver_name =
#ifdef BUILD_64_BIT
    "riscv64-gdbserver"
#else
    "riscv32-gdbserver"
#endif
    ;

//! Convenience function to output the usage to a specified stream.

//! @param[in] s  Output stream to use.

static void
usage (ostream & s)
{
  s << "Usage: " << gdbserver_name << " --core | -c <corename>" << endl
    << "                         [ --trace | -t <traceflag> ]" << endl
    << "                         [ --silent | -q ]" << endl
    << "                         [ --stdin | -s ]" << endl
    << "                         [ --help | -h ]" << endl
    << "                         [ --version | -v ]" << endl
    << "                         <rsp-port>" << endl
    << endl
    << "The trace option may appear multiple times. Trace flags are:" << endl
    << "  rsp     Trace RSP packets" << endl
    << "  conn    Trace RSP connection handling" << endl
    << "  break   Trace breakpoint handling" << endl
    << "  vcd     Generate a Verilog Change Dump" << endl
    << "  silent  Minimize informative messages (synonym for -q)" << endl;

}	// usage ()


//! Convenience function to output the version information

//! @param[in] s  Output stream to use.

static void
show_version (ostream &s)
{
  // @todo maybe include git information, and possibly pull version number
  // from some other source rather than hard-coding it here.
  s << gdbserver_name << " version 0.1" << endl;
}	// show_version ()

//! Create a new ITarget instance from a cpu name, or return NULL
//! if not matching target is known.

//! @param[in] name  C string containing cpu model name.
//! @param[in] traceFlags  Pointer to TraceFlags used to create model.
//! @return  Pointer to new ITarget instance, or nullptr.

static ITarget *
createCpu (const char *name, TraceFlags *traceFlags)
{
  ITarget *cpu;

  // The RISC-V model
  if (0)
    /* Nothing.  */;
#ifdef BUILD_GDBSIM_MODEL
  else if (0 == strcasecmp ("GDBSIM", name))
    cpu = new GdbSim (traceFlags);
#endif /* BUILD_GDBSIM_MODEL */
#ifdef BUILD_PICORV32_MODEL
  else if (0 == strcasecmp ("PicoRV32", name))
    cpu = new Picorv32 (traceFlags);
#endif /* BUILD_PICORV32_MODEL */
#ifdef BUILD_RI5CY_MODEL
  else if (0 == strcasecmp ("RI5CY", name))
    cpu = new Ri5cy (traceFlags);
#endif /* BUILD_RI5CY_MODEL */
  else
    {
      cerr << "ERROR: Unrecognized core: " << name << ": exiting" << endl;
      return  nullptr;
    }

  return cpu;
}	// createCpu



//! Main function

//! @see usage () for information on the parameters.  Instantiates the core
//! and GDB server.

//! @param[in] argc  Number of arguments.
//! @param[in] argv  Vector or arguments.
//! @return  The return code for the program.

int
main (int   argc,
      char *argv[] )
{
  // Argument handling.

  char         *coreName = nullptr;
  bool          from_stdin = false;
  int           port = -1;
  TraceFlags *  traceFlags = new TraceFlags ();
  int           nextArg;

  while (true) {
    int c;
    int longOptind = 0;
    static struct option longOptions[] = {
      {"core",   required_argument, nullptr,  'c' },
      {"help",   no_argument,       nullptr,  'h' },
      {"silent", no_argument,       nullptr,  'q' },
      {"trace",  required_argument, nullptr,  't' },
      {"stdin",  no_argument,       nullptr,  's' },
      {"version", no_argument,      nullptr,  'v' },
      {0,       0,                 0,  0 }
    };

    if ((c = getopt_long (argc, argv, "c:hqt:sv", longOptions, &longOptind)) == -1)
      break;

    switch (c) {
    case 'c':
      coreName = strdup (optarg);
      break;

    case 'h':
      usage (cout);
      return  EXIT_SUCCESS;

    case 'q':

      traceFlags->flag ("silent", true);
      break;

    case 't':

      if (!traceFlags->isFlag (optarg))
	{
	  cerr << "ERROR: Bad trace flag " << optarg << endl;
	  usage (cerr);
	  return EXIT_FAILURE;
	}

      traceFlags->flag (optarg, true);
      break;

    case 's':
      from_stdin = true;
      break;

    case '?':
    case ':':
      usage (cerr);
      return  EXIT_FAILURE;

    case 'v':
      show_version (cout);
      return EXIT_SUCCESS;

    default:
      cerr << "ERROR: getopt_long returned character code " << c << endl;
      return  EXIT_FAILURE;
    }
  }

  // Check for 1 positional arg (sometimes).  Also, stop using OPTIND, this
  // is a global and can be modified if we ever invoke the getopt framework
  // again (for example in starting a target).
  nextArg = optind;
  if (((argc - nextArg) != 1 && !from_stdin)
      || coreName == nullptr)
    {
      usage (cerr);
      return  EXIT_FAILURE;
    }

  // Create the cpu model.
  globalCpu = createCpu (coreName, traceFlags);
  if (globalCpu == nullptr)
    return  EXIT_FAILURE;

  AbstractConnection *conn;
  GdbServer::KillBehaviour killBehaviour;
  if (from_stdin)
    {
      conn = new StreamConnection (traceFlags);
      killBehaviour = GdbServer::KillBehaviour::EXIT_ON_KILL;
    }
  else
    {
      port = atoi (argv[nextArg]);
      conn = new RspConnection (port, traceFlags);
      killBehaviour = GdbServer::KillBehaviour::RESET_ON_KILL;
    }

  // The RSP server, connecting it to its CPU.

  GdbServer *gdbServer = new GdbServer (conn, globalCpu, traceFlags,
                                        killBehaviour);
  globalCpu->gdbServer (gdbServer);

  // Run the GDB server.

  int ret = gdbServer->rspServer ();

  // Free memory

  delete  conn;
  delete  gdbServer;
  delete  globalCpu;
  delete  traceFlags;
  free (coreName);

  return ret;

}	/* sc_main() */


//! Function to handle $time calls in the Verilog

double
sc_time_stamp ()
{
  // If we are called before cpu has been constructed, return 0.0
  if (globalCpu != nullptr)
    return globalCpu->timeStamp ();
  else
    return 0.0;
}


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// End:
