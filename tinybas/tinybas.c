/*
 * Tiny BASIC
 * Interpreter and Compiler Main Program
 *
 * Released as Public Domain by Damian Gareth Walker 2019
 * Created: 04-Aug-2019
 */


/* included headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "options.h"
#include "errors.h"
#include "parser.h"
#include "statement.h"
#include "interpret.h"
#include "formatter.h"
#include "generatec.h"

/* static variables */
static char *input_filename = NULL; /* name of the input file */
static enum { /* action to take with parsed program */
  OUTPUT_INTERPRET, /* interpret the program */
  OUTPUT_LST, /* output a formatted listing */
  OUTPUT_C, /* output a C program */
} output = OUTPUT_INTERPRET;
static ErrorHandler *errors; /* universal error handler */
static LanguageOptions *loptions; /* language options */
static int help = 0; /* true if usage was requested */


/*
 * Level 2 Routines
 */


/*
 * Case-insensitive test that an option value is a prefix of a keyword
 * params:
 *   char*   word     the full keyword, in lower case
 *   char*   option   the value supplied on the command line
 * returns:
 *   int              true if the value matches
 */
static int option_matches (char *word, char *option) {
  while (*option) {
    if (! *word || tolower (*option) != *word)
      return 0;
    ++word;
    ++option;
  }
  return 1;
}


/*
 * Print the usage message, with details of the options
 */
static void print_usage (void) {
  fprintf (stderr, "INF: Usage: tinybas [OPTIONS] INPUT-FILE\n");
  fprintf (stderr, "INF: Options are not case sensitive:\n");
  fprintf (stderr, "INF:   -nMODE    line numbers: optional (default), implied, mandatory\n");
  fprintf (stderr, "INF:   -lLIMIT   highest line number allowed (default 32767)\n");
  fprintf (stderr, "INF:   -cMODE    comments: enabled (default), disabled\n");
  fprintf (stderr, "INF:   -gLIMIT   maximum GOSUB nesting (default 64)\n");
  fprintf (stderr, "INF:   -oFORMAT  write the .lst (lst) or .c (c) file named after INPUT-FILE\n");
  fprintf (stderr, "INF:             instead of running the program\n");
  fprintf (stderr, "INF:   -h        show this help\n");
}

/*
 * Print the current error as an error message
 * params:
 *   char*   prefix   text to put in front of the error message
 */
static void print_error (char *prefix) {
  char *error_text; /* error text message */
  error_text = errors->get_text (errors);
  fprintf (stderr, "ERR: %s%s\n", prefix, error_text);
  free (error_text);
}

/*
 * Build an output filename by replacing the extension of the input filename
 * params:
 *   char*   extension   the new extension, without the dot
 * returns:
 *   char*               the new filename (caller frees), or NULL on error
 */
static char *output_name (char *extension) {
  char *name; /* the new filename */
  char *base; /* start of the filename part of the input path */
  char *dot; /* the extension separator in the input path */
  char *c; /* scan pointer */
  int length; /* length of the input name without its extension */

  /* find the filename part and the last dot within it */
  base = input_filename;
  dot = NULL;
  for (c = input_filename; *c; ++c) {
    if (*c == '/' || *c == '\\' || *c == ':') {
      base = c + 1;
      dot = NULL;
    } else if (*c == '.' && c > base)
      dot = c;
  }
  length = dot ? (int) (dot - input_filename) : (int) strlen (input_filename);

  name = malloc (length + strlen (extension) + 2);
  if (name) {
    strncpy (name, input_filename, length);
    name[length] = '\0';
    strcat (name, ".");
    strcat (name, extension);

    /* never overwrite the program being converted */
    for (c = name, base = input_filename; *c && tolower (*c) == tolower (*base);
      ++c, ++base)
      ;
    if (! *c && ! *base) {
      fprintf (stderr, "ERR: Output file %s would overwrite the input file\n",
        name);
      errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
      free (name);
      name = NULL;
    }
  }
  return name;
}

