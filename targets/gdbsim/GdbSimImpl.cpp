// GDB RSP server GDBSIM CPU model: definition

// Copyright (C) 2017  Embecosm Limited <info@embecosm.com>

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

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <cstring>

#include "GdbServer.h"
#include "GdbSimImpl.h"
#include "TraceFlags.h"

#include "gdb/signals.h"
#include "gdb/sim-riscv.h"

//! Constructor.

//! Initialize the counters and instantiate the Verilator model. Take the
//! model through its reset sequence.

//! @param[in] flags  The trace flags

GdbSimImpl::GdbSimImpl (const TraceFlags *flags)
  : mFlags (flags),
    mHaveReset (false)
{
  reset (ITarget::ResetType::COLD);
}	// GdbSimImpl::GdbSimImpl ()


//! Destructor.

//! Close VCD and delete the Verilator model.

GdbSimImpl::~GdbSimImpl ()
{
}	// GdbSimImpl::~GdbSimImpl ()


//! Resume execution with no timeout

//! @todo

//! @param[in]  step         The type of resume to carry out. Initially just
//!                          STEP and CONTINUE should suffice.
//! @return Why the target stopped.

ITarget::ResumeRes
GdbSimImpl::resume (ITarget::ResumeType step __attribute__ ((unused)))
{
  return resume (step, std::chrono::duration <double>::zero ());
}	// GdbSimImpl::resume ()


//! Resume execution with timeout

//! @todo

//! @param[in]  step         The type of resume to carry out. Initially just
//!                          STEP and CONTINUE should suffice.
//! @param[in]  timeout      Maximum time for execution to continue.
//! @return Why the target stopped.

ITarget::ResumeRes
GdbSimImpl::resume (ITarget::ResumeType step,
                    std::chrono::duration <double> timeout)
{
  switch (step)
    {
    case ITarget::ResumeType::STEP:
      return doOneStep (timeout);

    case ITarget::ResumeType::CONTINUE:
      return doRunToBreak (timeout);

    default:
      // Shouldn't see anything else here.
      std::cerr << "unexpected step type " << step << std::endl;
      abort ();
    }

  return ITarget::ResumeRes::FAILURE;
}	// GdbSimImpl::resume ()


//! Terminate.

//! This has no meaning for an embedded system, so it does nothing.

ITarget::ResumeRes
GdbSimImpl::terminate ()
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return ITarget::ResumeRes::FAILURE;
}	// GdbSimImpl::terminate ()


//! Reset

//! The only different between WARM and COLD is that we reset the counters. In
//! both cases we put the processor through its reset sequence.

//! @param[in]  type  Type of reset (can be warm or cold)
//! @return  Whether the reset was successful, which is always SUCCESS

ITarget::ResumeRes
GdbSimImpl::reset (ITarget::ResetType type __attribute__ ((unused)))
{
  char * const sim_argv[] = { strdup ("gdbsim"), NULL };

  if (mHaveReset)
    gdb_callback.shutdown (&gdb_callback);
  mHaveReset = true;

  gdb_callback = default_callback;
  gdb_callback.init (&gdb_callback);

  gdbsim_desc = sim_open (SIM_OPEN_DEBUG, &gdb_callback,
                          NULL, sim_argv);

  if (sim_create_inferior (gdbsim_desc, NULL, sim_argv, NULL) != SIM_RC_OK)
    abort ();

  return ITarget::ResumeRes::SUCCESS;
}	// reset ()


//! Accessor for the cycle count

//! The value is set during the execution of the model.

//! @return  The number of cycles executed since startup or the last cold
//!          reset.

uint64_t
GdbSimImpl::getCycleCount () const
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return 0;
}	// GdbSimImpl::getCycleCount ()


//! Accessor for the instruction count

//! The value is set during the execution of the model.

//! @return  The number of instructions executed since startup or the last cold
//!          reset.

uint64_t
GdbSimImpl::getInstrCount () const
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return 0;
}	// GdbSimImpl::getInstrCount ()


//! Read a register

//! We assume that the core is halted. If not we have a problem.  We use the
//! debug unit and need a different mechanism for the program counter (which
//! is the NEXT program counter).

//! @param[in]  reg    The register to read
//! @param[out] value  The value read
//! @return  The size of the register read in bytes (always 4)

