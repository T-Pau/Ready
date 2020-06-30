/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 25 "debugger/commandy.y" /* yacc.c:339  */


#include <config.h>

#include <stdio.h>		/* Needed by NetBSD yacc */
#include <stdlib.h>
#include <string.h>

#include "debugger/debugger.h"
#include "debugger/debugger_internals.h"
#include "mempool.h"
#include "ui/ui.h"
#include "z80/z80.h"
#include "z80/z80_macros.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE


#line 86 "debugger/commandy.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_DEBUGGER_COMMANDY_H_INCLUDED
# define YY_YY_DEBUGGER_COMMANDY_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    LOGICAL_OR = 258,
    LOGICAL_AND = 259,
    COMPARISON = 260,
    EQUALITY = 261,
    NEGATE = 262,
    BASE = 263,
    BREAK = 264,
    TBREAK = 265,
    CLEAR = 266,
    COMMANDS = 267,
    CONDITION = 268,
    CONTINUE = 269,
    DEBUGGER_DELETE = 270,
    DISASSEMBLE = 271,
    DEBUGGER_END = 272,
    EVENT = 273,
    EXIT = 274,
    FINISH = 275,
    IF = 276,
    DEBUGGER_IGNORE = 277,
    NEXT = 278,
    DEBUGGER_OUT = 279,
    PORT = 280,
    DEBUGGER_PRINT = 281,
    READ = 282,
    SET = 283,
    STEP = 284,
    TIME = 285,
    WRITE = 286,
    NUMBER = 287,
    STRING = 288,
    VARIABLE = 289,
    DEBUGGER_ERROR = 290
  };
#endif
/* Tokens.  */
#define LOGICAL_OR 258
#define LOGICAL_AND 259
#define COMPARISON 260
#define EQUALITY 261
#define NEGATE 262
#define BASE 263
#define BREAK 264
#define TBREAK 265
#define CLEAR 266
#define COMMANDS 267
#define CONDITION 268
#define CONTINUE 269
#define DEBUGGER_DELETE 270
#define DISASSEMBLE 271
#define DEBUGGER_END 272
#define EVENT 273
#define EXIT 274
#define FINISH 275
#define IF 276
#define DEBUGGER_IGNORE 277
#define NEXT 278
#define DEBUGGER_OUT 279
#define PORT 280
#define DEBUGGER_PRINT 281
#define READ 282
#define SET 283
#define STEP 284
#define TIME 285
#define WRITE 286
#define NUMBER 287
#define STRING 288
#define VARIABLE 289
#define DEBUGGER_ERROR 290

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 45 "debugger/commandy.y" /* yacc.c:355  */


  int token;

  libspectrum_dword integer;
  char *string;

  debugger_breakpoint_type bptype;
  debugger_breakpoint_life bplife;
  struct { libspectrum_word mask, value; } port;
  struct { int source; int page; int offset; } location;

  debugger_expression* exp;


#line 212 "debugger/commandy.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_DEBUGGER_COMMANDY_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 227 "debugger/commandy.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  46
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   265

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  49
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  15
/* YYNRULES -- Number of rules.  */
#define YYNRULES  68
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  128

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      43,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    38,     2,
      45,    46,    41,    39,     2,    40,     2,    42,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    44,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    47,     2,    48,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    36,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   142,   142,   143,   144,   145,   148,   149,   153,   158,
     162,   166,   170,   171,   172,   175,   176,   177,   178,   179,
     180,   181,   184,   185,   186,   187,   188,   189,   190,   193,
     194,   197,   198,   199,   202,   203,   206,   207,   219,   220,
     223,   224,   227,   228,   231,   232,   235,   238,   241,   244,
     247,   248,   252,   253,   257,   261,   265,   269,   273,   277,
     281,   285,   289,   293,   297,   303,   311,   312,   317
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "LOGICAL_OR", "LOGICAL_AND",
  "COMPARISON", "EQUALITY", "NEGATE", "BASE", "BREAK", "TBREAK", "CLEAR",
  "COMMANDS", "CONDITION", "CONTINUE", "DEBUGGER_DELETE", "DISASSEMBLE",
  "DEBUGGER_END", "EVENT", "EXIT", "FINISH", "IF", "DEBUGGER_IGNORE",
  "NEXT", "DEBUGGER_OUT", "PORT", "DEBUGGER_PRINT", "READ", "SET", "STEP",
  "TIME", "WRITE", "NUMBER", "STRING", "VARIABLE", "DEBUGGER_ERROR", "'|'",
  "'^'", "'&'", "'+'", "'-'", "'*'", "'/'", "'\\n'", "':'", "'('", "')'",
  "'['", "']'", "$accept", "input", "command", "breakpointlife",
  "breakpointtype", "breakpointport", "breakpointlocation",
  "portbreakpointtype", "optionalcondition", "numberorpc",
  "expressionornull", "number", "expression", "debuggercommands",
  "debuggercommand", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   124,    94,    38,    43,
      45,    42,    47,    10,    58,    40,    41,    91,    93
};
# endif