/*
 * Set line number option
 * params:
 *   char*   option   the option supplied on the command line
 */
static void set_line_numbers (char *option) {
  if (option_matches ("optional", option))
    loptions->set_line_numbers (loptions, LINE_NUMBERS_OPTIONAL);
  else if (option_matches ("implied", option))
    loptions->set_line_numbers (loptions, LINE_NUMBERS_IMPLIED);
  else if (option_matches ("mandatory", option))
    loptions->set_line_numbers (loptions, LINE_NUMBERS_MANDATORY);
  else
    errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
}

/*
 * Set line number limit
 * params:
 *   char*   option   the option supplied on the command line
 */
static void set_line_limit (char *option) {
  int limit; /* the limit contained in the option */
  if (sscanf (option, "%d", &limit))
    loptions->set_line_limit (loptions, limit);
  else
    errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
}

/*
 * Set comment option
 * params:
 *   char*   option   the option supplied on the command line
 */
static void set_comments (char *option) {
  if (option_matches ("enabled", option))
    loptions->set_comments (loptions, COMMENTS_ENABLED);
  else if (option_matches ("disabled", option))
    loptions->set_comments (loptions, COMMENTS_DISABLED);
  else
    errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
}

/*
 * Set the output options
 * params:
 *   char*   option   the option supplied on the command line
 */
static void set_output (char *option) {
  if (option_matches ("lst", option) && strlen (option) == 3)
    output = OUTPUT_LST;
  else if (option_matches ("c", option) && strlen (option) == 1)
    output = OUTPUT_C;
  else
    errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
}

/*
 * Set the GOSUB stack limit option
 * params:
 *   char*   option   the option supplied on the command line
 */
static void set_gosub_limit (char *option) {
  int limit; /* the limit contained in the option */
  if (sscanf (option, "%d", &limit))
    loptions->set_gosub_limit (loptions, limit);
  else
    errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
}


/*
 * Level 1 Routines
 */


/*
 * Process the command line options
 * params:
 *   int     argc   number of arguments on the command line
 *   char**  argv   the arguments
 */
static void set_options (int argc, char **argv) {

  /* local variables */
  int argn; /* argument number count */

  /* loop through all parameters */
  for (argn = 1; argn < argc && ! errors->get_code (errors); ++argn) {

    /* scan for options (case insensitive), e.g. -nimplied or -Olst */
    if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'n')
      set_line_numbers (&argv[argn][2]);
    else if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'l')
      set_line_limit (&argv[argn][2]);
    else if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'c')
      set_comments (&argv[argn][2]);
    else if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'o')
      set_output (&argv[argn][2]);
    else if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'g')
      set_gosub_limit (&argv[argn][2]);
    else if (argv[argn][0] == '-' && tolower (argv[argn][1]) == 'h')
      help = 1;

    /* accept filename */
    else if (! input_filename)
      input_filename = argv[argn];

    /* raise an error upon illegal option */
    else
      errors->set_code (errors, E_BAD_COMMAND_LINE, 0, 0);
  }
}

/*
 * Output a formatted program listing
 * params:
 *   ProgramNode*   program   the program to output
 */
static void output_lst (ProgramNode *program) {

  /* local variables */
  FILE *output; /* the output file */
  char *output_filename; /* the output filename */
  Formatter *formatter; /* the formatter object */

  /* ascertain the output filename */
  output_filename = output_name ("lst");
  if (output_filename) {

    /* open the output file */
    if ((output = fopen (output_filename, "w"))) {

      /* write to the output file */
      formatter = new_Formatter (errors);
      if (formatter) {
        formatter->generate (formatter, program);
        if (formatter->output)
          fprintf (output, "%s", formatter->output);
        formatter->destroy (formatter);
      }
      fclose (output);
    }

    /* deal with errors */
    else
      errors->set_code (errors, E_FILE_NOT_FOUND, 0, 0);

    /* free the output filename */
    free (output_filename);
  }

  /* deal with out of memory error */
  else if (! errors->get_code (errors))
    errors->set_code (errors, E_MEMORY, 0, 0);
}

