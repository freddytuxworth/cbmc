/*******************************************************************\

Module: Interface for returning Goto Traces from Goto Checkers

Author: Daniel Kroening, Peter Schrammel

\*******************************************************************/

/// \file
/// Interface for returning Goto Traces from Goto Checkers

#ifndef CPROVER_GOTO_CHECKER_GOTO_TRACE_PROVIDER_H
#define CPROVER_GOTO_CHECKER_GOTO_TRACE_PROVIDER_H

#include <util/irep.h>

class goto_tracet;
class namespacet;

/// An implementation of `incremental_goto_checkert`
/// may implement this interface to provide goto traces.
class goto_trace_providert
{
public:
  /// Builds and returns the complete trace
  virtual goto_tracet build_full_trace() const = 0;

  /// Builds and returns the trace up to the first failed property
  virtual goto_tracet build_shortest_trace() const = 0;

  /// Builds and returns the trace for the FAILed property
  /// with the given \p property_id
  virtual goto_tracet build_trace(const irep_idt &property_id) const = 0;

  /// Returns the namespace associated with the traces
  virtual const namespacet &get_namespace() const = 0;

  virtual ~goto_trace_providert() = default;
};

#endif // CPROVER_GOTO_CHECKER_GOTO_TRACE_PROVIDER_H
