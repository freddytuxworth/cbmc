/******************************************************************\

Module: goto_harness_generator

Author: Diffblue Ltd.

\******************************************************************/

#ifndef CPROVER_GOTO_HARNESS_GOTO_HARNESS_GENERATOR_H
#define CPROVER_GOTO_HARNESS_GOTO_HARNESS_GENERATOR_H

#include <list>
#include <string>

#include <util/irep.h>

class goto_modelt;

class goto_harness_generatort
{
public:
  /// Generate a harness according to the set options
  virtual void
  generate(goto_modelt &goto_model, const irep_idt &harness_function_name) = 0;

  virtual ~goto_harness_generatort() = default;
  friend class goto_harness_generator_factoryt;

protected:
  /// Handle a command line argument. Should throw an exception if the option
  /// doesn't apply to this generator. Should only set options rather than
  /// immediately performing work
  virtual void handle_option(
    const std::string &option,
    const std::list<std::string> &values) = 0;

  /// Check if options are in a sane state, throw otherwise
  virtual void validate_options() = 0;

  /// Returns the only value of a single element list,
  /// throws an exception if not passed a single element list
  static std::string require_exactly_one_value(
    const std::string &option,
    const std::list<std::string> &values);

  /// Asserts that the list of values to an option passed is empty
  static void assert_no_values(
    const std::string &option,
    const std::list<std::string> &values);
};

#endif // CPROVER_GOTO_HARNESS_GOTO_HARNESS_GENERATOR_H