std::size_t
GdbSimImpl::readRegister (const int reg, uint_reg_t &value)
{
  unsigned char *buf = reinterpret_cast <unsigned char *> (&value);
  std::size_t reg_size =
    sim_fetch_register (gdbsim_desc, reg, buf, sizeof (value));
  if (reg_size != sizeof (value))
    {
      if (reg_size == 0 || reg_size == static_cast <std::size_t> (-1))
        std::cerr << "error: failed to read register 0x"
                  << std::hex << reg << std::endl;
      else
        std::cerr << "error: failed to read register " << reg << " due to "
                  << "incorrect size, expected " << sizeof (value)
                  << " was " << reg_size << std::endl;
      value = 0;
    }
  return reg_size;
}	// GdbSimImpl::readRegister ()


//! Write a register

//! We assume that the core is halted. If not we have a problem.  We use the
//! debug unit and need a different mechanism for the program counter (which
//! is the NEXT program counter).

//! @param[in]  reg    The register to write
//! @param[out] value  The value to write
//! @return  The size of the register read in bytes (always 4)

std::size_t
GdbSimImpl::writeRegister (const int reg __attribute__ ((unused)),
			  const uint_reg_t value __attribute__ ((unused)))
{
  unsigned char *buf
    = const_cast <unsigned char *> (reinterpret_cast <const unsigned char *> (&value));
  int res
    = sim_store_register (gdbsim_desc, reg, buf, sizeof (value));
  if (res < 0)
    {
      std::cerr << "In " << __PRETTY_FUNCTION__ << " failed to write to "
                << "register " << reg << std::endl;
    }
  return sizeof (value);
}	// GdbSimImpl::writeRegister ()


//! Read data from memory

//! You can't write memory via the debug registers. So we need a Verilator
//! task in top.sv to manually read/write memory and then we can use
//! that. With the processor halted it *should* be safe.

//! Otherwise we may have to put the memory external to the core instead of in
//! top.sv, but that would be painful to implement.

//! @param[in]  addr    Address to read from
//! @param[out] buffer  Buffer into which read data is placed
//! @param[in]  size    Number of bytes to read
//! @return  Number of bytes read

std::size_t
GdbSimImpl::read (const uint32_t addr __attribute__ ((unused)),
		 uint8_t *buffer __attribute__ ((unused)),
		 const std::size_t size __attribute__ ((unused))) const
{
  int ans
    = sim_read (gdbsim_desc, addr, buffer, size);
  if (ans == 0)
    {
      //std::cerr << "In " << __PRETTY_FUNCTION__ << " failed to read "
      //        << "memory at " << std::hex << addr << std::endl;
    }
  return ans;
}	// GdbSimImpl::read ()


//! Write data to memory.

//! @todo

//! For discussion, @see read ()

//! @param[in] addr    Address to write to
//! @param[in] buffer  Buffer of data to write
//! @param[in] size    Number of bytes to write
//! @return  Number of bytes written

std::size_t
GdbSimImpl::write (const uint32_t addr __attribute__ ((unused)),
		  const uint8_t *buffer __attribute__ ((unused)),
		  const std::size_t size __attribute__ ((unused)))
{
  int res
    = sim_write (gdbsim_desc, addr, buffer, size);
  if (static_cast <int> (size) != res)
    {
      std::cerr << "In " << __PRETTY_FUNCTION__ << " failed to "
                << "write to memory at " << std::hex << addr << std::endl;
    }

  return res;
}	// GdbSimImpl::write ()


//! Insert a matchpoint (breakpoint or watchpoint)

//! @todo

//! It is acceptable to always fail. GDB will then use memory breakpoints,
//! implemented by writing EBREAK to the location.

//! @param[in] addr       Address for the matchpoint
//! @param[in] matchType  Type of breakpoint or watchpoint
//! @return  TRUE if the operation was successful, false otherwise.

bool
GdbSimImpl::insertMatchpoint (const uint32_t  addr __attribute__ ((unused)),
			     const ITarget::MatchType  matchType __attribute__ ((unused)))
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return  false;
}	// GdbSimImpl::insertMatchpoint ()


//! Insert a matchpoint (breakpoint or watchpoint)

//! @todo

//! It is acceptable to always fail. GDB will then use memory breakpoints,
//! implemented by writing EBREAK to the location.

//! @param[in] addr       Address for the matchpoint
//! @param[in] matchType  Type of breakpoint or watchpoint
//! @return  TRUE if the operation was successful, false otherwise.

bool
GdbSimImpl::removeMatchpoint (const uint32_t  addr __attribute__ ((unused)),
			     const ITarget::MatchType  matchType __attribute__ ((unused)) )
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return  false;
}	// GdbSimImpl::removeMatchpoint ()


//! Generic pass through of command

//! @todo

//! For now we have no commands, so we should never receive this, and thus
//! always fail.