#define YYPACT_NINF -31

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-31)))

#define YYTABLE_NINF -3

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      77,   -31,   121,   -31,   -31,   121,   121,   -23,   -31,   121,
     121,   121,   -31,   -17,   -31,   121,   121,     0,   -31,     2,
     -31,   234,   121,   -31,   -24,   -31,   121,   121,   121,   121,
     -31,   174,   -31,   -31,   -16,   121,   -31,   -31,   -31,   174,
     121,    -8,   -31,   121,   -18,   121,   -31,   227,     4,   -21,
     -31,   121,   -31,   124,   -31,     6,   -31,   -31,   106,    25,
     121,   121,   121,   121,   121,   121,   121,   121,   121,   121,
     121,     7,   -31,   -31,   -31,   -31,     9,   -31,   -31,    31,
     -31,   -31,   121,    23,    32,    23,   -31,   -31,   -31,   -31,
     186,   167,    98,    74,   192,    30,    42,   -19,   -19,   -31,
     -31,     3,   -12,   -31,   121,   -30,    23,    50,   121,   -31,
     142,   -31,   -31,   -31,   -31,   -31,    23,    23,   -31,   121,
     174,   -24,    51,   -31,   -31,   -31,   121,   -31
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     4,     0,    29,    30,    42,     0,     0,    15,    16,
       0,    44,    20,     0,    22,     0,     0,     0,    28,     0,
       3,    31,     0,    47,     0,    49,     0,     0,     0,     0,
       6,    46,    12,    43,     0,    44,    17,    18,    19,    45,
       0,     0,    24,     0,     0,     0,     1,     0,     0,     0,
      32,     0,    33,    42,    54,     0,    52,    53,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    14,    21,    23,    25,     0,    26,     5,     0,
      38,    39,     0,    40,     0,    40,    36,    48,    50,    51,
      65,    64,    60,    59,    63,    62,    61,    55,    56,    57,
      58,     0,     0,    66,     0,     0,    40,    34,     0,     9,
       0,     7,    68,    13,    67,    27,    40,    40,     8,     0,
      41,    48,     0,    10,    11,    35,     0,    37
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -31,   -31,    27,   -31,   -31,   -31,   -31,   -31,    19,    54,
      83,    -2,   -10,   -31,    17
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    19,    20,    21,    53,   106,    85,    82,   109,    32,
      38,    33,    31,   102,   103
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int8 yytable[] =
{
      30,    39,    46,   116,    34,   113,    80,    36,    37,    35,
      81,   117,    54,    41,    42,    40,    56,    57,    58,    59,
      55,   101,    69,    70,    74,    39,    76,    71,    60,    61,
      62,    63,    43,    44,    45,    62,    63,    79,    73,    87,
     101,    75,   104,    77,   108,    47,   112,    62,    63,    83,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,    64,    65,    66,    67,    68,    69,    70,    66,    67,
      68,    69,    70,    89,    78,   105,   110,    -2,     1,    62,
     107,    67,    68,    69,    70,     2,     3,     4,     5,     6,
       7,     8,     9,    10,   119,   126,    11,    12,   120,    13,
      14,    15,   115,    16,   111,    17,    18,    86,   122,    60,
      61,    62,    63,    67,    68,    69,    70,   125,    72,   114,
      -2,     0,     0,     0,   127,   118,     0,     0,    22,     0,
       0,    22,     0,     0,     0,   123,   124,    67,    68,    69,
      70,     0,    64,    65,    66,    67,    68,    69,    70,    22,
       0,     0,    88,    23,    24,    25,    23,    84,    25,     0,
      26,    27,     0,    26,    27,     0,    28,     0,    29,    28,
       0,    29,    62,    63,    23,   121,    25,    60,    61,    62,
      63,    26,    27,     0,     0,     0,     0,    28,     0,    29,
      61,    62,    63,     0,     0,     0,     0,    62,    63,     0,
       0,     0,     0,    64,    65,    66,    67,    68,    69,    70,
      64,    65,    66,    67,    68,    69,    70,     0,     0,     0,
       0,     0,    64,    65,    66,    67,    68,    69,    70,    65,
      66,    67,    68,    69,    70,     2,     3,     4,     5,     6,
       7,     8,     9,    10,     0,     0,    11,    12,     0,    13,
      14,    15,    48,    16,     0,    17,    18,     0,     0,    49,
       0,    50,     0,     0,    51,    52
};

