/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/
#line 48 "parse.y"

#include "sqliteInt.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

/*
** Indicate that sqlite3ParserFree() will never be called with a null
** pointer.
*/
#define YYPARSEFREENEVERNULL 1

/*
** Alternative datatype for the argument to the malloc() routine passed
** into sqlite3ParserAlloc().  The default is size_t.
*/
#define YYMALLOCARGTYPE  u64

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
  Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
  Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** An instance of this structure is used to store the LIKE,
** GLOB, NOT LIKE, and NOT GLOB operators.
*/
struct LikeOp {
  Token eOperator;  /* "like" or "glob" or "regexp" */
  int bNot;         /* True if the NOT keyword is present */
};

/*
** An instance of the following structure describes the event of a
** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
** TK_DELETE, or TK_INSTEAD.  If the event is of the form
**
**      UPDATE ON (a,b,c)
**
** Then the "b" IdList records the list "a,b,c".
*/
struct TrigEvent { int a; IdList * b; };

/*
** An instance of this structure holds the ATTACH key and the key type.
*/
struct AttachKey { int type;  Token key; };

/*
** Disable lookaside memory allocation for objects that might be
** shared across database connections.
*/
static void disableLookaside(Parse *pParse){
  pParse->disableLookaside++;
  pParse->db->lookaside.bDisable++;
}

#line 414 "parse.y"

  /*
  ** For a compound SELECT statement, make sure p->pPrior->pNext==p for
  ** all elements in the list.  And make sure list length does not exceed
  ** SQLITE_LIMIT_COMPOUND_SELECT.
  */
  static void parserDoubleLinkSelect(Parse *pParse, Select *p){
    if( p->pPrior ){
      Select *pNext = 0, *pLoop;
      int mxSelect, cnt = 0;
      for(pLoop=p; pLoop; pNext=pLoop, pLoop=pLoop->pPrior, cnt++){
        pLoop->pNext = pNext;
        pLoop->selFlags |= SF_Compound;
      }
      if( (p->selFlags & SF_MultiValue)==0 && 
        (mxSelect = pParse->db->aLimit[SQLITE_LIMIT_COMPOUND_SELECT])>0 &&
        cnt>mxSelect
      ){
        sqlite3ErrorMsg(pParse, "too many terms in compound SELECT");
      }
    }
  }
#line 831 "parse.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token t){
    pOut->pExpr = sqlite3PExpr(pParse, op, 0, 0, &t);
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 923 "parse.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(
    Parse *pParse,      /* The parsing context.  Errors accumulate here */
    int op,             /* The binary operation */
    ExprSpan *pLeft,    /* The left operand, and output */
    ExprSpan *pRight    /* The right operand */
  ){
    pLeft->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr, 0);
    pLeft->zEnd = pRight->zEnd;
  }

  /* If doNot is true, then add a TK_NOT Expr-node wrapper around the
  ** outside of *ppExpr.
  */
  static void exprNot(Parse *pParse, int doNot, ExprSpan *pSpan){
    if( doNot ){
      pSpan->pExpr = sqlite3PExpr(pParse, TK_NOT, pSpan->pExpr, 0, 0);
    }
  }
#line 982 "parse.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand, and output */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOperand->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOperand->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 999 "parse.y"

  /* A routine to convert a binary TK_IS or TK_ISNOT expression into a
  ** unary TK_ISNULL or TK_NOTNULL expression. */
  static void binaryToUnaryIfNull(Parse *pParse, Expr *pY, Expr *pA, int op){
    sqlite3 *db = pParse->db;
    if( pA && pY && pY->op==TK_NULL ){
      pA->op = (u8)op;
      sqlite3ExprDelete(db, pA->pRight);
      pA->pRight = 0;
    }
  }
#line 1027 "parse.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(
    ExprSpan *pOut,        /* Write the new expression node here */
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->zStart = pPreOp->z;
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOut->zEnd = pOperand->zEnd;
  }
#line 1229 "parse.y"

  /* Add a single new term to an ExprList that is used to store a
  ** list of identifiers.  Report an error if the ID list contains
  ** a COLLATE clause or an ASC or DESC keyword, except ignore the
  ** error while parsing a legacy schema.
  */
  static ExprList *parserAddExprIdListTerm(
    Parse *pParse,
    ExprList *pPrior,
    Token *pIdToken,
    int hasCollate,
    int sortOrder
  ){
    ExprList *p = sqlite3ExprListAppend(pParse, pPrior, 0);
    if( (hasCollate || sortOrder!=SQLITE_SO_UNDEFINED)
        && pParse->db->init.busy==0
    ){
      sqlite3ErrorMsg(pParse, "syntax error after column name \"%.*s\"",
                         pIdToken->n, pIdToken->z);
    }
    sqlite3ExprListSetName(pParse, p, pIdToken, 1);
    return p;
  }
#line 228 "parse.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 227
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 99
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  Select* yy7;
  SrcList* yy55;
  ExprList* yy86;
  With* yy211;
  IdList* yy224;
  struct LimitVal yy240;
  struct LikeOp yy286;
  ExprSpan yy306;
  int yy312;
  Expr* yy314;
  struct {int value; int mask;} yy411;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             307
#define YYNRULE              240
#define YY_MAX_SHIFT         306
#define YY_MIN_SHIFTREDUCE   457
#define YY_MAX_SHIFTREDUCE   696
#define YY_MIN_REDUCE        697
#define YY_MAX_REDUCE        936
#define YY_ERROR_ACTION      937
#define YY_ACCEPT_ACTION     938
#define YY_NO_ACTION         939
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE

**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1076)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   226,    3,   95,  138,  138,  166,  268,   51,   52,   42,
 /*    10 */   615,  615,  627,  247,  619,  619,   49,   49,   50,   50,
 /*    20 */    50,   50,  663,   48,   48,   48,   48,   47,   47,   46,
 /*    30 */    46,   46,   45,  245,  693,  693,  226,   46,   46,   46,
 /*    40 */    45,  245,  187,   51,   52,   42,  615,  615,  627,  296,
 /*    50 */   619,  619,   49,   49,   50,   50,   50,   50,  301,   48,
 /*    60 */    48,   48,   48,   47,   47,   46,   46,   46,   45,  245,
 /*    70 */    47,   47,   46,   46,   46,   45,  245,  569,  694,   79,
 /*    80 */    95,  226,  924,  693,  693,  515,  570,   23,   51,   52,
 /*    90 */    42,  615,  615,  627,  245,  619,  619,   49,   49,   50,
 /*   100 */    50,   50,   50,  586,   48,   48,   48,   48,   47,   47,
 /*   110 */    46,   46,   46,   45,  245,  142,  568,  267,  290,  287,
 /*   120 */   286,   44,   41,  226,  536,  537,  200,  249,  213,  285,
 /*   130 */    51,   52,   42,  615,  615,  627,   59,  619,  619,   49,
 /*   140 */    49,   50,   50,   50,   50,   24,   48,   48,   48,   48,
 /*   150 */    47,   47,   46,   46,   46,   45,  245,  226,  525,  689,
 /*   160 */   492,   44,   41,  259,   51,   52,   42,  615,  615,  627,
 /*   170 */   484,  619,  619,   49,   49,   50,   50,   50,   50,  641,
 /*   180 */    48,   48,   48,   48,   47,   47,   46,   46,   46,   45,
 /*   190 */   245,  226,  161,  260,  485,  688,  499,  180,   51,   52,
 /*   200 */    42,  615,  615,  627,  520,  619,  619,   49,   49,   50,
 /*   210 */    50,   50,   50,  882,   48,   48,   48,   48,   47,   47,
 /*   220 */    46,   46,   46,   45,  245,  226,   59,  938,  113,  662,
 /*   230 */     1,  256,   51,   52,   42,  615,  615,  627,  176,  619,
 /*   240 */   619,   49,   49,   50,   50,   50,   50,  487,   48,   48,
 /*   250 */    48,   48,   47,   47,   46,   46,   46,   45,  245,  226,
 /*   260 */   616,  616,  628,  291,  679,  680,   51,   52,   42,  615,
 /*   270 */   615,  627,  569,  619,  619,   49,   49,   50,   50,   50,
 /*   280 */    50,  570,   48,   48,   48,   48,   47,   47,   46,   46,
 /*   290 */    46,   45,  245,  226,  523,  689,  594,  238,  679,  680,
 /*   300 */    51,   52,   42,  615,  615,  627,  147,  619,  619,   49,
 /*   310 */    49,   50,   50,   50,   50,  301,   48,   48,   48,   48,
 /*   320 */    47,   47,   46,   46,   46,   45,  245,  567,  540,  460,
 /*   330 */   461,  462,  486,  187,  226,  694,   75,  159,  620,   44,
 /*   340 */    41,   51,   52,   42,  615,  615,  627,   31,  619,  619,
 /*   350 */    49,   49,   50,   50,   50,   50,  300,   48,   48,   48,
 /*   360 */    48,   47,   47,   46,   46,   46,   45,  245,  226,  297,
 /*   370 */   269,  277,   44,   41,  593,   51,   40,   42,  615,  615,
 /*   380 */   627,   34,  619,  619,   49,   49,   50,   50,   50,   50,
 /*   390 */   228,   48,   48,   48,   48,   47,   47,   46,   46,   46,
 /*   400 */    45,  245,  226,  180,  244,  146,  145,  144,  263,  554,
 /*   410 */    52,   42,  615,  615,  627,  127,  619,  619,   49,   49,
 /*   420 */    50,   50,   50,   50,  678,   48,   48,   48,   48,   47,
 /*   430 */    47,   46,   46,   46,   45,  245,  226,  239,  229,  204,
 /*   440 */   180,  914,  478,  227,  162,   42,  615,  615,  627,  589,
 /*   450 */   619,  619,   49,   49,   50,   50,   50,   50,  692,   48,
 /*   460 */    48,   48,   48,   47,   47,   46,   46,   46,   45,  245,
 /*   470 */    39,  681,   53,  678,  234,  685,  175,  252,  177,  494,
 /*   480 */    39,  163,   53,  166,  268,   30,  138,   53,  128,  272,
 /*   490 */   151,  123,  212,  294,  207,  293,  148,   36,   37,  541,
 /*   500 */   250,  674,  674,  203,   38,  246,  246,   36,   37,  694,
 /*   510 */    14,  554,   36,   37,   38,  246,  246,  693,  164,   38,
 /*   520 */   246,  246,   50,   50,   50,   50,   43,   48,   48,   48,
 /*   530 */    48,   47,   47,   46,   46,   46,   45,  245,   48,   48,
 /*   540 */    48,   48,   47,   47,   46,   46,   46,   45,  245,   54,
 /*   550 */   675,  676,  606,  254,  303,  302,   45,  245,  595,  118,
 /*   560 */   532,  690,  606,  495,  303,  302,  693,  606,  595,  303,
 /*   570 */   302,  271,  160,  595,  125,  124,  181,  220,  219,  218,
 /*   580 */   154,  216,  488,  488,  470,  183,  600,  600,  600,  602,
 /*   590 */    11,  674,  674,  237,  236,  180,  600,  600,  600,  602,
 /*   600 */    11,  600,  600,  600,  602,   11,  301,  180,   50,   50,
 /*   610 */    50,   50,  495,   48,   48,   48,   48,   47,   47,   46,
 /*   620 */    46,   46,   45,  245,   57,  652,  694,   75,  593,  606,
 /*   630 */   591,  601,  606,  137,  601,  595,  649,   58,  595,  116,
 /*   640 */   675,  676,  142,  306,  457,  290,  287,  286,  528,  174,
 /*   650 */   301,  110,  674,  674,  661,    1,  285,  482,  531,  204,
 /*   660 */   297,  273,   95,  600,  600,  600,  600,  600,  600,  653,
 /*   670 */   694,   75,  542,  674,  674,  674,  674,  301,  674,  674,
 /*   680 */   282,   27,  696,  696,  105,  185,  654,  230,  526,  301,
 /*   690 */   147,   13,  233,  301,  241,  301,  274,  694,   15,  301,
 /*   700 */   527,  675,  676,  179,  297,  298,  482,  506,  264,  694,
 /*   710 */    75,  231,  130,  694,   75,  694,   15,  475,  301,  694,
 /*   720 */    15,  292,  675,  676,  675,  676,  188,  675,  676,  280,
 /*   730 */   111,  507,  275,  235,  532,  690,  900,  663,  694,   15,
 /*   740 */   211,   59,  225,  232,  301,  271,  556,  278,  555,  301,
 /*   750 */    59,  210,  167,  536,  537,  301,  182,  301,  248,   27,
 /*   760 */   242,  168,  301,  686,  694,   67,  503,  301,  497,  694,
 /*   770 */    68,  301,  502,  301,  228,  694,   69,  694,   70,  203,
 /*   780 */   301,  534,  694,   71,   29,  301,  190,  694,   72,  301,
 /*   790 */   552,  694,   73,  694,   60,  301,  648,  301,  158,  140,
 /*   800 */   694,   61,  301,  603,  301,  694,   16,  301,  561,  694,
 /*   810 */    62,  301,   35,  301,   33,  694,   74,  694,   85,  301,
 /*   820 */    21,  301,  694,   76,  694,   64,  301,  694,   77,  301,
 /*   830 */   501,  694,   78,  694,   65,  301,  592,  301,  543,  694,
 /*   840 */   120,  694,  121,  193,  301,  648,  694,  122,  301,  694,
 /*   850 */    82,  301,  603,  301,  199,  694,   86,  694,   80,  516,
 /*   860 */   301,  205,  301,  500,  694,   87,  301,  206,  694,   88,
 /*   870 */   301,  694,   84,  694,  117,  301,  563,  599,  301,  137,
 /*   880 */   694,  112,  694,   97,  301,  283,  694,   93,  143,  301,
 /*   890 */   694,   89,  483,  301,  111,  694,   90,  477,  694,   81,
 /*   900 */   201,  165,  215,   27,  694,   83,  222,  651,  651,  694,
 /*   910 */    66,   17,  547,  694,   63,  650,  650,  467,  480,  644,
 /*   920 */   466,   56,  143,  505,  504,    4,  512,  513,  468,  668,
 /*   930 */   221,  583,  251,  178,  192,  198,  119,  288,  129,  646,
 /*   940 */   299,  645,  499,  671,  209,  588,   94,   98,  109,   21,
 /*   950 */   253,  580,  101,  135,  553,  640,  255,  258,  132,  103,
 /*   960 */   469,  133,  104,  108,  262,  276,  266,  550,   20,  139,
 /*   970 */    25,  189,  281,  549,  295,   22,  533,  194,  279,  197,
 /*   980 */   195,  223,  519,  196,  243,  191,  518,  517,  510,  240,
 /*   990 */   491,  224,  497,  509,  208,  490,  489,  683,   32,  152,
 /*  1000 */   669,  169,  474,   12,  304,  153,  155,  173,  156,  170,
 /*  1010 */    55,  305,  464,   91,  463,  171,  458,  587,   99,   92,
 /*  1020 */    18,   19,  100,  529,  102,    5,  131,  472,  184,  257,
 /*  1030 */   186,  658,  114,  134,  115,  261,  126,  659,    6,  265,
 /*  1040 */   535,    2,  136,  157,  270,  562,  106,   29,  557,    7,
 /*  1050 */   107,   27,    8,  210,  141,  284,   26,    9,  289,  202,
 /*  1060 */    10,  172,  481,  508,   28,   96,  605,  604,  630,  149,
 /*  1070 */   150,  916,  915,  214,  217,  664,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    16,   18,   92,   20,   20,  118,  119,   23,   24,   25,
 /*    10 */    26,   27,   28,  103,   30,   31,   32,   33,   34,   35,
 /*    20 */    36,   37,    1,   39,   40,   41,   42,   43,   44,   45,
 /*    30 */    46,   47,   48,   49,   51,   51,   16,   45,   46,   47,
 /*    40 */    48,   49,  150,   23,   24,   25,   26,   27,   28,   45,
 /*    50 */    30,   31,   32,   33,   34,   35,   36,   37,  150,   39,
 /*    60 */    40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
 /*    70 */    43,   44,   45,   46,   47,   48,   49,   57,  170,  171,
 /*    80 */    92,   16,   19,  100,  100,   22,   66,   22,   23,   24,
 /*    90 */    25,   26,   27,   28,   49,   30,   31,   32,   33,   34,
 /*   100 */    35,   36,   37,  161,   39,   40,   41,   42,   43,   44,
 /*   110 */    45,   46,   47,   48,   49,  102,  173,  225,  105,  106,
 /*   120 */   107,  218,  219,   16,  120,  121,   19,  150,  220,  116,
 /*   130 */    23,   24,   25,   26,   27,   28,  194,   30,   31,   32,
 /*   140 */    33,   34,   35,   36,   37,   80,   39,   40,   41,   42,
 /*   150 */    43,   44,   45,   46,   47,   48,   49,   16,  188,  189,
 /*   160 */    19,  218,  219,  155,   23,   24,   25,   26,   27,   28,
 /*   170 */   170,   30,   31,   32,   33,   34,   35,   36,   37,  106,
 /*   180 */    39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
 /*   190 */    49,   16,  215,  161,   19,  177,  178,  150,   23,   24,
 /*   200 */    25,   26,   27,   28,  186,   30,   31,   32,   33,   34,
 /*   210 */    35,   36,   37,  140,   39,   40,   41,   42,   43,   44,
 /*   220 */    45,   46,   47,   48,   49,   16,  194,  142,  143,  144,
 /*   230 */   145,  223,   23,   24,   25,   26,   27,   28,  191,   30,
 /*   240 */    31,   32,   33,   34,   35,   36,   37,  170,   39,   40,
 /*   250 */    41,   42,   43,   44,   45,   46,   47,   48,   49,   16,
 /*   260 */    26,   27,   28,  166,  167,  168,   23,   24,   25,   26,
 /*   270 */    27,   28,   57,   30,   31,   32,   33,   34,   35,   36,
 /*   280 */    37,   66,   39,   40,   41,   42,   43,   44,   45,   46,
 /*   290 */    47,   48,   49,   16,  188,  189,   19,  166,  167,  168,
 /*   300 */    23,   24,   25,   26,   27,   28,   26,   30,   31,   32,
 /*   310 */    33,   34,   35,   36,   37,  150,   39,   40,   41,   42,
 /*   320 */    43,   44,   45,   46,   47,   48,   49,  173,  207,    4,
 /*   330 */     5,    6,  170,  150,   16,  170,  171,   18,  104,  218,
 /*   340 */   219,   23,   24,   25,   26,   27,   28,  138,   30,   31,
 /*   350 */    32,   33,   34,   35,   36,   37,  150,   39,   40,   41,
 /*   360 */    42,   43,   44,   45,   46,   47,   48,   49,   16,  204,
 /*   370 */   205,  150,  218,  219,  150,   23,   24,   25,   26,   27,
 /*   380 */    28,  138,   30,   31,   32,   33,   34,   35,   36,   37,
 /*   390 */   110,   39,   40,   41,   42,   43,   44,   45,   46,   47,
 /*   400 */    48,   49,   16,  150,   16,  111,  112,  113,  225,   22,
 /*   410 */    24,   25,   26,   27,   28,  150,   30,   31,   32,   33,
 /*   420 */    34,   35,   36,   37,   51,   39,   40,   41,   42,   43,
 /*   430 */    44,   45,   46,   47,   48,   49,   16,   49,  214,  150,
 /*   440 */   150,  122,  164,  165,  191,   25,   26,   27,   28,   84,
 /*   450 */    30,   31,   32,   33,   34,   35,   36,   37,   22,   39,
 /*   460 */    40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
 /*   470 */    16,  169,   18,  100,  185,  183,  111,  112,  113,  177,
 /*   480 */    16,  191,   18,  118,  119,   16,   20,   18,  101,  150,
 /*   490 */   102,  103,  104,  105,  106,  107,  108,   43,   44,   45,
 /*   500 */   150,   51,   52,  115,   50,   51,   52,   43,   44,  170,
 /*   510 */   171,  124,   43,   44,   50,   51,   52,   51,  207,   50,
 /*   520 */    51,   52,   34,   35,   36,   37,   38,   39,   40,   41,
 /*   530 */    42,   43,   44,   45,   46,   47,   48,   49,   39,   40,
 /*   540 */    41,   42,   43,   44,   45,   46,   47,   48,   49,   18,
 /*   550 */   100,  101,   98,  150,  100,  101,   48,   49,  104,  123,
 /*   560 */   192,  193,   98,   51,  100,  101,  100,   98,  104,  100,
 /*   570 */   101,  203,    2,  104,   43,   44,   13,    7,    8,    9,
 /*   580 */    10,   11,   51,   52,   14,  150,  132,  133,  134,  135,
 /*   590 */   136,   51,   52,   43,   44,  150,  132,  133,  134,  135,
 /*   600 */   136,  132,  133,  134,  135,  136,  150,  150,   34,   35,
 /*   610 */    36,   37,  100,   39,   40,   41,   42,   43,   44,   45,
 /*   620 */    46,   47,   48,   49,   18,    9,  170,  171,  150,   98,
 /*   630 */    19,  100,   98,   22,  100,  104,  191,   18,  104,   20,
 /*   640 */   100,  101,  102,  146,  147,  105,  106,  107,  191,  152,
 /*   650 */   150,  154,   51,   52,  144,  145,  116,   51,  161,  150,
 /*   660 */   204,  205,   92,  132,  133,  134,  132,  133,  134,   53,
 /*   670 */   170,  171,   45,   51,   52,   51,   52,  150,   51,   52,
 /*   680 */    16,   22,  132,  133,   83,  150,   70,  117,   72,  150,
 /*   690 */    26,  194,  214,  150,  185,  150,   16,  170,  171,  150,
 /*   700 */    84,  100,  101,  140,  204,  205,  100,   61,  161,  170,
 /*   710 */   171,  184,   22,  170,  171,  170,  171,  161,  150,  170,
 /*   720 */   171,   75,  100,  101,  100,  101,  150,  100,  101,  184,
 /*   730 */   150,   85,   52,  184,  192,  193,    0,    1,  170,  171,
 /*   740 */   104,  194,  162,  204,  150,  203,  124,  204,  124,  150,
 /*   750 */   194,  115,  184,  120,  121,  150,   19,  150,  216,   22,
 /*   760 */   114,  181,  150,  183,  170,  171,  179,  150,  109,  170,
 /*   770 */   171,  150,  179,  150,  110,  170,  171,  170,  171,  115,
 /*   780 */   150,   19,  170,  171,   22,  150,  207,  170,  171,  150,
 /*   790 */   150,  170,  171,  170,  171,  150,   51,  150,  208,  209,
 /*   800 */   170,  171,  150,   51,  150,  170,  171,  150,  150,  170,
 /*   810 */   171,  150,  137,  150,  139,  170,  171,  170,  171,  150,
 /*   820 */   130,  150,  170,  171,  170,  171,  150,  170,  171,  150,
 /*   830 */   179,  170,  171,  170,  171,  150,  150,  150,  150,  170,
 /*   840 */   171,  170,  171,  150,  150,  100,  170,  171,  150,  170,
 /*   850 */   171,  150,  100,  150,  150,  170,  171,  170,  171,  150,
 /*   860 */   150,  150,  150,  150,  170,  171,  150,  150,  170,  171,
 /*   870 */   150,  170,  171,  170,  171,  150,   19,  150,  150,   22,
 /*   880 */   170,  171,  170,  171,  150,   19,  170,  171,   22,  150,
 /*   890 */   170,  171,  150,  150,  150,  170,  171,  150,  170,  171,
 /*   900 */    19,  196,  158,   22,  170,  171,  162,  132,  133,  170,
 /*   910 */   171,  206,  210,  170,  171,  132,  133,  150,   19,   19,
 /*   920 */   150,   22,   22,  103,  104,  195,    4,    5,  150,  150,
 /*   930 */   148,  198,  211,  211,  211,  197,  195,  174,  182,  173,
 /*   940 */   222,  173,  178,  153,  173,  187,   23,   18,  217,  130,
 /*   950 */    15,  198,  187,  122,  157,  198,  157,   15,  156,  190,
 /*   960 */   157,  156,  190,   18,  175,  125,  175,  213,  110,  157,
 /*   970 */   129,  212,  126,  213,  110,  128,  202,  201,  127,  198,
 /*   980 */   200,  175,  172,  199,   73,  212,  172,  172,  180,   49,
 /*   990 */   172,  175,  109,  180,  172,  174,  172,  172,  137,   21,
 /*  1000 */    10,  224,  160,   22,  159,  151,  151,  221,    3,  224,
 /*  1010 */   176,  149,  149,  163,  149,  176,  149,  119,  131,  163,
 /*  1020 */    33,   33,  114,   17,  123,   18,  110,   17,  140,   16,
 /*  1030 */    13,   19,   18,   22,   18,   20,  122,   19,   18,   20,
 /*  1040 */    19,   18,   60,   19,   22,   52,   18,   22,  124,   60,
 /*  1050 */    18,   22,   60,  115,  108,   78,   18,   18,   78,   19,
 /*  1060 */    18,   78,   19,   54,   18,   64,   19,   19,    8,  122,
 /*  1070 */   122,  122,  122,   19,   12,    1,
};
#define YY_SHIFT_USE_DFLT (-114)
#define YY_SHIFT_COUNT (306)
#define YY_SHIFT_MIN   (-113)
#define YY_SHIFT_MAX   (1074)
static const short yy_shift_ofst[] = {
 /*     0 */    21,  570,  464,  464,  464,  464,  464,  464,  464,  464,
 /*    10 */   464,  464,  540,  365,  -16,   20,   20,  454,  464,  464,
 /*    20 */   464,  464,  464,  464,  464,  464,  464,  464,  464,  464,
 /*    30 */   464,  464,  464,  464,  464,  464,  464,  464,  464,  464,
 /*    40 */   464,  464,  469,  464,  464,  464,  464,  464,  464,  464,
 /*    50 */   464,  464,  464,  464,  464,  664,  550,  550,  450, -113,
 /*    60 */    65,  107,  141,  175,  209,  243,  277,  318,  318,  318,
 /*    70 */   318,  318,  318,  318,  318,  318,  318,  318,  318,  318,
 /*    80 */   352,  386,  420,  420,  488,  574,  574,  574,  574,  574,
 /*    90 */   499,  388,  388,   27,  616,  601,  616,   -8,  450,  450,
 /*   100 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  -90,
 /*   110 */    73,  373,  508,  736,  -12,  -12,  -12,   45, -114, -114,
 /*   120 */  -114, -114, -114,  531,  534,  534,  627,  622,  624,   13,
 /*   130 */   450,  450,  450,  450,  450,  450,  450,  450,  450,  -17,
 /*   140 */   450,  450,  450,  450,  646,  646,  646,  450,  450,  450,
 /*   150 */   450,  450,  450,  450,  450,  450,  450,  466,  387,    4,
 /*   160 */   325,  690,  680,  680,  680,  436,  633,  659,   63,  215,
 /*   170 */   215,  280,  215,  675,  619,  923,  929,  923,  819,  935,
 /*   180 */   831,  935,  819,  831,  942,  831,  942,  945,  945,  840,
 /*   190 */   858,  840,  858,  831,  841,  846,  847,  851,  819,  945,
 /*   200 */   864,  864,  864,  911,  940,  940,  945,  864,  883,  864,
 /*   210 */   911,  864,  864,  861,  978,  981,  990,  990, 1005, 1005,
 /*   220 */  1005, 1005, -114, -114, -114, -114,  234,  606,  294,  611,
 /*   230 */   563,  737,  762,  857,  866,  881,  775,  783,  899,  512,
 /*   240 */   745,  900,  820,  922,  636,  752,  319,  898,  887,  987,
 /*   250 */   988,  908,  901, 1006, 1007,  916,  888, 1010, 1013, 1017,
 /*   260 */  1012, 1014, 1015, 1011, 1018, 1016, 1019, 1011, 1020, 1021,
 /*   270 */  1023, 1022,  914, 1024,  993,  982, 1028,  924, 1025,  989,
 /*   280 */  1029,  992,  938,  946, 1032,  977, 1038, 1039, 1040, 1042,
 /*   290 */   980, 1043, 1009, 1046,  983, 1001, 1047, 1025, 1048, 1060,
 /*   300 */   947,  948,  949,  950, 1054, 1062, 1074,
};
#define YY_REDUCE_USE_DFLT (-109)
#define YY_REDUCE_COUNT (225)
#define YY_REDUCE_MIN   (-108)
#define YY_REDUCE_MAX   (867)
static const short yy_reduce_ofst[] = {
 /*     0 */    85,  497,  165,  456,  500,  527,  539,  543,  545,  549,
 /*    10 */   568,  -92,  580,  542,  121,  -57,  154,  339,  594,  599,
 /*    20 */   605,  607,  612,  617,  621,  623,  630,  635,  639,  645,
 /*    30 */   647,  652,  654,  657,  661,  663,  669,  671,  676,  679,
 /*    40 */   685,  687,  694,  698,  701,  703,  710,  712,  716,  720,
 /*    50 */   725,  728,  734,  739,  743,   18,   97,  131,  744,  368,
 /*    60 */   -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,
 /*    70 */   -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,
 /*    80 */   -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,  -97,
 /*    90 */   -97,  302,  302,  -97,  -30, -108,  106,  -97,  224,   47,
 /*   100 */   -23,  253,  290,  445,  457,  183,  478,  289,  509,  -58,
 /*   110 */     8,  278,  -97,  510,   32,  547,  556,  -97,  590,  705,
 /*   120 */   -97,  -97,  -97,    0,   77,  162,  206,  221,  265,  292,
 /*   130 */   350,  403,  435,  535,  576,  640,  658,  686,  688,  311,
 /*   140 */   693,  704,  709,  711,  587,  593,  651,  713,  717,  727,
 /*   150 */   206,  742,  747,  767,  770,  778,  779,  579,  702,  730,
 /*   160 */   782,  733,  721,  722,  723,  738,  741,  763,  756,  766,
 /*   170 */   768,  764,  771,  718,  790,  758,  731,  765,  753,  769,
 /*   180 */   797,  772,  757,  799,  802,  803,  805,  789,  791,  754,
 /*   190 */   759,  760,  773,  812,  774,  776,  780,  784,  781,  806,
 /*   200 */   810,  814,  815,  808,  777,  785,  816,  818,  821,  822,
 /*   210 */   813,  824,  825,  786,  842,  845,  854,  855,  862,  863,
 /*   220 */   865,  867,  850,  834,  839,  856,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   905,  895,  877,  877,  877,  937,  937,  937,  937,  937,
 /*    10 */   937,  876,  937,  937,  784,  811,  811,  937,  937,  937,
 /*    20 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*    30 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*    40 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*    50 */   937,  937,  937,  937,  937,  761,  937,  937,  937,  937,
 /*    60 */   817,  937,  937,  937,  937,  937,  937,  825,  824,  798,
 /*    70 */   822,  815,  819,  818,  878,  879,  872,  873,  871,  875,
 /*    80 */   937,  848,  863,  847,  857,  862,  869,  861,  858,  850,
 /*    90 */   849,  913,  912,  851,  937,  937,  937,  852,  937,  937,
 /*   100 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  895,
 /*   110 */   713,  719,  853,  905,  895,  895,  895,  854,  788,  779,
 /*   120 */   866,  865,  864,  937,  937,  937,  937,  937,  937,  937,
 /*   130 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  784,
 /*   140 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*   150 */   937,  937,  937,  937,  937,  937,  907,  784,  786,  778,
 /*   160 */   699,  821,  800,  800,  800,  785,  778,  736,  927,  811,
 /*   170 */   811,  733,  811,  874,  937,  764,  830,  764,  821,  770,
 /*   180 */   791,  770,  821,  791,  711,  791,  711,  883,  883,  804,
 /*   190 */   799,  804,  799,  791,  816,  805,  814,  812,  821,  883,
 /*   200 */   762,  762,  762,  751,  887,  887,  883,  762,  736,  762,
 /*   210 */   751,  762,  762,  937,  716,  754,  910,  910,  906,  906,
 /*   220 */   906,  906,  922,  738,  738,  922,  937,  917,  937,  937,
 /*   230 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*   240 */   937,  937,  937,  937,  937,  937,  836,  937,  937,  937,
 /*   250 */   937,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*   260 */   937,  937,  937,  897,  937,  937,  937,  896,  937,  937,
 /*   270 */   937,  931,  937,  937,  937,  937,  937,  937,  813,  937,
 /*   280 */   806,  937,  937,  937,  937,  937,  937,  937,  937,  937,
 /*   290 */   937,  937,  937,  937,  937,  937,  937,  935,  937,  937,
 /*   300 */   838,  937,  837,  841,  937,  705,  937,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
   51,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
   51,  /*   DEFERRED => ID */
   51,  /*  IMMEDIATE => ID */
   51,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
   51,  /*        END => ID */
   51,  /*   ROLLBACK => ID */
   51,  /*  SAVEPOINT => ID */
   51,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   51,  /*         IF => ID */
    0,  /*        NOT => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
   51,  /*    WITHOUT => ID */
    0,  /*      COMMA => nothing */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*         IS => nothing */
   51,  /*      MATCH => ID */
   51,  /*    LIKE_KW => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*     BITNOT => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
   51,  /*      ABORT => ID */
   51,  /*     ACTION => ID */
   51,  /*      AFTER => ID */
   51,  /*    ANALYZE => ID */
   51,  /*        ASC => ID */
   51,  /*     ATTACH => ID */
   51,  /*     BEFORE => ID */
   51,  /*         BY => ID */
   51,  /*    CASCADE => ID */
   51,  /*       CAST => ID */
   51,  /*   COLUMNKW => ID */
   51,  /*   CONFLICT => ID */
   51,  /*   DATABASE => ID */
   51,  /*       DESC => ID */
   51,  /*     DETACH => ID */
   51,  /*       EACH => ID */
   51,  /*    EXPLAIN => ID */
   51,  /*       FAIL => ID */
   51,  /*        FOR => ID */
   51,  /*     IGNORE => ID */
   51,  /*  INITIALLY => ID */
   51,  /*    INSTEAD => ID */
   51,  /*         NO => ID */
   51,  /*       PLAN => ID */
   51,  /*      QUERY => ID */
   51,  /*        KEY => ID */
   51,  /*         OF => ID */
   51,  /*     OFFSET => ID */
   51,  /*     PRAGMA => ID */
   51,  /*      RAISE => ID */
   51,  /*  RECURSIVE => ID */
   51,  /*    REPLACE => ID */
   51,  /*   RESTRICT => ID */
   51,  /*        ROW => ID */
   51,  /*       TEMP => ID */
   51,  /*    TRIGGER => ID */
   51,  /*     VACUUM => ID */
   51,  /*       VIEW => ID */
   51,  /*    VIRTUAL => ID */
   51,  /*       WITH => ID */
   51,  /*     EXCEPT => ID */
   51,  /*  INTERSECT => ID */
   51,  /*      UNION => ID */
   51,  /*    REINDEX => ID */
   51,  /*     RENAME => ID */
   51,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax;                 /* Maximum value of yyidx */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SEMI",          "BEGIN",         "TRANSACTION", 
  "DEFERRED",      "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",      
  "END",           "ROLLBACK",      "SAVEPOINT",     "RELEASE",     
  "TO",            "TABLE",         "CREATE",        "IF",          
  "NOT",           "EXISTS",        "LP",            "RP",          
  "AS",            "WITHOUT",       "COMMA",         "OR",          
  "AND",           "IS",            "MATCH",         "LIKE_KW",     
  "BETWEEN",       "IN",            "ISNULL",        "NOTNULL",     
  "NE",            "EQ",            "GT",            "LE",          
  "LT",            "GE",            "ESCAPE",        "BITAND",      
  "BITOR",         "LSHIFT",        "RSHIFT",        "PLUS",        
  "MINUS",         "STAR",          "SLASH",         "REM",         
  "CONCAT",        "COLLATE",       "BITNOT",        "ID",          
  "INDEXED",       "ABORT",         "ACTION",        "AFTER",       
  "ANALYZE",       "ASC",           "ATTACH",        "BEFORE",      
  "BY",            "CASCADE",       "CAST",          "COLUMNKW",    
  "CONFLICT",      "DATABASE",      "DESC",          "DETACH",      
  "EACH",          "EXPLAIN",       "FAIL",          "FOR",         
  "IGNORE",        "INITIALLY",     "INSTEAD",       "NO",          
  "PLAN",          "QUERY",         "KEY",           "OF",          
  "OFFSET",        "PRAGMA",        "RAISE",         "RECURSIVE",   
  "REPLACE",       "RESTRICT",      "ROW",           "TEMP",        
  "TRIGGER",       "VACUUM",        "VIEW",          "VIRTUAL",     
  "WITH",          "EXCEPT",        "INTERSECT",     "UNION",       
  "REINDEX",       "RENAME",        "CTIME_KW",      "ANY",         
  "STRING",        "JOIN_KW",       "CONSTRAINT",    "DEFAULT",     
  "NULL",          "PRIMARY",       "UNIQUE",        "CHECK",       
  "REFERENCES",    "AUTOINCR",      "ON",            "INSERT",      
  "DELETE",        "UPDATE",        "SET",           "DEFERRABLE",  
  "FOREIGN",       "DROP",          "SELECT",        "VALUES",      
  "DISTINCT",      "ALL",           "DOT",           "FROM",        
  "JOIN",          "USING",         "ORDER",         "GROUP",       
  "HAVING",        "LIMIT",         "WHERE",         "INTO",        
  "INTEGER",       "FLOAT",         "BLOB",          "VARIABLE",    
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "error",         "input",         "cmdlist",     
  "ecmd",          "explain",       "cmdx",          "cmd",         
  "transtype",     "trans_opt",     "nm",            "savepoint_opt",
  "create_table",  "create_table_args",  "createkw",      "temp",        
  "ifnotexists",   "dbnm",          "columnlist",    "conslist_opt",
  "table_options",  "select",        "columnname",    "carglist",    
  "typetoken",     "typename",      "signed",        "plus_num",    
  "minus_num",     "ccons",         "term",          "expr",        
  "onconf",        "sortorder",     "autoinc",       "eidlist_opt", 
  "refargs",       "defer_subclause",  "refarg",        "refact",      
  "init_deferred_pred_opt",  "conslist",      "tconscomma",    "tcons",       
  "sortlist",      "eidlist",       "defer_subclause_opt",  "orconf",      
  "resolvetype",   "raisetype",     "ifexists",      "fullname",    
  "selectnowith",  "oneselect",     "with",          "distinct",    
  "selcollist",    "from",          "where_opt",     "groupby_opt", 
  "having_opt",    "orderby_opt",   "limit_opt",     "values",      
  "nexprlist",     "exprlist",      "sclp",          "as",          
  "seltablist",    "stl_prefix",    "joinop",        "indexed_opt", 
  "on_opt",        "using_opt",     "idlist",        "setlist",     
  "insert_cmd",    "idlist_opt",    "likeop",        "between_op",  
  "case_operand",  "case_exprlist",  "case_else",     "uniqueflag",  
  "collate",       "wqlist",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "cmdx ::= cmd",
 /*   1 */ "cmd ::= BEGIN transtype trans_opt",
 /*   2 */ "transtype ::=",
 /*   3 */ "transtype ::= DEFERRED",
 /*   4 */ "transtype ::= IMMEDIATE",
 /*   5 */ "transtype ::= EXCLUSIVE",
 /*   6 */ "cmd ::= COMMIT trans_opt",
 /*   7 */ "cmd ::= END trans_opt",
 /*   8 */ "cmd ::= ROLLBACK trans_opt",
 /*   9 */ "cmd ::= SAVEPOINT nm",
 /*  10 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  11 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  12 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  13 */ "createkw ::= CREATE",
 /*  14 */ "ifnotexists ::=",
 /*  15 */ "ifnotexists ::= IF NOT EXISTS",
 /*  16 */ "temp ::=",
 /*  17 */ "create_table_args ::= LP columnlist conslist_opt RP table_options",
 /*  18 */ "create_table_args ::= AS select",
 /*  19 */ "table_options ::=",
 /*  20 */ "table_options ::= WITHOUT nm",
 /*  21 */ "columnname ::= nm typetoken",
 /*  22 */ "typetoken ::=",
 /*  23 */ "typetoken ::= typename LP signed RP",
 /*  24 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  25 */ "typename ::= typename ID|STRING",
 /*  26 */ "ccons ::= CONSTRAINT nm",
 /*  27 */ "ccons ::= DEFAULT term",
 /*  28 */ "ccons ::= DEFAULT LP expr RP",
 /*  29 */ "ccons ::= DEFAULT PLUS term",
 /*  30 */ "ccons ::= DEFAULT MINUS term",
 /*  31 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  32 */ "ccons ::= NOT NULL onconf",
 /*  33 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  34 */ "ccons ::= UNIQUE onconf",
 /*  35 */ "ccons ::= CHECK LP expr RP",
 /*  36 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  37 */ "ccons ::= defer_subclause",
 /*  38 */ "ccons ::= COLLATE ID|STRING",
 /*  39 */ "autoinc ::=",
 /*  40 */ "autoinc ::= AUTOINCR",
 /*  41 */ "refargs ::=",
 /*  42 */ "refargs ::= refargs refarg",
 /*  43 */ "refarg ::= MATCH nm",
 /*  44 */ "refarg ::= ON INSERT refact",
 /*  45 */ "refarg ::= ON DELETE refact",
 /*  46 */ "refarg ::= ON UPDATE refact",
 /*  47 */ "refact ::= SET NULL",
 /*  48 */ "refact ::= SET DEFAULT",
 /*  49 */ "refact ::= CASCADE",
 /*  50 */ "refact ::= RESTRICT",
 /*  51 */ "refact ::= NO ACTION",
 /*  52 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  53 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  54 */ "init_deferred_pred_opt ::=",
 /*  55 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  56 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  57 */ "conslist_opt ::=",
 /*  58 */ "tconscomma ::= COMMA",
 /*  59 */ "tcons ::= CONSTRAINT nm",
 /*  60 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  61 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  62 */ "tcons ::= CHECK LP expr RP onconf",
 /*  63 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  64 */ "defer_subclause_opt ::=",
 /*  65 */ "onconf ::=",
 /*  66 */ "onconf ::= ON CONFLICT resolvetype",
 /*  67 */ "orconf ::=",
 /*  68 */ "orconf ::= OR resolvetype",
 /*  69 */ "resolvetype ::= IGNORE",
 /*  70 */ "resolvetype ::= REPLACE",
 /*  71 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  72 */ "ifexists ::= IF EXISTS",
 /*  73 */ "ifexists ::=",
 /*  74 */ "cmd ::= select",
 /*  75 */ "select ::= with selectnowith",
 /*  76 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  77 */ "values ::= VALUES LP nexprlist RP",
 /*  78 */ "values ::= values COMMA LP exprlist RP",
 /*  79 */ "distinct ::= DISTINCT",
 /*  80 */ "distinct ::= ALL",
 /*  81 */ "distinct ::=",
 /*  82 */ "sclp ::=",
 /*  83 */ "selcollist ::= sclp expr as",
 /*  84 */ "selcollist ::= sclp STAR",
 /*  85 */ "selcollist ::= sclp nm DOT STAR",
 /*  86 */ "as ::= AS nm",
 /*  87 */ "as ::=",
 /*  88 */ "from ::=",
 /*  89 */ "from ::= FROM seltablist",
 /*  90 */ "stl_prefix ::= seltablist joinop",
 /*  91 */ "stl_prefix ::=",
 /*  92 */ "seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt",
 /*  93 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt",
 /*  94 */ "dbnm ::=",
 /*  95 */ "dbnm ::= DOT nm",
 /*  96 */ "fullname ::= nm dbnm",
 /*  97 */ "joinop ::= COMMA|JOIN",
 /*  98 */ "joinop ::= JOIN_KW JOIN",
 /*  99 */ "joinop ::= JOIN_KW nm JOIN",
 /* 100 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 101 */ "on_opt ::= ON expr",
 /* 102 */ "on_opt ::=",
 /* 103 */ "indexed_opt ::=",
 /* 104 */ "indexed_opt ::= INDEXED BY nm",
 /* 105 */ "indexed_opt ::= NOT INDEXED",
 /* 106 */ "using_opt ::= USING LP idlist RP",
 /* 107 */ "using_opt ::=",
 /* 108 */ "orderby_opt ::=",
 /* 109 */ "orderby_opt ::= ORDER BY sortlist",
 /* 110 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 111 */ "sortlist ::= expr sortorder",
 /* 112 */ "sortorder ::= ASC",
 /* 113 */ "sortorder ::= DESC",
 /* 114 */ "sortorder ::=",
 /* 115 */ "groupby_opt ::=",
 /* 116 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 117 */ "having_opt ::=",
 /* 118 */ "having_opt ::= HAVING expr",
 /* 119 */ "limit_opt ::=",
 /* 120 */ "limit_opt ::= LIMIT expr",
 /* 121 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 122 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 123 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 124 */ "where_opt ::=",
 /* 125 */ "where_opt ::= WHERE expr",
 /* 126 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 127 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 128 */ "setlist ::= nm EQ expr",
 /* 129 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 130 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 131 */ "insert_cmd ::= INSERT orconf",
 /* 132 */ "insert_cmd ::= REPLACE",
 /* 133 */ "idlist_opt ::=",
 /* 134 */ "idlist_opt ::= LP idlist RP",
 /* 135 */ "idlist ::= idlist COMMA nm",
 /* 136 */ "idlist ::= nm",
 /* 137 */ "expr ::= LP expr RP",
 /* 138 */ "term ::= NULL",
 /* 139 */ "expr ::= ID|INDEXED",
 /* 140 */ "expr ::= JOIN_KW",
 /* 141 */ "expr ::= nm DOT nm",
 /* 142 */ "expr ::= nm DOT nm DOT nm",
 /* 143 */ "term ::= INTEGER|FLOAT|BLOB",
 /* 144 */ "term ::= STRING",
 /* 145 */ "expr ::= VARIABLE",
 /* 146 */ "expr ::= expr COLLATE ID|STRING",
 /* 147 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 148 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 149 */ "term ::= CTIME_KW",
 /* 150 */ "expr ::= expr AND expr",
 /* 151 */ "expr ::= expr OR expr",
 /* 152 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 153 */ "expr ::= expr EQ|NE expr",
 /* 154 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 155 */ "expr ::= expr PLUS|MINUS expr",
 /* 156 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 157 */ "expr ::= expr CONCAT expr",
 /* 158 */ "likeop ::= LIKE_KW|MATCH",
 /* 159 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 160 */ "expr ::= expr likeop expr",
 /* 161 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 162 */ "expr ::= expr ISNULL|NOTNULL",
 /* 163 */ "expr ::= expr NOT NULL",
 /* 164 */ "expr ::= expr IS expr",
 /* 165 */ "expr ::= expr IS NOT expr",
 /* 166 */ "expr ::= NOT expr",
 /* 167 */ "expr ::= BITNOT expr",
 /* 168 */ "expr ::= MINUS expr",
 /* 169 */ "expr ::= PLUS expr",
 /* 170 */ "between_op ::= BETWEEN",
 /* 171 */ "between_op ::= NOT BETWEEN",
 /* 172 */ "expr ::= expr between_op expr AND expr",
 /* 173 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 174 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 175 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 176 */ "case_else ::= ELSE expr",
 /* 177 */ "case_else ::=",
 /* 178 */ "case_operand ::= expr",
 /* 179 */ "case_operand ::=",
 /* 180 */ "exprlist ::=",
 /* 181 */ "nexprlist ::= nexprlist COMMA expr",
 /* 182 */ "nexprlist ::= expr",
 /* 183 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 184 */ "uniqueflag ::= UNIQUE",
 /* 185 */ "uniqueflag ::=",
 /* 186 */ "eidlist_opt ::=",
 /* 187 */ "eidlist_opt ::= LP eidlist RP",
 /* 188 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 189 */ "eidlist ::= nm collate sortorder",
 /* 190 */ "collate ::=",
 /* 191 */ "collate ::= COLLATE ID|STRING",
 /* 192 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 193 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 194 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 195 */ "raisetype ::= ROLLBACK",
 /* 196 */ "raisetype ::= ABORT",
 /* 197 */ "raisetype ::= FAIL",
 /* 198 */ "with ::=",
 /* 199 */ "with ::= WITH wqlist",
 /* 200 */ "with ::= WITH RECURSIVE wqlist",
 /* 201 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 202 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 203 */ "input ::= cmdlist",
 /* 204 */ "cmdlist ::= cmdlist ecmd",
 /* 205 */ "cmdlist ::= ecmd",
 /* 206 */ "ecmd ::= SEMI",
 /* 207 */ "ecmd ::= explain cmdx SEMI",
 /* 208 */ "explain ::=",
 /* 209 */ "trans_opt ::=",
 /* 210 */ "trans_opt ::= TRANSACTION",
 /* 211 */ "trans_opt ::= TRANSACTION nm",
 /* 212 */ "savepoint_opt ::= SAVEPOINT",
 /* 213 */ "savepoint_opt ::=",
 /* 214 */ "cmd ::= create_table create_table_args",
 /* 215 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 216 */ "columnlist ::= columnname carglist",
 /* 217 */ "nm ::= ID|INDEXED",
 /* 218 */ "nm ::= STRING",
 /* 219 */ "nm ::= JOIN_KW",
 /* 220 */ "typetoken ::= typename",
 /* 221 */ "typename ::= ID|STRING",
 /* 222 */ "signed ::= plus_num",
 /* 223 */ "signed ::= minus_num",
 /* 224 */ "carglist ::= carglist ccons",
 /* 225 */ "carglist ::=",
 /* 226 */ "ccons ::= NULL onconf",
 /* 227 */ "conslist_opt ::= COMMA conslist",
 /* 228 */ "conslist ::= conslist tconscomma tcons",
 /* 229 */ "conslist ::= tcons",
 /* 230 */ "tconscomma ::=",
 /* 231 */ "defer_subclause_opt ::= defer_subclause",
 /* 232 */ "resolvetype ::= raisetype",
 /* 233 */ "selectnowith ::= oneselect",
 /* 234 */ "oneselect ::= values",
 /* 235 */ "sclp ::= selcollist COMMA",
 /* 236 */ "as ::= ID|STRING",
 /* 237 */ "expr ::= term",
 /* 238 */ "exprlist ::= nexprlist",
 /* 239 */ "plus_num ::= INTEGER|FLOAT",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/
static void yyGrowStack(yyParser *p){
  int newSize;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  if( pNew ){
    p->yystack = pNew;
    p->yystksz = newSize;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, p->yystksz);
    }
#endif
  }
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to sqlite3Parser and sqlite3ParserFree.
*/
void *sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
    pParser->yyidx = -1;
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyidxMax = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    yyGrowStack(pParser);
#endif
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  sqlite3ParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 161: /* select */
    case 192: /* selectnowith */
    case 193: /* oneselect */
    case 203: /* values */
{
#line 408 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy7));
#line 1314 "parse.c"
}
      break;
    case 170: /* term */
    case 171: /* expr */
{
#line 829 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy306).pExpr);
#line 1322 "parse.c"
}
      break;
    case 175: /* eidlist_opt */
    case 184: /* sortlist */
    case 185: /* eidlist */
    case 196: /* selcollist */
    case 199: /* groupby_opt */
    case 201: /* orderby_opt */
    case 204: /* nexprlist */
    case 205: /* exprlist */
    case 206: /* sclp */
    case 215: /* setlist */
    case 221: /* case_exprlist */
{
#line 1227 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy86));
#line 1339 "parse.c"
}
      break;
    case 191: /* fullname */
    case 197: /* from */
    case 208: /* seltablist */
    case 209: /* stl_prefix */
{
#line 640 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy55));
#line 1349 "parse.c"
}
      break;
    case 194: /* with */
    case 225: /* wqlist */
{
#line 1504 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy211));
#line 1357 "parse.c"
}
      break;
    case 198: /* where_opt */
    case 200: /* having_opt */
    case 212: /* on_opt */
    case 220: /* case_operand */
    case 222: /* case_else */
{
#line 756 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy314));
#line 1368 "parse.c"
}
      break;
    case 213: /* using_opt */
    case 214: /* idlist */
    case 217: /* idlist_opt */
{
#line 674 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy224));
#line 1377 "parse.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yyidx>=0 );
  yytos = &pParser->yystack[pParser->yyidx--];
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int sqlite3ParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyidxMax;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    if( i==YY_SHIFT_USE_DFLT ) return yy_default[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
      if( iLookAhead>0 ){
#ifdef YYFALLBACK
        YYCODETYPE iFallback;            /* Fallback token */
        if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
               && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
          }
#endif
          assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
          iLookAhead = iFallback;
          continue;
        }
#endif
#ifdef YYWILDCARD
        {
          int j = i - iLookAhead + YYWILDCARD;
          if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
            j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
            j<YY_ACTTAB_COUNT &&
#endif
            yy_lookahead[j]==YYWILDCARD
          ){
#ifndef NDEBUG
            if( yyTraceFILE ){
              fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
                 yyTracePrompt, yyTokenName[iLookAhead],
                 yyTokenName[YYWILDCARD]);
            }
#endif /* NDEBUG */
            return yy_action[j];
          }
        }
#endif /* YYWILDCARD */
      }
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   sqlite3ParserARG_FETCH;
   yypParser->yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
#line 37 "parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1553 "parse.c"
/******** End %stack_overflow code ********************************************/
   sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yystack[yypParser->yyidx].major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yypParser->yyidx>yypParser->yyidxMax ){
    yypParser->yyidxMax = yypParser->yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yyidx>=yypParser->yystksz ){
    yyGrowStack(yypParser);
    if( yypParser->yyidx>=yypParser->yystksz ){
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 146, 1 },
  { 147, 3 },
  { 148, 0 },
  { 148, 1 },
  { 148, 1 },
  { 148, 1 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 3 },
  { 147, 5 },
  { 152, 6 },
  { 154, 1 },
  { 156, 0 },
  { 156, 3 },
  { 155, 0 },
  { 153, 5 },
  { 153, 2 },
  { 160, 0 },
  { 160, 2 },
  { 162, 2 },
  { 164, 0 },
  { 164, 4 },
  { 164, 6 },
  { 165, 2 },
  { 169, 2 },
  { 169, 2 },
  { 169, 4 },
  { 169, 3 },
  { 169, 3 },
  { 169, 2 },
  { 169, 3 },
  { 169, 5 },
  { 169, 2 },
  { 169, 4 },
  { 169, 4 },
  { 169, 1 },
  { 169, 2 },
  { 174, 0 },
  { 174, 1 },
  { 176, 0 },
  { 176, 2 },
  { 178, 2 },
  { 178, 3 },
  { 178, 3 },
  { 178, 3 },
  { 179, 2 },
  { 179, 2 },
  { 179, 1 },
  { 179, 1 },
  { 179, 2 },
  { 177, 3 },
  { 177, 2 },
  { 180, 0 },
  { 180, 2 },
  { 180, 2 },
  { 159, 0 },
  { 182, 1 },
  { 183, 2 },
  { 183, 7 },
  { 183, 5 },
  { 183, 5 },
  { 183, 10 },
  { 186, 0 },
  { 172, 0 },
  { 172, 3 },
  { 187, 0 },
  { 187, 2 },
  { 188, 1 },
  { 188, 1 },
  { 147, 4 },
  { 190, 2 },
  { 190, 0 },
  { 147, 1 },
  { 161, 2 },
  { 193, 9 },
  { 203, 4 },
  { 203, 5 },
  { 195, 1 },
  { 195, 1 },
  { 195, 0 },
  { 206, 0 },
  { 196, 3 },
  { 196, 2 },
  { 196, 4 },
  { 207, 2 },
  { 207, 0 },
  { 197, 0 },
  { 197, 2 },
  { 209, 2 },
  { 209, 0 },
  { 208, 7 },
  { 208, 9 },
  { 157, 0 },
  { 157, 2 },
  { 191, 2 },
  { 210, 1 },
  { 210, 2 },
  { 210, 3 },
  { 210, 4 },
  { 212, 2 },
  { 212, 0 },
  { 211, 0 },
  { 211, 3 },
  { 211, 2 },
  { 213, 4 },
  { 213, 0 },
  { 201, 0 },
  { 201, 3 },
  { 184, 4 },
  { 184, 2 },
  { 173, 1 },
  { 173, 1 },
  { 173, 0 },
  { 199, 0 },
  { 199, 3 },
  { 200, 0 },
  { 200, 2 },
  { 202, 0 },
  { 202, 2 },
  { 202, 4 },
  { 202, 4 },
  { 147, 6 },
  { 198, 0 },
  { 198, 2 },
  { 147, 8 },
  { 215, 5 },
  { 215, 3 },
  { 147, 6 },
  { 147, 7 },
  { 216, 2 },
  { 216, 1 },
  { 217, 0 },
  { 217, 3 },
  { 214, 3 },
  { 214, 1 },
  { 171, 3 },
  { 170, 1 },
  { 171, 1 },
  { 171, 1 },
  { 171, 3 },
  { 171, 5 },
  { 170, 1 },
  { 170, 1 },
  { 171, 1 },
  { 171, 3 },
  { 171, 5 },
  { 171, 4 },
  { 170, 1 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 218, 1 },
  { 218, 2 },
  { 171, 3 },
  { 171, 5 },
  { 171, 2 },
  { 171, 3 },
  { 171, 3 },
  { 171, 4 },
  { 171, 2 },
  { 171, 2 },
  { 171, 2 },
  { 171, 2 },
  { 219, 1 },
  { 219, 2 },
  { 171, 5 },
  { 171, 5 },
  { 221, 5 },
  { 221, 4 },
  { 222, 2 },
  { 222, 0 },
  { 220, 1 },
  { 220, 0 },
  { 205, 0 },
  { 204, 3 },
  { 204, 1 },
  { 147, 12 },
  { 223, 1 },
  { 223, 0 },
  { 175, 0 },
  { 175, 3 },
  { 185, 5 },
  { 185, 3 },
  { 224, 0 },
  { 224, 2 },
  { 147, 4 },
  { 167, 2 },
  { 168, 2 },
  { 189, 1 },
  { 189, 1 },
  { 189, 1 },
  { 194, 0 },
  { 194, 2 },
  { 194, 3 },
  { 225, 6 },
  { 225, 8 },
  { 142, 1 },
  { 143, 2 },
  { 143, 1 },
  { 144, 1 },
  { 144, 3 },
  { 145, 0 },
  { 149, 0 },
  { 149, 1 },
  { 149, 2 },
  { 151, 1 },
  { 151, 0 },
  { 147, 2 },
  { 158, 4 },
  { 158, 2 },
  { 150, 1 },
  { 150, 1 },
  { 150, 1 },
  { 164, 1 },
  { 165, 1 },
  { 166, 1 },
  { 166, 1 },
  { 163, 2 },
  { 163, 0 },
  { 169, 2 },
  { 159, 2 },
  { 181, 3 },
  { 181, 1 },
  { 182, 0 },
  { 186, 1 },
  { 188, 1 },
  { 192, 1 },
  { 193, 1 },
  { 206, 2 },
  { 207, 1 },
  { 171, 1 },
  { 205, 1 },
  { 167, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  sqlite3ParserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( yypParser->yyidx>yypParser->yyidxMax ){
      yypParser->yyidxMax = yypParser->yyidx;
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yyidx>=YYSTACKDEPTH-1 ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yyidx>=yypParser->yystksz-1 ){
      yyGrowStack(yypParser);
      if( yypParser->yyidx>=yypParser->yystksz-1 ){
        yyStackOverflow(yypParser);
        return;
      }
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* cmdx ::= cmd */
#line 130 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 1927 "parse.c"
        break;
      case 1: /* cmd ::= BEGIN transtype trans_opt */
#line 135 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy312);}
#line 1932 "parse.c"
        break;
      case 2: /* transtype ::= */
#line 140 "parse.y"
{yymsp[1].minor.yy312 = TK_DEFERRED;}
#line 1937 "parse.c"
        break;
      case 3: /* transtype ::= DEFERRED */
      case 4: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==4);
      case 5: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==5);
#line 141 "parse.y"
{yymsp[0].minor.yy312 = yymsp[0].major; /*A-overwrites-X*/}
#line 1944 "parse.c"
        break;
      case 6: /* cmd ::= COMMIT trans_opt */
      case 7: /* cmd ::= END trans_opt */ yytestcase(yyruleno==7);
#line 144 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 1950 "parse.c"
        break;
      case 8: /* cmd ::= ROLLBACK trans_opt */
#line 146 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 1955 "parse.c"
        break;
      case 9: /* cmd ::= SAVEPOINT nm */
#line 150 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 1962 "parse.c"
        break;
      case 10: /* cmd ::= RELEASE savepoint_opt nm */
#line 153 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 1969 "parse.c"
        break;
      case 11: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 156 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 1976 "parse.c"
        break;
      case 12: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
#line 163 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,yymsp[-4].minor.yy312,0,0,yymsp[-2].minor.yy312);
}
#line 1983 "parse.c"
        break;
      case 13: /* createkw ::= CREATE */
#line 166 "parse.y"
{disableLookaside(pParse);}
#line 1988 "parse.c"
        break;
      case 14: /* ifnotexists ::= */
      case 16: /* temp ::= */ yytestcase(yyruleno==16);
      case 19: /* table_options ::= */ yytestcase(yyruleno==19);
      case 39: /* autoinc ::= */ yytestcase(yyruleno==39);
      case 54: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==54);
      case 64: /* defer_subclause_opt ::= */ yytestcase(yyruleno==64);
      case 73: /* ifexists ::= */ yytestcase(yyruleno==73);
      case 81: /* distinct ::= */ yytestcase(yyruleno==81);
      case 190: /* collate ::= */ yytestcase(yyruleno==190);
#line 169 "parse.y"
{yymsp[1].minor.yy312 = 0;}
#line 2001 "parse.c"
        break;
      case 15: /* ifnotexists ::= IF NOT EXISTS */
#line 170 "parse.y"
{yymsp[-2].minor.yy312 = 1;}
#line 2006 "parse.c"
        break;
      case 17: /* create_table_args ::= LP columnlist conslist_opt RP table_options */
#line 176 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,yymsp[0].minor.yy312,0);
}
#line 2013 "parse.c"
        break;
      case 18: /* create_table_args ::= AS select */
#line 179 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy7);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy7);
}
#line 2021 "parse.c"
        break;
      case 20: /* table_options ::= WITHOUT nm */
#line 185 "parse.y"
{
  if( yymsp[0].minor.yy0.n==5 && sqlite3_strnicmp(yymsp[0].minor.yy0.z,"rowid",5)==0 ){
    yymsp[-1].minor.yy312 = TF_WithoutRowid | TF_NoVisibleRowid;
  }else{
    yymsp[-1].minor.yy312 = 0;
    sqlite3ErrorMsg(pParse, "unknown table option: %.*s", yymsp[0].minor.yy0.n, yymsp[0].minor.yy0.z);
  }
}
#line 2033 "parse.c"
        break;
      case 21: /* columnname ::= nm typetoken */
#line 195 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2038 "parse.c"
        break;
      case 22: /* typetoken ::= */
      case 57: /* conslist_opt ::= */ yytestcase(yyruleno==57);
      case 87: /* as ::= */ yytestcase(yyruleno==87);
#line 260 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2045 "parse.c"
        break;
      case 23: /* typetoken ::= typename LP signed RP */
#line 262 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2052 "parse.c"
        break;
      case 24: /* typetoken ::= typename LP signed COMMA signed RP */
#line 265 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2059 "parse.c"
        break;
      case 25: /* typename ::= typename ID|STRING */
#line 270 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2064 "parse.c"
        break;
      case 26: /* ccons ::= CONSTRAINT nm */
      case 59: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==59);
#line 279 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2070 "parse.c"
        break;
      case 27: /* ccons ::= DEFAULT term */
      case 29: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==29);
#line 280 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy306);}
#line 2076 "parse.c"
        break;
      case 28: /* ccons ::= DEFAULT LP expr RP */
#line 281 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy306);}
#line 2081 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT MINUS term */
#line 283 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy306.pExpr, 0, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy306.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2092 "parse.c"
        break;
      case 31: /* ccons ::= DEFAULT ID|INDEXED */
#line 290 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2101 "parse.c"
        break;
      case 32: /* ccons ::= NOT NULL onconf */
#line 300 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy312);}
#line 2106 "parse.c"
        break;
      case 33: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 302 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy312,yymsp[0].minor.yy312,yymsp[-2].minor.yy312);}
#line 2111 "parse.c"
        break;
      case 34: /* ccons ::= UNIQUE onconf */
#line 303 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,0,yymsp[0].minor.yy312,0,0,0,0);}
#line 2116 "parse.c"
        break;
      case 35: /* ccons ::= CHECK LP expr RP */
#line 304 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy306.pExpr);}
#line 2121 "parse.c"
        break;
      case 36: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 306 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy86,yymsp[0].minor.yy312);}
#line 2126 "parse.c"
        break;
      case 37: /* ccons ::= defer_subclause */
#line 307 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy312);}
#line 2131 "parse.c"
        break;
      case 38: /* ccons ::= COLLATE ID|STRING */
#line 308 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2136 "parse.c"
        break;
      case 40: /* autoinc ::= AUTOINCR */
#line 313 "parse.y"
{yymsp[0].minor.yy312 = 1;}
#line 2141 "parse.c"
        break;
      case 41: /* refargs ::= */
#line 321 "parse.y"
{ yymsp[1].minor.yy312 = OE_None*0x0101; /* EV: R-19803-45884 */}
#line 2146 "parse.c"
        break;
      case 42: /* refargs ::= refargs refarg */
#line 322 "parse.y"
{ yymsp[-1].minor.yy312 = (yymsp[-1].minor.yy312 & ~yymsp[0].minor.yy411.mask) | yymsp[0].minor.yy411.value; }
#line 2151 "parse.c"
        break;
      case 43: /* refarg ::= MATCH nm */
#line 324 "parse.y"
{ yymsp[-1].minor.yy411.value = 0;     yymsp[-1].minor.yy411.mask = 0x000000; }
#line 2156 "parse.c"
        break;
      case 44: /* refarg ::= ON INSERT refact */
#line 325 "parse.y"
{ yymsp[-2].minor.yy411.value = 0;     yymsp[-2].minor.yy411.mask = 0x000000; }
#line 2161 "parse.c"
        break;
      case 45: /* refarg ::= ON DELETE refact */
#line 326 "parse.y"
{ yymsp[-2].minor.yy411.value = yymsp[0].minor.yy312;     yymsp[-2].minor.yy411.mask = 0x0000ff; }
#line 2166 "parse.c"
        break;
      case 46: /* refarg ::= ON UPDATE refact */
#line 327 "parse.y"
{ yymsp[-2].minor.yy411.value = yymsp[0].minor.yy312<<8;  yymsp[-2].minor.yy411.mask = 0x00ff00; }
#line 2171 "parse.c"
        break;
      case 47: /* refact ::= SET NULL */
#line 329 "parse.y"
{ yymsp[-1].minor.yy312 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2176 "parse.c"
        break;
      case 48: /* refact ::= SET DEFAULT */
#line 330 "parse.y"
{ yymsp[-1].minor.yy312 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2181 "parse.c"
        break;
      case 49: /* refact ::= CASCADE */
#line 331 "parse.y"
{ yymsp[0].minor.yy312 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2186 "parse.c"
        break;
      case 50: /* refact ::= RESTRICT */
#line 332 "parse.y"
{ yymsp[0].minor.yy312 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2191 "parse.c"
        break;
      case 51: /* refact ::= NO ACTION */
#line 333 "parse.y"
{ yymsp[-1].minor.yy312 = OE_None;     /* EV: R-33326-45252 */}
#line 2196 "parse.c"
        break;
      case 52: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 335 "parse.y"
{yymsp[-2].minor.yy312 = 0;}
#line 2201 "parse.c"
        break;
      case 53: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 68: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==68);
      case 131: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==131);
#line 336 "parse.y"
{yymsp[-1].minor.yy312 = yymsp[0].minor.yy312;}
#line 2208 "parse.c"
        break;
      case 55: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 72: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==72);
      case 171: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==171);
      case 191: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==191);
#line 339 "parse.y"
{yymsp[-1].minor.yy312 = 1;}
#line 2216 "parse.c"
        break;
      case 56: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 340 "parse.y"
{yymsp[-1].minor.yy312 = 0;}
#line 2221 "parse.c"
        break;
      case 58: /* tconscomma ::= COMMA */
#line 346 "parse.y"
{pParse->constraintName.n = 0;}
#line 2226 "parse.c"
        break;
      case 60: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 350 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy86,yymsp[0].minor.yy312,yymsp[-2].minor.yy312,0);}
#line 2231 "parse.c"
        break;
      case 61: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 352 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[-2].minor.yy86,yymsp[0].minor.yy312,0,0,0,0);}
#line 2236 "parse.c"
        break;
      case 62: /* tcons ::= CHECK LP expr RP onconf */
#line 354 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy306.pExpr);}
#line 2241 "parse.c"
        break;
      case 63: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 356 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy86, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy86, yymsp[-1].minor.yy312);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy312);
}
#line 2249 "parse.c"
        break;
      case 65: /* onconf ::= */
      case 67: /* orconf ::= */ yytestcase(yyruleno==67);
#line 370 "parse.y"
{yymsp[1].minor.yy312 = OE_Default;}
#line 2255 "parse.c"
        break;
      case 66: /* onconf ::= ON CONFLICT resolvetype */
#line 371 "parse.y"
{yymsp[-2].minor.yy312 = yymsp[0].minor.yy312;}
#line 2260 "parse.c"
        break;
      case 69: /* resolvetype ::= IGNORE */
#line 375 "parse.y"
{yymsp[0].minor.yy312 = OE_Ignore;}
#line 2265 "parse.c"
        break;
      case 70: /* resolvetype ::= REPLACE */
      case 132: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==132);
#line 376 "parse.y"
{yymsp[0].minor.yy312 = OE_Replace;}
#line 2271 "parse.c"
        break;
      case 71: /* cmd ::= DROP TABLE ifexists fullname */
#line 380 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy55, 0, yymsp[-1].minor.yy312);
}
#line 2278 "parse.c"
        break;
      case 74: /* cmd ::= select */
#line 401 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy7, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy7);
}
#line 2287 "parse.c"
        break;
      case 75: /* select ::= with selectnowith */
#line 438 "parse.y"
{
  Select *p = yymsp[0].minor.yy7;
  if( p ){
    p->pWith = yymsp[-1].minor.yy211;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy211);
  }
  yymsp[-1].minor.yy7 = p; /*A-overwrites-W*/
}
#line 2301 "parse.c"
        break;
      case 76: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 479 "parse.y"
{
#if SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy7 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy86,yymsp[-5].minor.yy55,yymsp[-4].minor.yy314,yymsp[-3].minor.yy86,yymsp[-2].minor.yy314,yymsp[-1].minor.yy86,yymsp[-7].minor.yy312,yymsp[0].minor.yy240.pLimit,yymsp[0].minor.yy240.pOffset);
#if SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy7!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy7->zSelName), yymsp[-8].minor.yy7->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy7->zSelName), yymsp[-8].minor.yy7->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2335 "parse.c"
        break;
      case 77: /* values ::= VALUES LP nexprlist RP */
#line 513 "parse.y"
{
  yymsp[-3].minor.yy7 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy86,0,0,0,0,0,SF_Values,0,0);
}
#line 2342 "parse.c"
        break;
      case 78: /* values ::= values COMMA LP exprlist RP */
#line 516 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy7;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy86,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy7 = pRight;
  }else{
    yymsp[-4].minor.yy7 = pLeft;
  }
}
#line 2358 "parse.c"
        break;
      case 79: /* distinct ::= DISTINCT */
#line 533 "parse.y"
{yymsp[0].minor.yy312 = SF_Distinct;}
#line 2363 "parse.c"
        break;
      case 80: /* distinct ::= ALL */
#line 534 "parse.y"
{yymsp[0].minor.yy312 = SF_All;}
#line 2368 "parse.c"
        break;
      case 82: /* sclp ::= */
      case 108: /* orderby_opt ::= */ yytestcase(yyruleno==108);
      case 115: /* groupby_opt ::= */ yytestcase(yyruleno==115);
      case 180: /* exprlist ::= */ yytestcase(yyruleno==180);
      case 186: /* eidlist_opt ::= */ yytestcase(yyruleno==186);
#line 547 "parse.y"
{yymsp[1].minor.yy86 = 0;}
#line 2377 "parse.c"
        break;
      case 83: /* selcollist ::= sclp expr as */
#line 548 "parse.y"
{
   yymsp[-2].minor.yy86 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy86, yymsp[-1].minor.yy306.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy86, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy86,&yymsp[-1].minor.yy306);
}
#line 2386 "parse.c"
        break;
      case 84: /* selcollist ::= sclp STAR */
#line 553 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy86 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy86, p);
}
#line 2394 "parse.c"
        break;
      case 85: /* selcollist ::= sclp nm DOT STAR */
#line 557 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0, &yymsp[0].minor.yy0);
  Expr *pLeft = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight, 0);
  yymsp[-3].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy86, pDot);
}
#line 2404 "parse.c"
        break;
      case 86: /* as ::= AS nm */
      case 95: /* dbnm ::= DOT nm */ yytestcase(yyruleno==95);
      case 193: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==193);
      case 194: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==194);
#line 568 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2412 "parse.c"
        break;
      case 88: /* from ::= */
#line 582 "parse.y"
{yymsp[1].minor.yy55 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy55));}
#line 2417 "parse.c"
        break;
      case 89: /* from ::= FROM seltablist */
#line 583 "parse.y"
{
  yymsp[-1].minor.yy55 = yymsp[0].minor.yy55;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy55);
}
#line 2425 "parse.c"
        break;
      case 90: /* stl_prefix ::= seltablist joinop */
#line 591 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy55 && yymsp[-1].minor.yy55->nSrc>0) ) yymsp[-1].minor.yy55->a[yymsp[-1].minor.yy55->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy312;
}
#line 2432 "parse.c"
        break;
      case 91: /* stl_prefix ::= */
#line 594 "parse.y"
{yymsp[1].minor.yy55 = 0;}
#line 2437 "parse.c"
        break;
      case 92: /* seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt */
#line 596 "parse.y"
{
  yymsp[-6].minor.yy55 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy55,&yymsp[-5].minor.yy0,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy314,yymsp[0].minor.yy224);
  sqlite3SrcListIndexedBy(pParse, yymsp[-6].minor.yy55, &yymsp[-2].minor.yy0);
}
#line 2445 "parse.c"
        break;
      case 93: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt */
#line 601 "parse.y"
{
  yymsp[-8].minor.yy55 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-8].minor.yy55,&yymsp[-7].minor.yy0,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy314,yymsp[0].minor.yy224);
  sqlite3SrcListFuncArgs(pParse, yymsp[-8].minor.yy55, yymsp[-4].minor.yy86);
}
#line 2453 "parse.c"
        break;
      case 94: /* dbnm ::= */
      case 103: /* indexed_opt ::= */ yytestcase(yyruleno==103);
#line 636 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2459 "parse.c"
        break;
      case 96: /* fullname ::= nm dbnm */
#line 642 "parse.y"
{yymsp[-1].minor.yy55 = sqlite3SrcListAppend(pParse->db,0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2464 "parse.c"
        break;
      case 97: /* joinop ::= COMMA|JOIN */
#line 645 "parse.y"
{ yymsp[0].minor.yy312 = JT_INNER; }
#line 2469 "parse.c"
        break;
      case 98: /* joinop ::= JOIN_KW JOIN */
#line 647 "parse.y"
{yymsp[-1].minor.yy312 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2474 "parse.c"
        break;
      case 99: /* joinop ::= JOIN_KW nm JOIN */
#line 649 "parse.y"
{yymsp[-2].minor.yy312 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2479 "parse.c"
        break;
      case 100: /* joinop ::= JOIN_KW nm nm JOIN */
#line 651 "parse.y"
{yymsp[-3].minor.yy312 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2484 "parse.c"
        break;
      case 101: /* on_opt ::= ON expr */
      case 118: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==118);
      case 125: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==125);
      case 176: /* case_else ::= ELSE expr */ yytestcase(yyruleno==176);
#line 655 "parse.y"
{yymsp[-1].minor.yy314 = yymsp[0].minor.yy306.pExpr;}
#line 2492 "parse.c"
        break;
      case 102: /* on_opt ::= */
      case 117: /* having_opt ::= */ yytestcase(yyruleno==117);
      case 124: /* where_opt ::= */ yytestcase(yyruleno==124);
      case 177: /* case_else ::= */ yytestcase(yyruleno==177);
      case 179: /* case_operand ::= */ yytestcase(yyruleno==179);
#line 656 "parse.y"
{yymsp[1].minor.yy314 = 0;}
#line 2501 "parse.c"
        break;
      case 104: /* indexed_opt ::= INDEXED BY nm */
#line 670 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2506 "parse.c"
        break;
      case 105: /* indexed_opt ::= NOT INDEXED */
#line 671 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2511 "parse.c"
        break;
      case 106: /* using_opt ::= USING LP idlist RP */
#line 675 "parse.y"
{yymsp[-3].minor.yy224 = yymsp[-1].minor.yy224;}
#line 2516 "parse.c"
        break;
      case 107: /* using_opt ::= */
      case 133: /* idlist_opt ::= */ yytestcase(yyruleno==133);
#line 676 "parse.y"
{yymsp[1].minor.yy224 = 0;}
#line 2522 "parse.c"
        break;
      case 109: /* orderby_opt ::= ORDER BY sortlist */
      case 116: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==116);
#line 690 "parse.y"
{yymsp[-2].minor.yy86 = yymsp[0].minor.yy86;}
#line 2528 "parse.c"
        break;
      case 110: /* sortlist ::= sortlist COMMA expr sortorder */
#line 691 "parse.y"
{
  yymsp[-3].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy86,yymsp[-1].minor.yy306.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy86,yymsp[0].minor.yy312);
}
#line 2536 "parse.c"
        break;
      case 111: /* sortlist ::= expr sortorder */
#line 695 "parse.y"
{
  yymsp[-1].minor.yy86 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy306.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy86,yymsp[0].minor.yy312);
}
#line 2544 "parse.c"
        break;
      case 112: /* sortorder ::= ASC */
#line 702 "parse.y"
{yymsp[0].minor.yy312 = SQLITE_SO_ASC;}
#line 2549 "parse.c"
        break;
      case 113: /* sortorder ::= DESC */
#line 703 "parse.y"
{yymsp[0].minor.yy312 = SQLITE_SO_DESC;}
#line 2554 "parse.c"
        break;
      case 114: /* sortorder ::= */
#line 704 "parse.y"
{yymsp[1].minor.yy312 = SQLITE_SO_UNDEFINED;}
#line 2559 "parse.c"
        break;
      case 119: /* limit_opt ::= */
#line 729 "parse.y"
{yymsp[1].minor.yy240.pLimit = 0; yymsp[1].minor.yy240.pOffset = 0;}
#line 2564 "parse.c"
        break;
      case 120: /* limit_opt ::= LIMIT expr */
#line 730 "parse.y"
{yymsp[-1].minor.yy240.pLimit = yymsp[0].minor.yy306.pExpr; yymsp[-1].minor.yy240.pOffset = 0;}
#line 2569 "parse.c"
        break;
      case 121: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 732 "parse.y"
{yymsp[-3].minor.yy240.pLimit = yymsp[-2].minor.yy306.pExpr; yymsp[-3].minor.yy240.pOffset = yymsp[0].minor.yy306.pExpr;}
#line 2574 "parse.c"
        break;
      case 122: /* limit_opt ::= LIMIT expr COMMA expr */
#line 734 "parse.y"
{yymsp[-3].minor.yy240.pOffset = yymsp[-2].minor.yy306.pExpr; yymsp[-3].minor.yy240.pLimit = yymsp[0].minor.yy306.pExpr;}
#line 2579 "parse.c"
        break;
      case 123: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 748 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy211, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy55, &yymsp[-1].minor.yy0);
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy55,yymsp[0].minor.yy314);
}
#line 2588 "parse.c"
        break;
      case 126: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 775 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy211, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy55, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy86,"set list"); 
  sqlite3Update(pParse,yymsp[-4].minor.yy55,yymsp[-1].minor.yy86,yymsp[0].minor.yy314,yymsp[-5].minor.yy312);
}
#line 2598 "parse.c"
        break;
      case 127: /* setlist ::= setlist COMMA nm EQ expr */
#line 786 "parse.y"
{
  yymsp[-4].minor.yy86 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy86, yymsp[0].minor.yy306.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy86, &yymsp[-2].minor.yy0, 1);
}
#line 2606 "parse.c"
        break;
      case 128: /* setlist ::= nm EQ expr */
#line 790 "parse.y"
{
  yylhsminor.yy86 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy306.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy86, &yymsp[-2].minor.yy0, 1);
}
#line 2614 "parse.c"
  yymsp[-2].minor.yy86 = yylhsminor.yy86;
        break;
      case 129: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 797 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy211, 1);
  sqlite3Insert(pParse, yymsp[-2].minor.yy55, yymsp[0].minor.yy7, yymsp[-1].minor.yy224, yymsp[-4].minor.yy312);
}
#line 2623 "parse.c"
        break;
      case 130: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 802 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy211, 1);
  sqlite3Insert(pParse, yymsp[-3].minor.yy55, 0, yymsp[-2].minor.yy224, yymsp[-5].minor.yy312);
}
#line 2631 "parse.c"
        break;
      case 134: /* idlist_opt ::= LP idlist RP */
#line 817 "parse.y"
{yymsp[-2].minor.yy224 = yymsp[-1].minor.yy224;}
#line 2636 "parse.c"
        break;
      case 135: /* idlist ::= idlist COMMA nm */
#line 819 "parse.y"
{yymsp[-2].minor.yy224 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy224,&yymsp[0].minor.yy0);}
#line 2641 "parse.c"
        break;
      case 136: /* idlist ::= nm */
#line 821 "parse.y"
{yymsp[0].minor.yy224 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 2646 "parse.c"
        break;
      case 137: /* expr ::= LP expr RP */
#line 854 "parse.y"
{spanSet(&yymsp[-2].minor.yy306,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy306.pExpr = yymsp[-1].minor.yy306.pExpr;}
#line 2651 "parse.c"
        break;
      case 138: /* term ::= NULL */
      case 143: /* term ::= INTEGER|FLOAT|BLOB */ yytestcase(yyruleno==143);
      case 144: /* term ::= STRING */ yytestcase(yyruleno==144);
#line 855 "parse.y"
{spanExpr(&yymsp[0].minor.yy306,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 2658 "parse.c"
        break;
      case 139: /* expr ::= ID|INDEXED */
      case 140: /* expr ::= JOIN_KW */ yytestcase(yyruleno==140);
#line 856 "parse.y"
{spanExpr(&yymsp[0].minor.yy306,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2664 "parse.c"
        break;
      case 141: /* expr ::= nm DOT nm */
#line 858 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  spanSet(&yymsp[-2].minor.yy306,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy306.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2, 0);
}
#line 2674 "parse.c"
        break;
      case 142: /* expr ::= nm DOT nm DOT nm */
#line 864 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-4].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp3 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3, 0);
  spanSet(&yymsp[-4].minor.yy306,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-4].minor.yy306.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4, 0);
}
#line 2686 "parse.c"
        break;
      case 145: /* expr ::= VARIABLE */
#line 874 "parse.y"
{
  if( !(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1])) ){
    spanExpr(&yymsp[0].minor.yy306, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy306.pExpr);
  }else{
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
    assert( t.n>=2 );
    spanSet(&yymsp[0].minor.yy306, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy306.pExpr = 0;
    }else{
      yymsp[0].minor.yy306.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0, &t);
      if( yymsp[0].minor.yy306.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy306.pExpr->iTable);
    }
  }
}
#line 2710 "parse.c"
        break;
      case 146: /* expr ::= expr COLLATE ID|STRING */
#line 894 "parse.y"
{
  yymsp[-2].minor.yy306.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy306.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy306.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 2718 "parse.c"
        break;
      case 147: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 904 "parse.y"
{
  if( yymsp[-1].minor.yy86 && yymsp[-1].minor.yy86->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy306.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy86, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy306,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy312==SF_Distinct && yylhsminor.yy306.pExpr ){
    yylhsminor.yy306.pExpr->flags |= EP_Distinct;
  }
}
#line 2732 "parse.c"
  yymsp[-4].minor.yy306 = yylhsminor.yy306;
        break;
      case 148: /* expr ::= ID|INDEXED LP STAR RP */
#line 914 "parse.y"
{
  yylhsminor.yy306.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy306,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2741 "parse.c"
  yymsp[-3].minor.yy306 = yylhsminor.yy306;
        break;
      case 149: /* term ::= CTIME_KW */
#line 918 "parse.y"
{
  yylhsminor.yy306.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy306, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2750 "parse.c"
  yymsp[0].minor.yy306 = yylhsminor.yy306;
        break;
      case 150: /* expr ::= expr AND expr */
      case 151: /* expr ::= expr OR expr */ yytestcase(yyruleno==151);
      case 152: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==152);
      case 153: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==153);
      case 154: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==154);
      case 155: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==155);
      case 156: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==156);
      case 157: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==157);
#line 947 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy306,&yymsp[0].minor.yy306);}
#line 2763 "parse.c"
        break;
      case 158: /* likeop ::= LIKE_KW|MATCH */
#line 960 "parse.y"
{yymsp[0].minor.yy286.eOperator = yymsp[0].minor.yy0; yymsp[0].minor.yy286.bNot = 0;/*A-overwrites-X*/}
#line 2768 "parse.c"
        break;
      case 159: /* likeop ::= NOT LIKE_KW|MATCH */
#line 961 "parse.y"
{yymsp[-1].minor.yy286.eOperator = yymsp[0].minor.yy0; yymsp[-1].minor.yy286.bNot = 1;}
#line 2773 "parse.c"
        break;
      case 160: /* expr ::= expr likeop expr */
#line 962 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy306.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy306.pExpr);
  yymsp[-2].minor.yy306.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy286.eOperator);
  exprNot(pParse, yymsp[-1].minor.yy286.bNot, &yymsp[-2].minor.yy306);
  yymsp[-2].minor.yy306.zEnd = yymsp[0].minor.yy306.zEnd;
  if( yymsp[-2].minor.yy306.pExpr ) yymsp[-2].minor.yy306.pExpr->flags |= EP_InfixFunc;
}
#line 2786 "parse.c"
        break;
      case 161: /* expr ::= expr likeop expr ESCAPE expr */
#line 971 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy306.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy306.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy306.pExpr);
  yymsp[-4].minor.yy306.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy286.eOperator);
  exprNot(pParse, yymsp[-3].minor.yy286.bNot, &yymsp[-4].minor.yy306);
  yymsp[-4].minor.yy306.zEnd = yymsp[0].minor.yy306.zEnd;
  if( yymsp[-4].minor.yy306.pExpr ) yymsp[-4].minor.yy306.pExpr->flags |= EP_InfixFunc;
}
#line 2800 "parse.c"
        break;
      case 162: /* expr ::= expr ISNULL|NOTNULL */
#line 996 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy306,&yymsp[0].minor.yy0);}
#line 2805 "parse.c"
        break;
      case 163: /* expr ::= expr NOT NULL */
#line 997 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy306,&yymsp[0].minor.yy0);}
#line 2810 "parse.c"
        break;
      case 164: /* expr ::= expr IS expr */
#line 1018 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy306,&yymsp[0].minor.yy306);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy306.pExpr, yymsp[-2].minor.yy306.pExpr, TK_ISNULL);
}
#line 2818 "parse.c"
        break;
      case 165: /* expr ::= expr IS NOT expr */
#line 1022 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy306,&yymsp[0].minor.yy306);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy306.pExpr, yymsp[-3].minor.yy306.pExpr, TK_NOTNULL);
}
#line 2826 "parse.c"
        break;
      case 166: /* expr ::= NOT expr */
      case 167: /* expr ::= BITNOT expr */ yytestcase(yyruleno==167);
#line 1046 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy306,pParse,yymsp[-1].major,&yymsp[0].minor.yy306,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 2832 "parse.c"
        break;
      case 168: /* expr ::= MINUS expr */
#line 1050 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy306,pParse,TK_UMINUS,&yymsp[0].minor.yy306,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 2837 "parse.c"
        break;
      case 169: /* expr ::= PLUS expr */
#line 1052 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy306,pParse,TK_UPLUS,&yymsp[0].minor.yy306,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 2842 "parse.c"
        break;
      case 170: /* between_op ::= BETWEEN */
#line 1055 "parse.y"
{yymsp[0].minor.yy312 = 0;}
#line 2847 "parse.c"
        break;
      case 172: /* expr ::= expr between_op expr AND expr */
#line 1057 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy306.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy306.pExpr);
  yymsp[-4].minor.yy306.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy306.pExpr, 0, 0);
  if( yymsp[-4].minor.yy306.pExpr ){
    yymsp[-4].minor.yy306.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy312, &yymsp[-4].minor.yy306);
  yymsp[-4].minor.yy306.zEnd = yymsp[0].minor.yy306.zEnd;
}
#line 2863 "parse.c"
        break;
      case 173: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1152 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy306,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy306.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy314, 0, 0);
  if( yymsp[-4].minor.yy306.pExpr ){
    yymsp[-4].minor.yy306.pExpr->x.pList = yymsp[-1].minor.yy314 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy86,yymsp[-1].minor.yy314) : yymsp[-2].minor.yy86;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy306.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy86);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy314);
  }
}
#line 2878 "parse.c"
        break;
      case 174: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1165 "parse.y"
{
  yymsp[-4].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy86, yymsp[-2].minor.yy306.pExpr);
  yymsp[-4].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy86, yymsp[0].minor.yy306.pExpr);
}
#line 2886 "parse.c"
        break;
      case 175: /* case_exprlist ::= WHEN expr THEN expr */
#line 1169 "parse.y"
{
  yymsp[-3].minor.yy86 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy306.pExpr);
  yymsp[-3].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy86, yymsp[0].minor.yy306.pExpr);
}
#line 2894 "parse.c"
        break;
      case 178: /* case_operand ::= expr */
#line 1179 "parse.y"
{yymsp[0].minor.yy314 = yymsp[0].minor.yy306.pExpr; /*A-overwrites-X*/}
#line 2899 "parse.c"
        break;
      case 181: /* nexprlist ::= nexprlist COMMA expr */
#line 1190 "parse.y"
{yymsp[-2].minor.yy86 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy86,yymsp[0].minor.yy306.pExpr);}
#line 2904 "parse.c"
        break;
      case 182: /* nexprlist ::= expr */
#line 1192 "parse.y"
{yymsp[0].minor.yy86 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy306.pExpr); /*A-overwrites-Y*/}
#line 2909 "parse.c"
        break;
      case 183: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
#line 1198 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy86, yymsp[-10].minor.yy312,
                      &yymsp[-11].minor.yy0, yymsp[0].minor.yy314, SQLITE_SO_ASC, yymsp[-8].minor.yy312);
}
#line 2918 "parse.c"
        break;
      case 184: /* uniqueflag ::= UNIQUE */
      case 196: /* raisetype ::= ABORT */ yytestcase(yyruleno==196);
#line 1205 "parse.y"
{yymsp[0].minor.yy312 = OE_Abort;}
#line 2924 "parse.c"
        break;
      case 185: /* uniqueflag ::= */
#line 1206 "parse.y"
{yymsp[1].minor.yy312 = OE_None;}
#line 2929 "parse.c"
        break;
      case 187: /* eidlist_opt ::= LP eidlist RP */
#line 1255 "parse.y"
{yymsp[-2].minor.yy86 = yymsp[-1].minor.yy86;}
#line 2934 "parse.c"
        break;
      case 188: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1256 "parse.y"
{
  yymsp[-4].minor.yy86 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy86, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy312, yymsp[0].minor.yy312);
}
#line 2941 "parse.c"
        break;
      case 189: /* eidlist ::= nm collate sortorder */
#line 1259 "parse.y"
{
  yymsp[-2].minor.yy86 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy312, yymsp[0].minor.yy312); /*A-overwrites-Y*/
}
#line 2948 "parse.c"
        break;
      case 192: /* cmd ::= DROP INDEX ifexists fullname */
#line 1270 "parse.y"
{sqlite3DropIndex(pParse, yymsp[0].minor.yy55, yymsp[-1].minor.yy312);}
#line 2953 "parse.c"
        break;
      case 195: /* raisetype ::= ROLLBACK */
#line 1420 "parse.y"
{yymsp[0].minor.yy312 = OE_Rollback;}
#line 2958 "parse.c"
        break;
      case 197: /* raisetype ::= FAIL */
#line 1422 "parse.y"
{yymsp[0].minor.yy312 = OE_Fail;}
#line 2963 "parse.c"
        break;
      case 198: /* with ::= */
#line 1507 "parse.y"
{yymsp[1].minor.yy211 = 0;}
#line 2968 "parse.c"
        break;
      case 199: /* with ::= WITH wqlist */
#line 1509 "parse.y"
{ yymsp[-1].minor.yy211 = yymsp[0].minor.yy211; }
#line 2973 "parse.c"
        break;
      case 200: /* with ::= WITH RECURSIVE wqlist */
#line 1510 "parse.y"
{ yymsp[-2].minor.yy211 = yymsp[0].minor.yy211; }
#line 2978 "parse.c"
        break;
      case 201: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1512 "parse.y"
{
  yymsp[-5].minor.yy211 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy86, yymsp[-1].minor.yy7); /*A-overwrites-X*/
}
#line 2985 "parse.c"
        break;
      case 202: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1515 "parse.y"
{
  yymsp[-7].minor.yy211 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy211, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy86, yymsp[-1].minor.yy7);
}
#line 2992 "parse.c"
        break;
      default:
      /* (203) input ::= cmdlist */ yytestcase(yyruleno==203);
      /* (204) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==204);
      /* (205) cmdlist ::= ecmd */ yytestcase(yyruleno==205);
      /* (206) ecmd ::= SEMI */ yytestcase(yyruleno==206);
      /* (207) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==207);
      /* (208) explain ::= */ yytestcase(yyruleno==208);
      /* (209) trans_opt ::= */ yytestcase(yyruleno==209);
      /* (210) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==210);
      /* (211) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==211);
      /* (212) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==212);
      /* (213) savepoint_opt ::= */ yytestcase(yyruleno==213);
      /* (214) cmd ::= create_table create_table_args */ yytestcase(yyruleno==214);
      /* (215) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==215);
      /* (216) columnlist ::= columnname carglist */ yytestcase(yyruleno==216);
      /* (217) nm ::= ID|INDEXED */ yytestcase(yyruleno==217);
      /* (218) nm ::= STRING */ yytestcase(yyruleno==218);
      /* (219) nm ::= JOIN_KW */ yytestcase(yyruleno==219);
      /* (220) typetoken ::= typename */ yytestcase(yyruleno==220);
      /* (221) typename ::= ID|STRING */ yytestcase(yyruleno==221);
      /* (222) signed ::= plus_num */ yytestcase(yyruleno==222);
      /* (223) signed ::= minus_num */ yytestcase(yyruleno==223);
      /* (224) carglist ::= carglist ccons */ yytestcase(yyruleno==224);
      /* (225) carglist ::= */ yytestcase(yyruleno==225);
      /* (226) ccons ::= NULL onconf */ yytestcase(yyruleno==226);
      /* (227) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==227);
      /* (228) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==228);
      /* (229) conslist ::= tcons */ yytestcase(yyruleno==229);
      /* (230) tconscomma ::= */ yytestcase(yyruleno==230);
      /* (231) defer_subclause_opt ::= defer_subclause */ yytestcase(yyruleno==231);
      /* (232) resolvetype ::= raisetype */ yytestcase(yyruleno==232);
      /* (233) selectnowith ::= oneselect */ yytestcase(yyruleno==233);
      /* (234) oneselect ::= values */ yytestcase(yyruleno==234);
      /* (235) sclp ::= selcollist COMMA */ yytestcase(yyruleno==235);
      /* (236) as ::= ID|STRING */ yytestcase(yyruleno==236);
      /* (237) expr ::= term */ yytestcase(yyruleno==237);
      /* (238) exprlist ::= nexprlist */ yytestcase(yyruleno==238);
      /* (239) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==239);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    yypParser->yyidx -= yysize - 1;
    yymsp -= yysize-1;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yyidx -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  sqlite3ParserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 32 "parse.y"

  UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
  assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
  sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
#line 3091 "parse.c"
/************ End %syntax_error code ******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "sqlite3ParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yypParser->yystksz <=0 ){
      yyStackOverflow(yypParser);
      return;
    }
#endif
    yypParser->yyidx = 0;
#ifndef YYNOERRORRECOVERY
    yypParser->yyerrcnt = -1;
#endif
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sInitialize. Empty stack. State 0\n",
              yyTracePrompt);
    }
#endif
  }
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  sqlite3ParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      if( yyact > YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yypParser->yystack[yypParser->yyidx].stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
#ifndef NDEBUG
  if( yyTraceFILE ){
    int i;
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=1; i<=yypParser->yyidx; i++)
      fprintf(yyTraceFILE,"%c%s", i==1 ? '[' : ' ', 
              yyTokenName[yypParser->yystack[i].major]);
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
