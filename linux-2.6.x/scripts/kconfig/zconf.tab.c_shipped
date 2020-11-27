/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0

/* If NAME_PREFIX is specified substitute the variables and functions
   names.  */
#define yyparse zconfparse
#define yylex   zconflex
#define yyerror zconferror
#define yylval  zconflval
#define yychar  zconfchar
#define yydebug zconfdebug
#define yynerrs zconfnerrs


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_MAINMENU = 258,
     T_MENU = 259,
     T_ENDMENU = 260,
     T_SOURCE = 261,
     T_CHOICE = 262,
     T_ENDCHOICE = 263,
     T_COMMENT = 264,
     T_CONFIG = 265,
     T_MENUCONFIG = 266,
     T_HELP = 267,
     T_HELPTEXT = 268,
     T_IF = 269,
     T_ENDIF = 270,
     T_DEPENDS = 271,
     T_REQUIRES = 272,
     T_OPTIONAL = 273,
     T_PROMPT = 274,
     T_DEFAULT = 275,
     T_TRISTATE = 276,
     T_DEF_TRISTATE = 277,
     T_BOOLEAN = 278,
     T_DEF_BOOLEAN = 279,
     T_STRING = 280,
     T_INT = 281,
     T_HEX = 282,
     T_WORD = 283,
     T_WORD_QUOTE = 284,
     T_UNEQUAL = 285,
     T_EOF = 286,
     T_EOL = 287,
     T_CLOSE_PAREN = 288,
     T_OPEN_PAREN = 289,
     T_ON = 290,
     T_SELECT = 291,
     T_RANGE = 292,
     T_LOCK = 293,
     T_ENDLOCK = 294,
     T_OR = 295,
     T_AND = 296,
     T_EQUAL = 297,
     T_NOT = 298
   };
#endif
#define T_MAINMENU 258
#define T_MENU 259
#define T_ENDMENU 260
#define T_SOURCE 261
#define T_CHOICE 262
#define T_ENDCHOICE 263
#define T_COMMENT 264
#define T_CONFIG 265
#define T_MENUCONFIG 266
#define T_HELP 267
#define T_HELPTEXT 268
#define T_IF 269
#define T_ENDIF 270
#define T_DEPENDS 271
#define T_REQUIRES 272
#define T_OPTIONAL 273
#define T_PROMPT 274
#define T_DEFAULT 275
#define T_TRISTATE 276
#define T_DEF_TRISTATE 277
#define T_BOOLEAN 278
#define T_DEF_BOOLEAN 279
#define T_STRING 280
#define T_INT 281
#define T_HEX 282
#define T_WORD 283
#define T_WORD_QUOTE 284
#define T_UNEQUAL 285
#define T_EOF 286
#define T_EOL 287
#define T_CLOSE_PAREN 288
#define T_OPEN_PAREN 289
#define T_ON 290
#define T_SELECT 291
#define T_RANGE 292
#define T_LOCK 293
#define T_ENDLOCK 294
#define T_OR 295
#define T_AND 296
#define T_EQUAL 297
#define T_NOT 298




/* Copy the first part of user declarations.  */
#line 1 "scripts/kconfig/zconf.y"

/*
 * Copyright (C) 2002 Roman Zippel <zippel@linux-m68k.org>
 * Copyright (C) 2006 Motorola
 * Released under the terms of the GNU GPL v2.0.
 *
 * Motorola  2006-Nov-16 - Add support for lock/endlock blocks.
 */

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define printd(mask, fmt...) if (cdebug & (mask)) printf(fmt)

#define PRINTD		0x0001
#define DEBUG_PARSE	0x0002

int cdebug = PRINTD;

extern int zconflex(void);
static void zconfprint(const char *err, ...);
static void zconferror(const char *err);
static bool zconf_endtoken(int token, int starttoken, int endtoken);

struct symbol *symbol_hash[257];

static struct menu *current_menu, *current_entry;
static int current_lock = 0;