static const yytype_int8 yycheck[] =
{
       2,    11,     0,    33,     6,    17,    27,     9,    10,    32,
      31,    41,    22,    15,    16,    32,    26,    27,    28,    29,
      44,    33,    41,    42,    32,    35,    44,    43,     3,     4,
       5,     6,    32,    33,    34,     5,     6,    33,    40,    33,
      33,    43,    33,    45,    21,    43,    43,     5,     6,    51,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    36,    37,    38,    39,    40,    41,    42,    38,    39,
      40,    41,    42,    48,    47,    44,    44,     0,     1,     5,
      82,    39,    40,    41,    42,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    44,    44,    19,    20,   108,    22,
      23,    24,   104,    26,    85,    28,    29,    53,   110,     3,
       4,     5,     6,    39,    40,    41,    42,   119,    35,   102,
      43,    -1,    -1,    -1,   126,   106,    -1,    -1,     7,    -1,
      -1,     7,    -1,    -1,    -1,   116,   117,    39,    40,    41,
      42,    -1,    36,    37,    38,    39,    40,    41,    42,     7,
      -1,    -1,    46,    32,    33,    34,    32,    33,    34,    -1,
      39,    40,    -1,    39,    40,    -1,    45,    -1,    47,    45,
      -1,    47,     5,     6,    32,    33,    34,     3,     4,     5,
       6,    39,    40,    -1,    -1,    -1,    -1,    45,    -1,    47,
       4,     5,     6,    -1,    -1,    -1,    -1,     5,     6,    -1,
      -1,    -1,    -1,    36,    37,    38,    39,    40,    41,    42,
      36,    37,    38,    39,    40,    41,    42,    -1,    -1,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    37,
      38,    39,    40,    41,    42,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    -1,    -1,    19,    20,    -1,    22,
      23,    24,    18,    26,    -1,    28,    29,    -1,    -1,    25,
      -1,    27,    -1,    -1,    30,    31
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    19,    20,    22,    23,    24,    26,    28,    29,    50,
      51,    52,     7,    32,    33,    34,    39,    40,    45,    47,
      60,    61,    58,    60,    60,    32,    60,    60,    59,    61,
      32,    60,    60,    32,    33,    34,     0,    43,    18,    25,
      27,    30,    31,    53,    61,    44,    61,    61,    61,    61,
       3,     4,     5,     6,    36,    37,    38,    39,    40,    41,
      42,    43,    59,    60,    32,    60,    44,    60,    51,    33,
      27,    31,    56,    60,    33,    55,    58,    33,    46,    48,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      61,    33,    62,    63,    33,    44,    54,    60,    21,    57,
      44,    57,    43,    17,    63,    60,    33,    41,    57,    44,
      61,    33,    60,    57,    57,    60,    44,    60
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    49,    50,    50,    50,    50,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    51,
      51,    51,    51,    51,    51,    51,    51,    51,    51,    52,
      52,    53,    53,    53,    54,    54,    55,    55,    56,    56,
      57,    57,    58,    58,    59,    59,    60,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    62,    62,    63
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     1,     1,     3,     2,     4,     5,     4,
       6,     6,     2,     5,     3,     1,     1,     2,     2,     2,
       1,     3,     1,     3,     2,     3,     3,     5,     1,     1,
       1,     0,     1,     1,     1,     3,     1,     5,     1,     1,
       0,     2,     0,     1,     0,     1,     1,     1,     3,     1,
       3,     3,     2,     2,     2,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     2,     2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 6:
#line 148 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_output_base = (yyvsp[0].integer); }
#line 1418 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 7:
#line 149 "debugger/commandy.y" /* yacc.c:1646  */
    {
             debugger_breakpoint_add_address( (yyvsp[-2].bptype), (yyvsp[-1].location).source, (yyvsp[-1].location).page, (yyvsp[-1].location).offset,
                                              0, (yyvsp[-3].bplife), (yyvsp[0].exp) );
	   }
#line 1427 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 8:
#line 153 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     int mask = (yyvsp[-1].port).mask;
	     if( mask == 0 ) mask = ( (yyvsp[-1].port).value < 0x100 ? 0x00ff : 0xffff );
	     debugger_breakpoint_add_port( (yyvsp[-2].bptype), (yyvsp[-1].port).value, mask, 0, (yyvsp[-4].bplife), (yyvsp[0].exp) );
           }