/*
 * Output a C source file
 * params:
 *   ProgramNode*   program   the parsed program
 */
static void output_c (ProgramNode *program) {

  /* local variables */
  FILE *output; /* the output file */
  char *output_filename; /* the output filename */
  CProgram *c_program; /* the C program */

  /* open the output file */
  output_filename = output_name ("c");
  if (! output_filename) {
    if (! errors->get_code (errors))
      errors->set_code (errors, E_MEMORY, 0, 0);
    return;
  }
  if ((output = fopen (output_filename, "w"))) {

    /* write to the output file */
    c_program = new_CProgram (errors, loptions);
    if (c_program) {
      c_program->generate (c_program, program);
      if (c_program->c_output)
        fputs (c_program->c_output, output);
      if (c_program->c_code)
        fputs (c_program->c_code, output);
      if (c_program->c_tail)
        fputs (c_program->c_tail, output);
      c_program->destroy (c_program);
    }
    fclose (output);
  }

  /* deal with errors */
  else
    errors->set_code (errors, E_FILE_NOT_FOUND, 0, 0);

  /* clean up allocated memory */
  free (output_filename);
}

/*
 * Top Level Routine
 */


/*
 * Main Program
 * params:
 *   int     argc   number of arguments on the command line
 *   char**  argv   the arguments
 * returns:
 *   int            any error code from processing/running the program
 */
int main (int argc, char **argv) {

  /* local variables */
  FILE *input; /* input file */
  ProgramNode *program; /* the parsed program */
  ErrorCode code; /* error returned */
  Parser *parser; /* parser object */
  Interpreter *interpreter; /* interpreter object */
  char *command; /* command for compilation */

  /* interpret the command line arguments */
  errors = new_ErrorHandler ();
  loptions = new_LanguageOptions ();
  set_options (argc, argv);

  /* give usage if requested */
  if (help) {
    print_usage ();
    errors->destroy (errors);
    loptions->destroy (loptions);
    return 0;
  }

  /* report a bad command line */
  if ((code = errors->get_code (errors))) {
    print_error ("");
    print_usage ();
    errors->destroy (errors);
    loptions->destroy (loptions);
    return code;
  }

  /* give usage if filename not given */
  if (! input_filename) {
    print_usage ();
    errors->destroy (errors);
    loptions->destroy (loptions);
    return 0;
  }

  /* otherwise attempt to open the file */
  if (!(input = fopen (input_filename, "r"))) {
    fprintf (stderr, "ERR: Cannot open file %s\n", input_filename);
    errors->destroy (errors);
    loptions->destroy (loptions);
    return E_FILE_NOT_FOUND;
  }

  /* get the parse tree */
  parser = new_Parser (errors, loptions, input);
  program = parser->parse (parser);
  parser->destroy (parser);
  fclose (input);

  /* deal with errors */
  if ((code = errors->get_code (errors))) {
    print_error ("Parse error: ");
    loptions->destroy (loptions);
    errors->destroy (errors);
    return code;
  }

  /* perform the desired action */
  switch (output) {
    case OUTPUT_INTERPRET:
      interpreter = new_Interpreter (errors, loptions);
      interpreter->interpret (interpreter, program);
      interpreter->destroy (interpreter);
      if ((code = errors->get_code (errors))) {
        print_error ("Runtime error: ");
      }
      break;
    case OUTPUT_LST:
      output_lst (program);
      break;
    case OUTPUT_C:
      output_c (program);
      break;
  }

  /* report any error writing the output file */
  if (output != OUTPUT_INTERPRET && (code = errors->get_code (errors))) {
    print_error ("");
    program_destroy (program);
    loptions->destroy (loptions);
    errors->destroy (errors);
    return code;
  }

  /* clean up and return success */
  program_destroy (program);
  loptions->destroy (loptions);
  errors->destroy (errors);
  return 0;

}