#define YYERROR_VERBOSE


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 39 "scripts/kconfig/zconf.y"
typedef union YYSTYPE {
	int token;
	char *string;
	struct symbol *symbol;
	struct expr *expr;
	struct menu *menu;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 215 "scripts/kconfig/zconf.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */
#line 97 "scripts/kconfig/zconf.y"

#define LKC_DIRECT_LINK
#include "lkc.h"


/* Line 214 of yacc.c.  */
#line 231 "scripts/kconfig/zconf.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   235

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  44
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  45
/* YYNRULES -- Number of rules. */
#define YYNRULES  112
/* YYNRULES -- Number of states. */
#define YYNSTATES  193

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   298

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    11,    13,    17,    19,
      21,    23,    26,    28,    30,    32,    34,    36,    38,    40,
      44,    47,    51,    54,    55,    58,    61,    64,    67,    71,
      76,    80,    85,    89,    93,    97,   102,   107,   112,   118,
     121,   124,   126,   130,   133,   134,   137,   140,   143,   146,
     151,   155,   159,   162,   167,   168,   171,   175,   177,   181,
     184,   185,   188,   191,   194,   197,   200,   204,   205,   208,
     211,   214,   218,   221,   223,   227,   230,   231,   234,   237,
     240,   244,   248,   250,   254,   257,   260,   263,   264,   267,
     270,   275,   279,   283,   284,   287,   289,   291,   294,   297,
     300,   302,   304,   305,   308,   310,   314,   318,   322,   325,
     329,   333,   335
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      45,     0,    -1,    -1,    45,    46,    -1,    47,    -1,    57,
      -1,    72,    -1,     3,    83,    85,    -1,     5,    -1,    15,
      -1,     8,    -1,     1,    85,    -1,    63,    -1,    67,    -1,
      77,    -1,    49,    -1,    51,    -1,    75,    -1,    85,    -1,
      10,    28,    32,    -1,    48,    52,    -1,    11,    28,    32,
      -1,    50,    52,    -1,    -1,    52,    53,    -1,    52,    81,
      -1,    52,    79,    -1,    52,    32,    -1,    21,    82,    32,
      -1,    22,    87,    86,    32,    -1,    23,    82,    32,    -1,
      24,    87,    86,    32,    -1,    26,    82,    32,    -1,    27,
      82,    32,    -1,    25,    82,    32,    -1,    19,    83,    86,
      32,    -1,    20,    87,    86,    32,    -1,    36,    28,    86,
      32,    -1,    37,    88,    88,    86,    32,    -1,     7,    32,
      -1,    54,    58,    -1,    84,    -1,    55,    60,    56,    -1,
      55,    60,    -1,    -1,    58,    59,    -1,    58,    81,    -1,
      58,    79,    -1,    58,    32,    -1,    19,    83,    86,    32,
      -1,    21,    82,    32,    -1,    23,    82,    32,    -1,    18,
      32,    -1,    20,    28,    86,    32,    -1,    -1,    60,    47,
      -1,    14,    87,    32,    -1,    84,    -1,    61,    64,    62,
      -1,    61,    64,    -1,    -1,    64,    47,    -1,    64,    72,
      -1,    64,    57,    -1,    38,    32,    -1,    39,    32,    -1,
      65,    68,    66,    -1,    -1,    68,    47,    -1,    68,    72,
      -1,    68,    57,    -1,     4,    83,    32,    -1,    69,    80,
      -1,    84,    -1,    70,    73,    71,    -1,    70,    73,    -1,
      -1,    73,    47,    -1,    73,    72,    -1,    73,    57,    -1,
      73,     1,    32,    -1,     6,    83,    32,    -1,    74,    -1,
       9,    83,    32,    -1,    76,    80,    -1,    12,    32,    -1,
      78,    13,    -1,    -1,    80,    81,    -1,    80,    32,    -1,
      16,    35,    87,    32,    -1,    16,    87,    32,    -1,    17,
      87,    32,    -1,    -1,    83,    86,    -1,    28,    -1,    29,
      -1,     5,    85,    -1,     8,    85,    -1,    15,    85,    -1,
      32,    -1,    31,    -1,    -1,    14,    87,    -1,    88,    -1,
      88,    42,    88,    -1,    88,    30,    88,    -1,    34,    87,
      33,    -1,    43,    87,    -1,    87,    40,    87,    -1,    87,
      41,    87,    -1,    28,    -1,    29,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   102,   102,   103,   106,   107,   108,   109,   110,   111,
     112,   113,   117,   118,   119,   120,   121,   122,   123,   129,
     137,   143,   151,   161,   163,   164,   165,   166,   169,   175,
     182,   188,   195,   201,   207,   213,   219,   225,   231,   239,
     248,   254,   263,   264,   270,   272,   273,   274,   275,   278,
     284,   290,   296,   302,   308,   310,   315,   324,   333,   334,
     340,   342,   343,   344,   349,   354,   359,   362,   364,   365,
     366,   371,   378,   384,   393,   394,   400,   402,   403,   404,
     405,   408,   414,   421,   428,   435,   441,   448,   449,   450,
     453,   458,   463,   471,   473,   478,   479,   482,   483,   484,
     488,   488,   490,   491,   494,   495,   496,   497,   498,   499,
     500,   503,   504
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_MAINMENU", "T_MENU", "T_ENDMENU",
  "T_SOURCE", "T_CHOICE", "T_ENDCHOICE", "T_COMMENT", "T_CONFIG",
  "T_MENUCONFIG", "T_HELP", "T_HELPTEXT", "T_IF", "T_ENDIF", "T_DEPENDS",
  "T_REQUIRES", "T_OPTIONAL", "T_PROMPT", "T_DEFAULT", "T_TRISTATE",
  "T_DEF_TRISTATE", "T_BOOLEAN", "T_DEF_BOOLEAN", "T_STRING", "T_INT",
  "T_HEX", "T_WORD", "T_WORD_QUOTE", "T_UNEQUAL", "T_EOF", "T_EOL",
  "T_CLOSE_PAREN", "T_OPEN_PAREN", "T_ON", "T_SELECT", "T_RANGE", "T_LOCK",
  "T_ENDLOCK", "T_OR", "T_AND", "T_EQUAL", "T_NOT", "$accept", "input",
  "block", "common_block", "config_entry_start", "config_stmt",
  "menuconfig_entry_start", "menuconfig_stmt", "config_option_list",
  "config_option", "choice", "choice_entry", "choice_end", "choice_stmt",
  "choice_option_list", "choice_option", "choice_block", "if", "if_end",
  "if_stmt", "if_block", "lock_entry", "lock_end", "lock_stmt",
  "lock_block", "menu", "menu_entry", "menu_end", "menu_stmt",
  "menu_block", "source", "source_stmt", "comment", "comment_stmt",
  "help_start", "help", "depends_list", "depends", "prompt_stmt_opt",
  "prompt", "end", "nl_or_eof", "if_expr", "expr", "symbol", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    44,    45,    45,    46,    46,    46,    46,    46,    46,
      46,    46,    47,    47,    47,    47,    47,    47,    47,    48,
      49,    50,    51,    52,    52,    52,    52,    52,    53,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    54,
      55,    56,    57,    57,    58,    58,    58,    58,    58,    59,
      59,    59,    59,    59,    60,    60,    61,    62,    63,    63,
      64,    64,    64,    64,    65,    66,    67,    68,    68,    68,
      68,    69,    70,    71,    72,    72,    73,    73,    73,    73,
      73,    74,    75,    76,    77,    78,    79,    80,    80,    80,
      81,    81,    81,    82,    82,    83,    83,    84,    84,    84,
      85,    85,    86,    86,    87,    87,    87,    87,    87,    87,
      87,    88,    88
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     3,     1,     1,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     3,
       2,     3,     2,     0,     2,     2,     2,     2,     3,     4,
       3,     4,     3,     3,     3,     4,     4,     4,     5,     2,
       2,     1,     3,     2,     0,     2,     2,     2,     2,     4,
       3,     3,     2,     4,     0,     2,     3,     1,     3,     2,
       0,     2,     2,     2,     2,     2,     3,     0,     2,     2,
       2,     3,     2,     1,     3,     2,     0,     2,     2,     2,
       3,     3,     1,     3,     2,     2,     2,     0,     2,     2,
       4,     3,     3,     0,     2,     1,     1,     2,     2,     2,
       1,     1,     0,     2,     1,     3,     3,     3,     2,     3,
       3,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,     0,     0,     8,     0,     0,    10,
       0,     0,     0,     0,     9,   101,   100,     0,     3,     4,
      23,    15,    23,    16,    44,    54,     5,    60,    12,    67,
      13,    87,    76,     6,    82,    17,    87,    14,    18,    11,
      95,    96,     0,     0,     0,    39,     0,     0,     0,   111,
     112,     0,     0,     0,   104,    64,    20,    22,    40,    43,
      59,     0,    72,     0,    84,     7,    71,    81,    83,    19,
      21,     0,   108,    56,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    93,     0,    93,     0,    93,    93,    93,
      27,     0,     0,    24,     0,    26,    25,     0,     0,     0,
      93,    93,    48,    45,    47,    46,     0,     0,     0,    55,
      42,    41,    61,    63,    58,    62,    57,     0,    68,    70,
      66,    69,    89,    88,     0,    77,    79,    74,    78,    73,
     107,   109,   110,   106,   105,    85,     0,     0,     0,   102,
     102,     0,   102,   102,     0,   102,     0,     0,     0,   102,
       0,    86,    52,   102,   102,     0,     0,    97,    98,    99,
      65,    80,     0,    91,    92,     0,     0,     0,    28,    94,
       0,    30,     0,    34,    32,    33,     0,   102,     0,     0,
      50,    51,    90,   103,    35,    36,    29,    31,    37,     0,
      49,    53,    38
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     1,    18,    19,    20,    21,    22,    23,    56,    93,
      24,    25,   110,    26,    58,   103,    59,    27,   114,    28,
      60,    29,   120,    30,    61,    31,    32,   127,    33,    63,
      34,    35,    36,    37,    94,    95,    62,    96,   141,   142,
     111,    38,   166,    53,    54
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -63
static const short yypact[] =
{
     -63,    58,   -63,   -19,    23,    23,   -63,    23,   -23,   -63,
      23,   -12,     4,    -7,   -63,   -63,   -63,    25,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -19,    28,    47,   -63,    48,    55,    59,   -63,
     -63,    -7,    -7,   -15,   -22,   -63,   177,   177,   155,   154,
     123,   111,     2,    39,     2,   -63,   -63,   -63,   -63,   -63,
     -63,    70,   -63,   -63,    -7,    -7,    27,    27,    65,   192,
      -7,    23,    -7,    23,    -7,    23,    -7,    23,    23,    23,
     -63,    72,    27,   -63,    96,   -63,   -63,    84,    23,    91,
      23,    23,   -63,   -63,   -63,   -63,   -19,   -19,   -19,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   108,   -63,   -63,
     -63,   -63,   -63,   -63,   115,   -63,   -63,   -63,   -63,   -63,
     -63,   110,   -63,   -63,   -63,   -63,    -7,    42,    52,   138,
      -3,   121,   138,    -3,   124,    -3,   125,   126,   134,   138,
      27,   -63,   -63,   138,   138,   145,   149,   -63,   -63,   -63,
     -63,   -63,    54,   -63,   -63,    -7,   151,   152,   -63,   -63,
     156,   -63,   158,   -63,   -63,   -63,   159,   138,   163,   173,
     -63,   -63,   -63,    35,   -63,   -63,   -63,   -63,   -63,   174,
     -63,   -63,   -63
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -63,   -63,   -63,    85,   -63,   -63,   -63,   -63,   148,   -63,
     -63,   -63,   -63,    63,   -63,   -63,   -63,   -63,   -63,   -63,
     -63,   -63,   -63,   -63,   -63,   -63,   -63,   -63,   119,   -63,
     -63,   -63,   -63,   -63,   -63,   150,   171,    77,   130,     0,
     -57,    -1,   -41,   -51,   -62
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -76
static const short yytable[] =
{
      71,    72,    39,   116,    42,    43,   129,    44,    76,    45,
      46,   165,    15,    16,   133,   134,    47,    73,    79,    80,
      77,    49,    50,   131,   132,    74,    75,    51,   137,   138,
     150,   140,    48,   143,   122,   145,    52,    74,    75,   -75,
     124,    65,   -75,     5,   106,     7,     8,   107,    10,    11,
      12,    40,    41,    13,   108,    49,    50,    55,     2,     3,
      66,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      15,    16,    13,    14,   163,    74,    75,    17,   -75,    67,
      68,   139,    74,    75,   164,   162,   182,    69,   177,    15,
      16,    70,    74,    75,    74,    75,    17,   135,   153,   167,
     149,   169,   170,   130,   172,   157,   158,   159,   176,   151,
      74,    75,   178,   179,   183,     5,   152,     7,     8,   154,
      10,    11,    12,   113,   119,    13,   126,     5,   106,     7,
       8,   107,    10,    11,    12,   105,   189,    13,   108,   123,
     160,   123,    15,    16,   109,   112,   118,   161,   125,    17,
     117,    75,   165,   168,    15,    16,   171,   173,   174,   106,
       7,    17,   107,    10,    11,    12,   175,    78,    13,   108,
      57,    79,    80,    97,    98,    99,   100,   180,   101,   115,
     121,   181,   128,   184,   185,    15,    16,   102,   186,    78,
     187,   188,    17,    79,    80,   190,    81,    82,    83,    84,
      85,    86,    87,    88,    89,   191,   192,    64,   104,    90,
       0,     0,     0,    91,    92,   144,     0,   146,   147,   148,
      49,    50,     0,     0,     0,     0,    51,   136,     0,     0,
     155,   156,     0,     0,     0,    52
};

static const short yycheck[] =
{
      51,    52,     3,    60,     4,     5,    63,     7,    30,    32,
      10,    14,    31,    32,    76,    77,    28,    32,    16,    17,
      42,    28,    29,    74,    75,    40,    41,    34,    79,    80,
      92,    82,    28,    84,    32,    86,    43,    40,    41,     0,
       1,    42,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    28,    29,    14,    15,    28,    29,    32,     0,     1,
      32,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      31,    32,    14,    15,    32,    40,    41,    38,    39,    32,
      32,    81,    40,    41,    32,   136,    32,    32,   150,    31,
      32,    32,    40,    41,    40,    41,    38,    32,    98,   140,
      28,   142,   143,    33,   145,   106,   107,   108,   149,    13,
      40,    41,   153,   154,   165,     4,    32,     6,     7,    28,
       9,    10,    11,    60,    61,    14,    63,     4,     5,     6,
       7,     8,     9,    10,    11,    58,   177,    14,    15,    62,
      32,    64,    31,    32,    59,    60,    61,    32,    63,    38,
      39,    41,    14,    32,    31,    32,    32,    32,    32,     5,
       6,    38,     8,     9,    10,    11,    32,    12,    14,    15,
      22,    16,    17,    18,    19,    20,    21,    32,    23,    60,
      61,    32,    63,    32,    32,    31,    32,    32,    32,    12,
      32,    32,    38,    16,    17,    32,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    32,    32,    36,    58,    32,
      -1,    -1,    -1,    36,    37,    85,    -1,    87,    88,    89,
      28,    29,    -1,    -1,    -1,    -1,    34,    35,    -1,    -1,
     100,   101,    -1,    -1,    -1,    43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    45,     0,     1,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    14,    15,    31,    32,    38,    46,    47,
      48,    49,    50,    51,    54,    55,    57,    61,    63,    65,
      67,    69,    70,    72,    74,    75,    76,    77,    85,    85,
      28,    29,    83,    83,    83,    32,    83,    28,    28,    28,
      29,    34,    43,    87,    88,    32,    52,    52,    58,    60,
      64,    68,    80,    73,    80,    85,    32,    32,    32,    32,
      32,    87,    87,    32,    40,    41,    30,    42,    12,    16,
      17,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      32,    36,    37,    53,    78,    79,    81,    18,    19,    20,
      21,    23,    32,    59,    79,    81,     5,     8,    15,    47,
      56,    84,    47,    57,    62,    72,    84,    39,    47,    57,
      66,    72,    32,    81,     1,    47,    57,    71,    72,    84,
      33,    87,    87,    88,    88,    32,    35,    87,    87,    83,
      87,    82,    83,    87,    82,    87,    82,    82,    82,    28,
      88,    13,    32,    83,    28,    82,    82,    85,    85,    85,
      32,    32,    87,    32,    32,    14,    86,    86,    32,    86,
      86,    32,    86,    32,    32,    32,    86,    88,    86,    86,
      32,    32,    32,    87,    32,    32,    32,    32,    32,    86,
      32,    32,    32
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 8:
#line 110 "scripts/kconfig/zconf.y"
    { zconfprint("unexpected 'endmenu' statement"); ;}
    break;

  case 9:
#line 111 "scripts/kconfig/zconf.y"
    { zconfprint("unexpected 'endif' statement"); ;}
    break;

  case 10:
#line 112 "scripts/kconfig/zconf.y"
    { zconfprint("unexpected 'endchoice' statement"); ;}
    break;

  case 11:
#line 113 "scripts/kconfig/zconf.y"
    { zconfprint("syntax error"); yyerrok; ;}
    break;

  case 19:
#line 130 "scripts/kconfig/zconf.y"
    {
	struct symbol *sym = sym_lookup(yyvsp[-1].string, 0);
	sym->flags |= SYMBOL_OPTIONAL;
	menu_add_entry(sym);
	printd(DEBUG_PARSE, "%s:%d:config %s\n", zconf_curname(), zconf_lineno(), yyvsp[-1].string);
;}
    break;

  case 20:
#line 138 "scripts/kconfig/zconf.y"
    {
	menu_end_entry();
	printd(DEBUG_PARSE, "%s:%d:endconfig\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 21:
#line 144 "scripts/kconfig/zconf.y"
    {
	struct symbol *sym = sym_lookup(yyvsp[-1].string, 0);
	sym->flags |= SYMBOL_OPTIONAL;
	menu_add_entry(sym);
	printd(DEBUG_PARSE, "%s:%d:menuconfig %s\n", zconf_curname(), zconf_lineno(), yyvsp[-1].string);
;}
    break;

  case 22:
#line 152 "scripts/kconfig/zconf.y"
    {
	if (current_entry->prompt)
		current_entry->prompt->type = P_MENU;
	else
		zconfprint("warning: menuconfig statement without prompt");
	menu_end_entry();
	printd(DEBUG_PARSE, "%s:%d:endconfig\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 28:
#line 170 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_TRISTATE);
	printd(DEBUG_PARSE, "%s:%d:tristate\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 29:
#line 176 "scripts/kconfig/zconf.y"
    {
	menu_add_expr(P_DEFAULT, yyvsp[-2].expr, yyvsp[-1].expr);
	menu_set_type(S_TRISTATE);
	printd(DEBUG_PARSE, "%s:%d:def_boolean\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 30:
#line 183 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_BOOLEAN);
	printd(DEBUG_PARSE, "%s:%d:boolean\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 31:
#line 189 "scripts/kconfig/zconf.y"
    {
	menu_add_expr(P_DEFAULT, yyvsp[-2].expr, yyvsp[-1].expr);
	menu_set_type(S_BOOLEAN);
	printd(DEBUG_PARSE, "%s:%d:def_boolean\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 32:
#line 196 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_INT);
	printd(DEBUG_PARSE, "%s:%d:int\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 33:
#line 202 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_HEX);
	printd(DEBUG_PARSE, "%s:%d:hex\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 34:
#line 208 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_STRING);
	printd(DEBUG_PARSE, "%s:%d:string\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 35:
#line 214 "scripts/kconfig/zconf.y"
    {
	menu_add_prompt(P_PROMPT, yyvsp[-2].string, yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:prompt\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 36:
#line 220 "scripts/kconfig/zconf.y"
    {
	menu_add_expr(P_DEFAULT, yyvsp[-2].expr, yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:default\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 37:
#line 226 "scripts/kconfig/zconf.y"
    {
	menu_add_symbol(P_SELECT, sym_lookup(yyvsp[-2].string, 0), yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:select\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 38:
#line 232 "scripts/kconfig/zconf.y"
    {
	menu_add_expr(P_RANGE, expr_alloc_comp(E_RANGE,yyvsp[-3].symbol, yyvsp[-2].symbol), yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:range\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 39:
#line 240 "scripts/kconfig/zconf.y"
    {
	struct symbol *sym = sym_lookup(NULL, 0);
	sym->flags |= SYMBOL_CHOICE;
	menu_add_entry(sym);
	menu_add_expr(P_CHOICE, NULL, NULL);
	printd(DEBUG_PARSE, "%s:%d:choice\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 40:
#line 249 "scripts/kconfig/zconf.y"
    {
	menu_end_entry();
	menu_add_menu();
;}
    break;

  case 41:
#line 255 "scripts/kconfig/zconf.y"
    {
	if (zconf_endtoken(yyvsp[0].token, T_CHOICE, T_ENDCHOICE)) {
		menu_end_menu();
		printd(DEBUG_PARSE, "%s:%d:endchoice\n", zconf_curname(), zconf_lineno());
	}
;}
    break;

  case 43:
#line 265 "scripts/kconfig/zconf.y"
    {
	printf("%s:%d: missing 'endchoice' for this 'choice' statement\n", current_menu->file->name, current_menu->lineno);
	zconfnerrs++;
;}
    break;

  case 49:
#line 279 "scripts/kconfig/zconf.y"
    {
	menu_add_prompt(P_PROMPT, yyvsp[-2].string, yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:prompt\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 50:
#line 285 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_TRISTATE);
	printd(DEBUG_PARSE, "%s:%d:tristate\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 51:
#line 291 "scripts/kconfig/zconf.y"
    {
	menu_set_type(S_BOOLEAN);
	printd(DEBUG_PARSE, "%s:%d:boolean\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 52:
#line 297 "scripts/kconfig/zconf.y"
    {
	current_entry->sym->flags |= SYMBOL_OPTIONAL;
	printd(DEBUG_PARSE, "%s:%d:optional\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 53:
#line 303 "scripts/kconfig/zconf.y"
    {
	menu_add_symbol(P_DEFAULT, sym_lookup(yyvsp[-2].string, 0), yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:default\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 56:
#line 316 "scripts/kconfig/zconf.y"
    {
	printd(DEBUG_PARSE, "%s:%d:if\n", zconf_curname(), zconf_lineno());
	menu_add_entry(NULL);
	menu_add_dep(yyvsp[-1].expr);
	menu_end_entry();
	menu_add_menu();
;}
    break;

  case 57:
#line 325 "scripts/kconfig/zconf.y"
    {
	if (zconf_endtoken(yyvsp[0].token, T_IF, T_ENDIF)) {
		menu_end_menu();
		printd(DEBUG_PARSE, "%s:%d:endif\n", zconf_curname(), zconf_lineno());
	}
;}
    break;

  case 59:
#line 335 "scripts/kconfig/zconf.y"
    {
	printf("%s:%d: missing 'endif' for this 'if' statement\n", current_menu->file->name, current_menu->lineno);
	zconfnerrs++;
;}
    break;

  case 64:
#line 350 "scripts/kconfig/zconf.y"
    {
	current_lock++;
;}
    break;

  case 65:
#line 355 "scripts/kconfig/zconf.y"
    {
	current_lock--;
;}
    break;

  case 71:
#line 372 "scripts/kconfig/zconf.y"
    {
	menu_add_entry(NULL);
	menu_add_prop(P_MENU, yyvsp[-1].string, NULL, NULL);
	printd(DEBUG_PARSE, "%s:%d:menu\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 72:
#line 379 "scripts/kconfig/zconf.y"
    {
	menu_end_entry();
	menu_add_menu();
;}
    break;

  case 73:
#line 385 "scripts/kconfig/zconf.y"
    {
	if (zconf_endtoken(yyvsp[0].token, T_MENU, T_ENDMENU)) {
		menu_end_menu();
		printd(DEBUG_PARSE, "%s:%d:endmenu\n", zconf_curname(), zconf_lineno());
	}
;}
    break;

  case 75:
#line 395 "scripts/kconfig/zconf.y"
    {
	printf("%s:%d: missing 'endmenu' for this 'menu' statement\n", current_menu->file->name, current_menu->lineno);
	zconfnerrs++;
;}
    break;

  case 80:
#line 405 "scripts/kconfig/zconf.y"
    { zconfprint("invalid menu option"); yyerrok; ;}
    break;

  case 81:
#line 409 "scripts/kconfig/zconf.y"
    {
	yyval.string = yyvsp[-1].string;
	printd(DEBUG_PARSE, "%s:%d:source %s\n", zconf_curname(), zconf_lineno(), yyvsp[-1].string);
;}
    break;

  case 82:
#line 415 "scripts/kconfig/zconf.y"
    {
	zconf_nextfile(yyvsp[0].string);
;}
    break;

  case 83:
#line 422 "scripts/kconfig/zconf.y"
    {
	menu_add_entry(NULL);
	menu_add_prop(P_COMMENT, yyvsp[-1].string, NULL, NULL);
	printd(DEBUG_PARSE, "%s:%d:comment\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 84:
#line 429 "scripts/kconfig/zconf.y"
    {
	menu_end_entry();
;}
    break;

  case 85:
#line 436 "scripts/kconfig/zconf.y"
    {
	printd(DEBUG_PARSE, "%s:%d:help\n", zconf_curname(), zconf_lineno());
	zconf_starthelp();
;}
    break;

  case 86:
#line 442 "scripts/kconfig/zconf.y"
    {
	current_entry->sym->help = yyvsp[0].string;
;}
    break;

  case 90:
#line 454 "scripts/kconfig/zconf.y"
    {
	menu_add_dep(yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:depends on\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 91:
#line 459 "scripts/kconfig/zconf.y"
    {
	menu_add_dep(yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:depends\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 92:
#line 464 "scripts/kconfig/zconf.y"
    {
	menu_add_dep(yyvsp[-1].expr);
	printd(DEBUG_PARSE, "%s:%d:requires\n", zconf_curname(), zconf_lineno());
;}
    break;

  case 94:
#line 474 "scripts/kconfig/zconf.y"
    {
	menu_add_prop(P_PROMPT, yyvsp[-1].string, NULL, yyvsp[0].expr);
;}
    break;

  case 97:
#line 482 "scripts/kconfig/zconf.y"
    { yyval.token = T_ENDMENU; ;}
    break;

  case 98:
#line 483 "scripts/kconfig/zconf.y"
    { yyval.token = T_ENDCHOICE; ;}
    break;

  case 99:
#line 484 "scripts/kconfig/zconf.y"
    { yyval.token = T_ENDIF; ;}
    break;

  case 102:
#line 490 "scripts/kconfig/zconf.y"
    { yyval.expr = NULL; ;}
    break;

  case 103:
#line 491 "scripts/kconfig/zconf.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 104:
#line 494 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_symbol(yyvsp[0].symbol); ;}
    break;

  case 105:
#line 495 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_comp(E_EQUAL, yyvsp[-2].symbol, yyvsp[0].symbol); ;}
    break;

  case 106:
#line 496 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_comp(E_UNEQUAL, yyvsp[-2].symbol, yyvsp[0].symbol); ;}
    break;

  case 107:
#line 497 "scripts/kconfig/zconf.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 108:
#line 498 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_one(E_NOT, yyvsp[0].expr); ;}
    break;

  case 109:
#line 499 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_two(E_OR, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 110:
#line 500 "scripts/kconfig/zconf.y"
    { yyval.expr = expr_alloc_two(E_AND, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 111:
#line 503 "scripts/kconfig/zconf.y"
    { yyval.symbol = sym_lookup(yyvsp[0].string, 0); free(yyvsp[0].string); ;}
    break;

  case 112:
#line 504 "scripts/kconfig/zconf.y"
    { yyval.symbol = sym_lookup(yyvsp[0].string, 1); free(yyvsp[0].string); ;}
    break;


    }

/* Line 993 of yacc.c.  */
#line 1760 "scripts/kconfig/zconf.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
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

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 507 "scripts/kconfig/zconf.y"


void conf_parse(const char *name)
{
	struct symbol *sym;
	int i;

	zconf_initscan(name);

	sym_init();
	menu_init();
	modules_sym = sym_lookup("MODULES", 0);
	rootmenu.prompt = menu_add_prop(P_MENU, "Linux Kernel Configuration", NULL, NULL);

	//zconfdebug = 1;
	zconfparse();
	if (zconfnerrs)
		exit(1);
	menu_finalize(&rootmenu);
	for_all_symbols(i, sym) {
                if (!(sym->flags & SYMBOL_CHECKED) && sym_check_deps(sym))
                        printf("\n");
		else
			sym->flags |= SYMBOL_CHECK_DONE;
        }

	sym_change_count = 1;
}

const char *zconf_tokenname(int token)
{
	switch (token) {
	case T_MENU:		return "menu";
	case T_ENDMENU:		return "endmenu";
	case T_CHOICE:		return "choice";
	case T_ENDCHOICE:	return "endchoice";
	case T_IF:		return "if";
	case T_ENDIF:		return "endif";
	}
	return "<token>";
}

static bool zconf_endtoken(int token, int starttoken, int endtoken)
{
	if (token != endtoken) {
		zconfprint("unexpected '%s' within %s block", zconf_tokenname(token), zconf_tokenname(starttoken));
		zconfnerrs++;
		return false;
	}
	if (current_menu->file != current_file) {
		zconfprint("'%s' in different file than '%s'", zconf_tokenname(token), zconf_tokenname(starttoken));
		zconfprint("location of the '%s'", zconf_tokenname(starttoken));
		zconfnerrs++;
		return false;
	}
	return true;
}

static void zconfprint(const char *err, ...)
{
	va_list ap;

	fprintf(stderr, "%s:%d: ", zconf_curname(), zconf_lineno() + 1);
	va_start(ap, err);
	vfprintf(stderr, err, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}

static void zconferror(const char *err)
{
	fprintf(stderr, "%s:%d: %s\n", zconf_curname(), zconf_lineno() + 1, err);
}

void print_quoted_string(FILE *out, const char *str)
{
	const char *p;
	int len;

	putc('"', out);
	while ((p = strchr(str, '"'))) {
		len = p - str;
		if (len)
			fprintf(out, "%.*s", len, str);
		fputs("\\\"", out);
		str = p + 1;
	}
	fputs(str, out);
	putc('"', out);
}

void print_symbol(FILE *out, struct menu *menu)
{
	struct symbol *sym = menu->sym;
	struct property *prop;

	if (sym_is_choice(sym))
		fprintf(out, "choice\n");
	else
		fprintf(out, "config %s\n", sym->name);
	switch (sym->type) {
	case S_BOOLEAN:
		fputs("  boolean\n", out);
		break;
	case S_TRISTATE:
		fputs("  tristate\n", out);
		break;
	case S_STRING:
		fputs("  string\n", out);
		break;
	case S_INT:
		fputs("  integer\n", out);
		break;
	case S_HEX:
		fputs("  hex\n", out);
		break;
	default:
		fputs("  ???\n", out);
		break;
	}
	for (prop = sym->prop; prop; prop = prop->next) {
		if (prop->menu != menu)
			continue;
		switch (prop->type) {
		case P_PROMPT:
			fputs("  prompt ", out);
			print_quoted_string(out, prop->text);
			if (!expr_is_yes(prop->visible.expr)) {
				fputs(" if ", out);
				expr_fprint(prop->visible.expr, out);
			}
			fputc('\n', out);
			break;
		case P_DEFAULT:
			fputs( "  default ", out);
			expr_fprint(prop->expr, out);
			if (!expr_is_yes(prop->visible.expr)) {
				fputs(" if ", out);
				expr_fprint(prop->visible.expr, out);
			}
			fputc('\n', out);
			break;
		case P_CHOICE:
			fputs("  #choice value\n", out);
			break;
		default:
			fprintf(out, "  unknown prop %d!\n", prop->type);
			break;
		}
	}
	if (sym->help) {
		int len = strlen(sym->help);
		while (sym->help[--len] == '\n')
			sym->help[len] = 0;
		fprintf(out, "  help\n%s\n", sym->help);
	}
	fputc('\n', out);
}

void zconfdump(FILE *out)
{
	struct property *prop;
	struct symbol *sym;
	struct menu *menu;

	menu = rootmenu.list;
	while (menu) {
		if ((sym = menu->sym))
			print_symbol(out, menu);
		else if ((prop = menu->prompt)) {
			switch (prop->type) {
			case P_COMMENT:
				fputs("\ncomment ", out);
				print_quoted_string(out, prop->text);
				fputs("\n", out);
				break;
			case P_MENU:
				fputs("\nmenu ", out);
				print_quoted_string(out, prop->text);
				fputs("\n", out);
				break;
			default:
				;
			}
			if (!expr_is_yes(prop->visible.expr)) {
				fputs("  depends ", out);
				expr_fprint(prop->visible.expr, out);
				fputc('\n', out);
			}
			fputs("\n", out);
		}

		if (menu->list)
			menu = menu->list;
		else if (menu->next)
			menu = menu->next;
		else while ((menu = menu->parent)) {
			if (menu->prompt && menu->prompt->type == P_MENU)
				fputs("\nendmenu\n", out);
			if (menu->next) {
				menu = menu->next;
				break;
			}
		}
	}
}

#include "lex.zconf.c"
#include "confdata.c"
#include "expr.c"
#include "symbol.c"
#include "menu.c"