#line 1437 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 9:
#line 158 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     debugger_breakpoint_add_time( DEBUGGER_BREAKPOINT_TYPE_TIME,
					   (yyvsp[-1].integer), 0, (yyvsp[-3].bplife), (yyvsp[0].exp) );
	   }
#line 1446 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 10:
#line 162 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     debugger_breakpoint_add_event( DEBUGGER_BREAKPOINT_TYPE_EVENT,
					    (yyvsp[-3].string), (yyvsp[-1].string), 0, (yyvsp[-5].bplife), (yyvsp[0].exp) );
	   }
#line 1455 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 11:
#line 166 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     debugger_breakpoint_add_event( DEBUGGER_BREAKPOINT_TYPE_EVENT,
					    (yyvsp[-3].string), "*", 0, (yyvsp[-5].bplife), (yyvsp[0].exp) );
	   }
#line 1464 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 12:
#line 170 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_breakpoint_clear( (yyvsp[0].integer) ); }
#line 1470 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 13:
#line 171 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_breakpoint_set_commands( (yyvsp[-3].integer), (yyvsp[-1].string) ); }
#line 1476 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 14:
#line 172 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     debugger_breakpoint_set_condition( (yyvsp[-1].integer), (yyvsp[0].exp) );
           }
#line 1484 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 15:
#line 175 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_run(); }
#line 1490 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 16:
#line 176 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_breakpoint_remove_all(); }
#line 1496 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 17:
#line 177 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_breakpoint_remove( (yyvsp[0].integer) ); }
#line 1502 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 18:
#line 178 "debugger/commandy.y" /* yacc.c:1646  */
    { ui_debugger_disassemble( (yyvsp[0].integer) ); }
#line 1508 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 19:
#line 179 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_exit_emulator( (yyvsp[0].exp) ); }
#line 1514 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 20:
#line 180 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_breakpoint_exit(); }
#line 1520 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 21:
#line 181 "debugger/commandy.y" /* yacc.c:1646  */
    {
	     debugger_breakpoint_ignore( (yyvsp[-1].integer), (yyvsp[0].integer) );
	   }
#line 1528 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 22:
#line 184 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_next(); }
#line 1534 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 23:
#line 185 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_port_write( (yyvsp[-1].integer), (yyvsp[0].integer) ); }
#line 1540 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 24:
#line 186 "debugger/commandy.y" /* yacc.c:1646  */
    { printf( "0x%x\n", (yyvsp[0].integer) ); }
#line 1546 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 25:
#line 187 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_poke( (yyvsp[-1].integer), (yyvsp[0].integer) ); }
#line 1552 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 26:
#line 188 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_variable_set( (yyvsp[-1].string), (yyvsp[0].integer) ); }
#line 1558 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 27:
#line 189 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_system_variable_set( (yyvsp[-3].string), (yyvsp[-1].string), (yyvsp[0].integer) ); }
#line 1564 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 28:
#line 190 "debugger/commandy.y" /* yacc.c:1646  */
    { debugger_step(); }
#line 1570 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 29:
#line 193 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bplife) = DEBUGGER_BREAKPOINT_LIFE_PERMANENT; }
#line 1576 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 30:
#line 194 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bplife) = DEBUGGER_BREAKPOINT_LIFE_ONESHOT; }
#line 1582 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 31:
#line 197 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bptype) = DEBUGGER_BREAKPOINT_TYPE_EXECUTE; }
#line 1588 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 32:
#line 198 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bptype) = DEBUGGER_BREAKPOINT_TYPE_READ; }
#line 1594 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 33:
#line 199 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bptype) = DEBUGGER_BREAKPOINT_TYPE_WRITE; }
#line 1600 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 34:
#line 202 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.port).mask = 0; (yyval.port).value = (yyvsp[0].integer); }
#line 1606 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 35:
#line 203 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.port).mask = (yyvsp[-2].integer); (yyval.port).value = (yyvsp[0].integer); }
#line 1612 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 36:
#line 206 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.location).source = memory_source_any; (yyval.location).offset = (yyvsp[0].integer); }
#line 1618 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 37:
#line 207 "debugger/commandy.y" /* yacc.c:1646  */
    {
                        (yyval.location).source = memory_source_find( (yyvsp[-4].string) );
                        if( (yyval.location).source == -1 ) {
                          char buffer[80];
                          snprintf( buffer, 80, "unknown memory source \"%s\"", (yyvsp[-4].string) );
                          yyerror( buffer );
                          YYERROR;
                        }
                        (yyval.location).page = (yyvsp[-2].integer);
                        (yyval.location).offset = (yyvsp[0].integer);
                      }
