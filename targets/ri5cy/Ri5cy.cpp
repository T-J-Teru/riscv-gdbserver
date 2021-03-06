// GDB RSP server RI5CY CPU model wrapper: definition

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

#include "GdbServer.h"
#include "Ri5cy.h"
#include "Ri5cyImpl.h"
#include "TraceFlags.h"


//! Constructor.

//! We only instantiate the implementation class.

//! @param[in] wantVcd  TRUE if we want a VCD generated, false otherwise.

Ri5cy::Ri5cy (const TraceFlags * flags) :
  ITarget (flags)
{
  mRi5cyImpl = new Ri5cyImpl (flags);

}	// Ri5cy::Ri5cy ()


//! Destructor

//! Free the implementation class

Ri5cy::~Ri5cy ()
{
  delete mRi5cyImpl;

}	// Ri5cy::~Ri5cy ()


//! Resume execution

//! Wrapper for the implementation class

//! @param[in] step         Type of resumption required
//! @return The type of termination encountered.

ITarget::ResumeRes
Ri5cy::resume (ResumeType  step)
{
  return mRi5cyImpl->resume (step);

}	// Ri5cy::resume ()


//! Resume execution

//! Wrapper for the implementation class

//! @param[in] step         Type of resumption required
//! @param[in] timeout      Timeout requested
//! @return The type of termination encountered.

ITarget::ResumeRes
Ri5cy::resume (ResumeType  step,
	       std::chrono::duration <double>  timeout)
{
  return mRi5cyImpl->resume (step, timeout);

}	// Ri5cy::resume ()


//! Terminate execution

//! Wrapper for the implementation class.

//! @return  Result of termination

ITarget::ResumeRes
Ri5cy::terminate (void)
{
  return  mRi5cyImpl->terminate ();

}	// Ri5cy::terminate ()


//! Reset execution

//! Wrapper for the implementation class.

//! @return  Result of reset

ITarget::ResumeRes
Ri5cy::reset (ITarget::ResetType  type)
{
  return mRi5cyImpl->reset (type);

}	// Ri5cy::reset ()


//! Get count of cycles since startup or reset

//! Wrapper for the implementation class.

//! @return  Cycle count since startup or reset

uint64_t
Ri5cy::getCycleCount (void) const
{
  return mRi5cyImpl->getCycleCount ();

}	// Ri5cy::getCycleCount ()


//! Get count of instructions since startup or reset

//! Wrapper for the implementation class.

//! @return  Instruction count since startup or reset

uint64_t
Ri5cy::getInstrCount (void) const
{
  return mRi5cyImpl->getInstrCount ();

}	// Ri5cy::getInstrCount ()


//! Read a register

//! Wrapper for the implementation class.

//! @param[in] reg     Register to read
//! @param[out] value  Where to put the result of the read
//! @return  Number of bytes read into the register

std::size_t
Ri5cy::readRegister (const int  reg,
		     uint_reg_t & value) const
{
  return mRi5cyImpl->readRegister (reg, value);

}	// Ri5cy::readRegister ()


//! Write a register

//! Wrapper for the implementation class.

//! @param[in] reg    Register to write
//! @param[in] value  Value to write
//! @return  Number of bytes written into the register

std::size_t
Ri5cy::writeRegister (const int  reg,
		      const uint_reg_t  value)
{
  return mRi5cyImpl->writeRegister (reg, value);

}	// Ri5cy::writeRegister ()


//! Read from memory

//! Wrapper for the implementation class.

//! @param[in]  addr    Address to read
//! @param[out] buffer  Where to put the result of the read
//! @param[in]  size    Number of bytes to read
//! @return  Number of bytes read

std::size_t
Ri5cy::read (const uint32_t addr,
	     uint8_t * buffer,
	     const std::size_t  size) const
{
  return mRi5cyImpl->read (addr, buffer, size);

}	// Ri5cy::read ()


//! Write to memory

//! Wrapper for the implementation class.

//! @param[in]  addr    Address to write
//! @param[in]  buffer  Data to write
//! @param[in]  size    Number of bytes to write
//! @return  Number of bytes written

std::size_t
Ri5cy::write (const uint32_t  addr,
	      const uint8_t * buffer,
	      const std::size_t size)
{
  return mRi5cyImpl->write (addr, buffer, size);

}	// Ri5cy::write ()


//! Insert a matchpoint

//! Wrapper for the implementation class.

//! @param[in] addr  Address at which to set the matchpoint
//! @param[in] matchType  Type of matchpoint (breakpoint or watchpoint)
//! @return  TRUE if watchpoint set, FALSE otherwise.

bool
Ri5cy::insertMatchpoint (const uint32_t  addr,
			 const MatchType  matchType)
{
  return mRi5cyImpl->insertMatchpoint (addr, matchType);

}	// Ri5cy::insertMatchpoint ()


//! Remove a matchpoint

//! Wrapper for the implementation class.

//! @param[in] addr  Address from which to clear the matchpoint
//! @param[in] matchType  Type of matchpoint (breakpoint or watchpoint)
//! @return  TRUE if watchpoint set, FALSE otherwise.

bool
Ri5cy::removeMatchpoint (const uint32_t  addr,
			 const MatchType matchType)
{
  return mRi5cyImpl->removeMatchpoint (addr, matchType);

}	// Ri5cy::removeMatchpoint ()


//! Pass a command through to the target

//! Wrapper for the implementation class.

//! @param[in]  cmd  The command being passed
//! @param[out] stream  Stream on which to write any response.
//! @return  TRUE if the command succeeded, FALSE otherwise.

bool
Ri5cy::command (const std::string  cmd,
		std::ostream & stream)
{
  return  mRi5cyImpl->command (cmd, stream);

}	// Ri5cy::command ()


//! Wrapper for the implementation class

//! @param[in] server  The server to use

void
Ri5cy::gdbServer (GdbServer * server)
{
  mRi5cyImpl->gdbServer (server);

}	// Ri5cy::gdbServer ()


//! Return a timestamp.

//! This is needed to support the $time function in Verilog.  This in turn is
//! needed for VCD output.

//! Pass through to the implementation class.

//! @return  The current simulation time in seconds.

double
Ri5cy::timeStamp ()
{
  return mRi5cyImpl->timeStamp ();

}	// Ri5cy::timeStamp ()


// Local Variables:
// mode: C++
// c-file-style: "gnu"
// show-trailing-whitespace: t
// End:
