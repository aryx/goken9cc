/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    FOR = 258,                     /* FOR  */
    IN = 259,                      /* IN  */
    WHILE = 260,                   /* WHILE  */
    IF = 261,                      /* IF  */
    NOT = 262,                     /* NOT  */
    SWITCH = 263,                  /* SWITCH  */
    FN = 264,                      /* FN  */
    TWIDDLE = 265,                 /* TWIDDLE  */
    BANG = 266,                    /* BANG  */
    REDIR = 267,                   /* REDIR  */
    PIPE = 268,                    /* PIPE  */
    ANDAND = 269,                  /* ANDAND  */
    OROR = 270,                    /* OROR  */
    COUNT = 271,                   /* COUNT  */
    SUB = 272,                     /* SUB  */
    WORD = 273,                    /* WORD  */
    SIMPLE = 274,                  /* SIMPLE  */
    ARGLIST = 275,                 /* ARGLIST  */
    WORDS = 276,                   /* WORDS  */
    BRACE = 277,                   /* BRACE  */
    PAREN = 278,                   /* PAREN  */
    PCMD = 279,                    /* PCMD  */
    SUBSHELL = 280,                /* SUBSHELL  */
    DUP = 281,                     /* DUP  */
    PIPEFD = 282                   /* PIPEFD  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define FOR 258
#define IN 259
#define WHILE 260
#define IF 261
#define NOT 262
#define SWITCH 263
#define FN 264
#define TWIDDLE 265
#define BANG 266
#define REDIR 267
#define PIPE 268
#define ANDAND 269
#define OROR 270
#define COUNT 271
#define SUB 272
#define WORD 273
#define SIMPLE 274
#define ARGLIST 275
#define WORDS 276
#define BRACE 277
#define PAREN 278
#define PCMD 279
#define SUBSHELL 280
#define DUP 281
#define PIPEFD 282

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 7 "syn.y"

 struct Tree *tree;

#line 125 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