#line 1634 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 38:
#line 219 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bptype) = DEBUGGER_BREAKPOINT_TYPE_PORT_READ; }
#line 1640 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 39:
#line 220 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.bptype) = DEBUGGER_BREAKPOINT_TYPE_PORT_WRITE; }
#line 1646 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 40:
#line 223 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = NULL; }
#line 1652 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 41:
#line 224 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 1658 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 42:
#line 227 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.integer) = PC; }
#line 1664 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 43:
#line 228 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.integer) = (yyvsp[0].integer); }
#line 1670 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 44:
#line 231 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = NULL; }
#line 1676 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 45:
#line 232 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 1682 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 46:
#line 235 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.integer) = debugger_expression_evaluate( (yyvsp[0].exp) ); }
#line 1688 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 47:
#line 238 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = debugger_expression_new_number( (yyvsp[0].integer), debugger_memory_pool );
		       if( !(yyval.exp) ) YYABORT;
		     }
#line 1696 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 48:
#line 241 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = debugger_expression_new_system_variable( (yyvsp[-2].string), (yyvsp[0].string), debugger_memory_pool );
                                  if( !(yyval.exp) ) YYABORT;
                                }
#line 1704 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 49:
#line 244 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = debugger_expression_new_variable( (yyvsp[0].string), debugger_memory_pool );
			 if( !(yyval.exp) ) YYABORT;
		       }
#line 1712 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 50:
#line 247 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = (yyvsp[-1].exp); }
#line 1718 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 51:
#line 248 "debugger/commandy.y" /* yacc.c:1646  */
    {
                (yyval.exp) = debugger_expression_new_unaryop( DEBUGGER_TOKEN_DEREFERENCE, (yyvsp[-1].exp), debugger_memory_pool );
                if( !(yyval.exp) ) YYABORT;
              }
#line 1727 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 52:
#line 252 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.exp) = (yyvsp[0].exp); }
#line 1733 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 53:
#line 253 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_unaryop( '-', (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1742 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 54:
#line 257 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_unaryop( (yyvsp[-1].token), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1751 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 55:
#line 261 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '+', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1760 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 56:
#line 265 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '-', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1769 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 57:
#line 269 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '*', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1778 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 58:
#line 273 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '/', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1787 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 59:
#line 277 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( (yyvsp[-1].token), (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1796 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 60:
#line 281 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( (yyvsp[-1].token), (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1805 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 61:
#line 285 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '&', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1814 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 62:
#line 289 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '^', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1823 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 63:
#line 293 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop( '|', (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1832 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 64:
#line 297 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop(
		  DEBUGGER_TOKEN_LOGICAL_AND, (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool
                );
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1843 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 65:
#line 303 "debugger/commandy.y" /* yacc.c:1646  */
    {
	        (yyval.exp) = debugger_expression_new_binaryop(
		  DEBUGGER_TOKEN_LOGICAL_OR, (yyvsp[-2].exp), (yyvsp[0].exp), debugger_memory_pool
		);
		if( !(yyval.exp) ) YYABORT;
	      }
#line 1854 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 66:
#line 311 "debugger/commandy.y" /* yacc.c:1646  */
    { (yyval.string) = (yyvsp[0].string); }
#line 1860 "debugger/commandy.c" /* yacc.c:1646  */
    break;

  case 67:
#line 312 "debugger/commandy.y" /* yacc.c:1646  */
    {
                      (yyval.string) = mempool_new( debugger_memory_pool, char, strlen( (yyvsp[-1].string) ) + strlen( (yyvsp[0].string) ) + 2 );
                      sprintf( (yyval.string), "%s\n%s", (yyvsp[-1].string), (yyvsp[0].string) );
                    }
#line 1869 "debugger/commandy.c" /* yacc.c:1646  */
    break;


#line 1873 "debugger/commandy.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 319 "debugger/commandy.y" /* yacc.c:1906  */

