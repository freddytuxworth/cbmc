[CPROVER Manual TOC](../../)

## Goto Harness

This is a tool that can generate harnesses, that is functions that instrument
another function under analysis, by setting up an appropriate environment before
calling them.

This is useful when trying to analyse an isolated unit of a program without
having to manually construct an appropriate environment.

### Usage

We have two types of harnesses we can generate for now:

* The `memory-snapshot` harness. TODO: Daniel can document this.
* The `function-call` harness, which automatically synthesises an environment
without having any information about the program state.

It is used similarly to how `goto-instrument` is used by modifying an existing
GOTO binary, as produced by `goto-cc`.

### The function call harness generator

For the most basic use of the `function-call` harness generator, imagine that we
have the following C program:

``` C
// main.c

#include <assert.h>

int function_to_test(int some_argument)
{
  assert(some_argument == 0);
  return some_argument;
}
```

We first need to generate a GOTO binary.

```sh
$ goto-cc -o main.gb main.c
```

Then we can call `goto-harness` on the generated GOTO binary
to get a modified one that contains the harness function:

```sh
$ goto-harness \
--harness-function-name harness \
--harness-type call-function \
--function function_to_test \
main.gb \
main-with-harness.gb
```

The options we pass to `goto-harness` are:

* `harness-function-name` is the name of the function generated by the harness
  generator (this needs to be specified for all harness generators).
* `harness-type` is the type of harness to generate (`call-function` is the
  function-call harness generator)
* `function` is the name of the function that gets instrumented
* then we pass the input GOTO-binary and a name for the output GOTO binary.

What comes of this is a GOTO binary roughly corresponding to the following C
pseudocode:

```C
// modified_goto_binary.c

#include <assert.h>

int function_to_test(int some_argument)
{
  assert(some_argument == 0);
  return some_argument;
}

void harness(void)
{
  int argument = nondet_int();
  function_to_test(argument);
}
```

You can pass that modified GOTO binary to `cbmc` and select `harness` as the
entry function for analysis, or further modify it with `goto-harness` or
`goto-instrument` or other tools.

The example above demonstrates the most simple case, which is roughly the same
as the entry point `cbmc` automically generates for functions. However, the
`function-call` harness can also non-deterministically initialise global
variables, array and struct elements. Consider this more complicated example:

```C
// list_example.c
#include <assert.h>
#include <stdlib.h>

typedef struct linked_list linked_list;
struct linked_list {
    int value;
    linked_list *next;
};

linked_list *global_linked_list;

/// initialize all values in the global
/// list to 0
void initialize_global(void)
{
  for(linked_list *ll = global_linked_list;
    ll != NULL;
    ll = ll->next)
  {
    ll->value = 0;
  }
}

/// try to initialize all values in the linked list
/// to 0 - but this version contains two bugs,
/// as it won't work with nullpointer arguments
/// and it will also not initialize the last element
void initialize_other(linked_list *ll)
{
    do {
        ll->value = 0;
        ll = ll->next;
    } while(ll->next != NULL);
}

void check_list(linked_list *list_parameter)
{
    assert(list_parameter != global_linked_list);
    initialize_global();
    initialize_other(list_parameter);
    linked_list *global_cursor = global_linked_list;
    linked_list *parameter_cursor = list_parameter;

    // global list should be a prefix of the parameter now,
    // or the other way round
    while(global_cursor != NULL && parameter_cursor != NULL)
    {
        // this assertion should fail since we didn't
        // initialize the last element of of the
        // list parameter correctly
        assert(global_cursor->value
          == parameter_cursor->value);
        global_cursor = global_cursor->next;
        parameter_cursor = parameter_cursor->next;
    }
}
```

Now we'll try to find the bug in `check_list` by generating a harness for it.

```sh
$ goto-cc -o list_example.gb list_example.c
$ goto-harness \
   --harness-function-name harness \
   --harness-type call-function \
   --function check_list \
   --max-nondet-tree-depth 4 \
   --min-null-tree-depth 1 \
   --nondet-globals \
   list_example.gb \
   list_example_with_harness.gb
$ cbmc --function harness list_example_with_harness.gb --unwind 20 --unwinding-assertions
```

We have 3 new options here:

* `max-nondet-tree-depth` is the maximum extent to which we will generate and
  initialize non-null pointers - in this case, this means generating lists up to
  length 2
* `min-null-tree-depth` this is the minimum depth at which pointers can be
  nullpointers for generated values - in this case, this sets the _minimum_
  length for our linked lists to one.
* `nondet-globals` is non-deterministically initialising global variables.

```
CBMC version 5.11 (cbmc-5.11-1523-g419a958-dirty) 64-bit x86_64 linux
[...]

** Results:
example.c function initialize_global
[initialize_global.unwind.0] line 17 unwinding assertion loop 0: SUCCESS

example.c function initialize_other
[initialize_other.unwind.0] line 32 unwinding assertion loop 0: SUCCESS

example.c function check_list
[check_list.assertion.1] line 42 assertion list_parameter != global_linked_list: SUCCESS
[check_list.unwind.0] line 50 unwinding assertion loop 0: SUCCESS
[check_list.assertion.2] line 55 assertion global_cursor->value == parameter_cursor->value: FAILURE

** 1 of 5 failed (2 iterations)
VERIFICATION FAILED
```

We also have support for arrays (currently only for array function parameters,
globals and struct members are considered for the future).

Take this example of an implementation of an `is_prefix_of` function that
should return true if the first string parameter `string` is a prefix of the
second one `prefix`.

```c
// array_example.c

int is_prefix_of(
  const char *string,
  int string_length,
  const char *prefix,
  int prefix_length
)
{
  if(prefix_length > string_length) { return 0; }
  // oops, we should have used prefix_length here
  for(int i = 0; i < string_length; ++i)
  {
    // we'll get an out of bounds error here!
    if(prefix[i] != string[i]) { return 0; }
  }
  return 1;
}
```

We can compile and run it like this:

```sh
$ goto-cc -o array_example.gb array_example.c
$ goto-harness \
  --harness-function-name harness \
  --harness-type call-function \
  --function is_prefix_of \
  --associated_array-size string:string_length \
  --associated-array-size prefix:prefix_length \
  array_example.gb array_example-mod.gb
$ cbmc --function harness --pointer-check array_example-mod.gb
```

We have the additional option `associated-array-size` here. This is in the format
`<array-parameter-name>:<array-size-parameter-name>` and will cause the array
parameter with name`array-parameter-name` to be initialised as an array, with
the parameter `array-size-parameter-name` holding its size (it should have
some integral type like `int` or `size_t`).

Running the example shows the bug highlighted in the comments:
```
[...]
[is_prefix_of.pointer_dereference.6] line 14 dereference failure: pointer outside object bounds in prefix[(signed long int)i]: FAILURE
```

By default, arrays are created with a minimum size of 1 and a maximum size of 2.
These limits can be set with the `--min-array-size` and `--max-array-size`
options.

If you have a function that takes an array parameter, but without an associated
size parameter, you can also use the `--treat-pointer-as-array <parameter-name>`
option.