//!@param[in]  cmd     The command to process
//!@param[out] stream  A stream to write any output from the command
//!@return  TRUE if the command was handled successfully, FALSE otherwise. We
//!         awlways return FALSE.

bool
GdbSimImpl::command (const std::string  cmd __attribute__ ((unused)),
		    std::ostream & stream __attribute__ ((unused)))
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
  return false;
}	// GdbSimImpl::command ()


//! Record the server we are associated with.

//! @param[in] server  Our invoking server.

void
GdbSimImpl::gdbServer (GdbServer *server)
{
  mServer = server;
}	// GdbSimImpl::gdbServer ()


//! Provide a time stamp (needed for $time)

//! We count in nanoseconds since (cold) reset.

//! @return  The time in nanoseconds (as specified in Verilator user manual)

double
GdbSimImpl::timeStamp ()
{
  std::cerr << "In " << __PRETTY_FUNCTION__ << std::endl;
  abort ();
}	// GdbSimImpl::timeStamp ()



ITarget::ResumeRes
GdbSimImpl::doOneStep (std::chrono::duration <double> timeout)
{
  uint32_t insn;
  uint16_t cinsn;
  uint_reg_t stepAddr;
  enum sim_stop stop_reason;
  int signo;
  (void) timeout;

  /* If we are sat looking at a syscall (ECALL instruction) then nudge the
     $pc past the ECALL, and then return that a syscall has been
     performed.  */
  readRegister (SIM_RISCV_PC_REGNUM, stepAddr);
  read (stepAddr,
        reinterpret_cast <uint8_t *> (&insn),
        sizeof (insn));
  if (insn == 0x00000073 /* ECALL */)
    {
      writeRegister (SIM_RISCV_PC_REGNUM, (stepAddr + 4));
      return ITarget::ResumeRes::SYSCALL;
    }

  sim_resume (gdbsim_desc, 1, 0 /* No signal.  */);
  sim_stop_reason (gdbsim_desc, &stop_reason, &signo);

  switch (stop_reason)
    {
    case sim_stopped:
      /* This is the common case.  */
      if (signo == GDB_SIGNAL_TRAP)
        {
          /* If we stopped looking at either C.EBREAK or EBREAK then we
             have hit a breakpoint.  Return an appropriate reply.  */
          read (stepAddr,
                reinterpret_cast <uint8_t *> (&cinsn),
                sizeof (cinsn));
          if (cinsn == 0x9002 /* C.EBREAK */)
            return ITarget::ResumeRes::INTERRUPTED;

          read (stepAddr,
                reinterpret_cast <uint8_t *> (&insn),
                sizeof (insn));
          if (insn == 0x00100073 /* EBREAK */)
            return ITarget::ResumeRes::INTERRUPTED;

          /* We must have just completed a step.  */
          return ITarget::ResumeRes::STEPPED;
        }
      else
        {
          std::cerr << "Unexpected signal " << std::dec << signo
                    << " from simulator" << std::endl;
          return ITarget::ResumeRes::INTERRUPTED;
        }

      break;

    case sim_signalled:
      /* Simulator was terminated with a signal.  There's currently no way
         to pass the signal number back out to the gdbserver code.  */
      std::cerr << "Simulator terminated with signal "
                << signo << std::endl;
      break;

    case sim_exited:
      /* Simulator exited.  */
      return ITarget::ResumeRes::SYSCALL;

    default:
    case sim_running:
    case sim_polling:
      /* These should not happen.  */
      std::cerr << "Error, unexpected simulator stop, reason = "
                << stop_reason << ", signal = " << signo << std::endl;
      break;
    }

  std::cerr << "Invalid simulator stop" << std::endl;
  abort ();
  return ITarget::ResumeRes::FAILURE;
}


ITarget::ResumeRes
GdbSimImpl::doRunToBreak (std::chrono::duration <double> timeout)
{
  bool haveTimeout = std::chrono::duration <double>::zero() != timeout;
  std::chrono::time_point <std::chrono::system_clock,
                           std::chrono::duration <double> > timeout_end;

  if (haveTimeout)
    timeout_end = std::chrono::system_clock::now () + timeout;

  do
    {
      // Step without a timeout.
      ITarget::ResumeRes res
        = doOneStep (std::chrono::duration <double>::zero ());

      // If the result is anything other than "step completed" then we're
      // done.
      if (res != ITarget::ResumeRes::STEPPED)
        return res;

      // Have we been running too long?
      if (haveTimeout && (std::chrono::system_clock::now () > timeout_end))
        return ITarget::ResumeRes::TIMEOUT;
    }
  while (true);
}

// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
