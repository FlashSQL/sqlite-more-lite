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
#define YYNOCODE 251
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 96
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  struct LimitVal yy64;
  Expr* yy122;
  Select* yy159;
  IdList* yy180;
  struct {int value; int mask;} yy207;
  struct LikeOp yy318;
  TriggerStep* yy327;
  With* yy331;
  ExprSpan yy342;
  SrcList* yy347;
  int yy392;
  struct TrigEvent yy410;
  ExprList* yy442;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             440
#define YYNRULE              326
#define YY_MAX_SHIFT         439
#define YY_MIN_SHIFTREDUCE   649
#define YY_MAX_SHIFTREDUCE   974
#define YY_MIN_REDUCE        975
#define YY_MAX_REDUCE        1300
#define YY_ERROR_ACTION      1301
#define YY_ACCEPT_ACTION     1302
#define YY_NO_ACTION         1303
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
#define YY_ACTTAB_COUNT (1501)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   315,  810,  339,  804,    5,  194,  194,  798,   92,   93,
 /*    10 */    83,  819,  819,  831,  834,  823,  823,   90,   90,   91,
 /*    20 */    91,   91,   91,  290,   89,   89,   89,   89,   88,   88,
 /*    30 */    87,   87,   87,   86,  339,  315,  952,  952,  803,  803,
 /*    40 */   803,  922,  342,   92,   93,   83,  819,  819,  831,  834,
 /*    50 */   823,  823,   90,   90,   91,   91,   91,   91,  123,   89,
 /*    60 */    89,   89,   89,   88,   88,   87,   87,   87,   86,  339,
 /*    70 */    88,   88,   87,   87,   87,   86,  339,  772,  952,  952,
 /*    80 */   315,   87,   87,   87,   86,  339,  773,   68,   92,   93,
 /*    90 */    83,  819,  819,  831,  834,  823,  823,   90,   90,   91,
 /*   100 */    91,   91,   91,  434,   89,   89,   89,   89,   88,   88,
 /*   110 */    87,   87,   87,   86,  339, 1302,  146,  921,    2,  315,
 /*   120 */   427,   24,  679,  953,   48,   86,  339,   92,   93,   83,
 /*   130 */   819,  819,  831,  834,  823,  823,   90,   90,   91,   91,
 /*   140 */    91,   91,   94,   89,   89,   89,   89,   88,   88,   87,
 /*   150 */    87,   87,   86,  339,  933,  933,  315,  259,  412,  398,
 /*   160 */   396,   57,  733,  733,   92,   93,   83,  819,  819,  831,
 /*   170 */   834,  823,  823,   90,   90,   91,   91,   91,   91,   56,
 /*   180 */    89,   89,   89,   89,   88,   88,   87,   87,   87,   86,
 /*   190 */   339,  315, 1245,  922,  342,  268,  934,  935,  241,   92,
 /*   200 */    93,   83,  819,  819,  831,  834,  823,  823,   90,   90,
 /*   210 */    91,   91,   91,   91,  291,   89,   89,   89,   89,   88,
 /*   220 */    88,   87,   87,   87,   86,  339,  315,  913, 1295,  682,
 /*   230 */   687, 1295,  233,  397,   92,   93,   83,  819,  819,  831,
 /*   240 */   834,  823,  823,   90,   90,   91,   91,   91,   91,  326,
 /*   250 */    89,   89,   89,   89,   88,   88,   87,   87,   87,   86,
 /*   260 */   339,  315,   85,   82,  168,  680,  431,  938,  939,   92,
 /*   270 */    93,   83,  819,  819,  831,  834,  823,  823,   90,   90,
 /*   280 */    91,   91,   91,   91,  291,   89,   89,   89,   89,   88,
 /*   290 */    88,   87,   87,   87,   86,  339,  315,  319,  913, 1296,
 /*   300 */   797,  911, 1296,  681,   92,   93,   83,  819,  819,  831,
 /*   310 */   834,  823,  823,   90,   90,   91,   91,   91,   91,  335,
 /*   320 */    89,   89,   89,   89,   88,   88,   87,   87,   87,   86,
 /*   330 */   339,  315,  876,  876,  373,   85,   82,  168,  944,   92,
 /*   340 */    93,   83,  819,  819,  831,  834,  823,  823,   90,   90,
 /*   350 */    91,   91,   91,   91,  896,   89,   89,   89,   89,   88,
 /*   360 */    88,   87,   87,   87,   86,  339,  315,  370,  307,  973,
 /*   370 */   367,    1,  911,  433,   92,   93,   83,  819,  819,  831,
 /*   380 */   834,  823,  823,   90,   90,   91,   91,   91,   91,  189,
 /*   390 */    89,   89,   89,   89,   88,   88,   87,   87,   87,   86,
 /*   400 */   339,  315,  720,  948,  933,  933,  149,  718,  948,   92,
 /*   410 */    93,   83,  819,  819,  831,  834,  823,  823,   90,   90,
 /*   420 */    91,   91,   91,   91,  434,   89,   89,   89,   89,   88,
 /*   430 */    88,   87,   87,   87,   86,  339,  338,  938,  939,  947,
 /*   440 */   694,  940,  974,  315,  953,   48,  934,  935,  715,  689,
 /*   450 */    71,   92,   93,   83,  819,  819,  831,  834,  823,  823,
 /*   460 */    90,   90,   91,   91,   91,   91,  320,   89,   89,   89,
 /*   470 */    89,   88,   88,   87,   87,   87,   86,  339,  315,  412,
 /*   480 */   403,  820,  820,  832,  835,   74,   92,   81,   83,  819,
 /*   490 */   819,  831,  834,  823,  823,   90,   90,   91,   91,   91,
 /*   500 */    91,  698,   89,   89,   89,   89,   88,   88,   87,   87,
 /*   510 */    87,   86,  339,  315,  259,  654,  655,  656,  393,  111,
 /*   520 */   331,  153,   93,   83,  819,  819,  831,  834,  823,  823,
 /*   530 */    90,   90,   91,   91,   91,   91,  434,   89,   89,   89,
 /*   540 */    89,   88,   88,   87,   87,   87,   86,  339,  315,  188,
 /*   550 */   187,  186,  824,  937,  328,  219,  953,   48,   83,  819,
 /*   560 */   819,  831,  834,  823,  823,   90,   90,   91,   91,   91,
 /*   570 */    91,  956,   89,   89,   89,   89,   88,   88,   87,   87,
 /*   580 */    87,   86,  339,   79,  429,  738,    3, 1174,  955,  348,
 /*   590 */   737,  332,  792,  933,  933,  937,   79,  429,  730,    3,
 /*   600 */   203,  160,  278,  391,  273,  390,  190,  892,  434,  400,
 /*   610 */   741,   76,   77,  271,  287,  253,  353,  242,   78,  340,
 /*   620 */   340,   85,   82,  168,   76,   77,  233,  397,  953,   48,
 /*   630 */   432,   78,  340,  340,  277,  934,  935,  185,  439,  651,
 /*   640 */   388,  385,  384,  432,  234,  276,  107,  418,  349,  337,
 /*   650 */   336,  383,  893,  728,  215,  949,  123,  971,  308,  810,
 /*   660 */   418,  436,  435,  412,  394,  798,  400,  873,  894,  123,
 /*   670 */   721,  872,  810,  889,  436,  435,  215,  949,  798,  351,
 /*   680 */   722,  697,  380,  434,  771,  371,   22,  434,  400,   79,
 /*   690 */   429,  232,    3,  189,  413,  870,  803,  803,  803,  805,
 /*   700 */    18,   54,  148,  953,   48,  956,  113,  953,    9,  803,
 /*   710 */   803,  803,  805,   18,  310,  123,  748,   76,   77,  742,
 /*   720 */   123,  325,  955,  866,   78,  340,  340,  113,  350,  359,
 /*   730 */    85,   82,  168,  343,  960,  960,  432,  770,  412,  414,
 /*   740 */   407,   23, 1240, 1240,   79,  429,  357,    3,  166,   91,
 /*   750 */    91,   91,   91,  418,   89,   89,   89,   89,   88,   88,
 /*   760 */    87,   87,   87,   86,  339,  810,  434,  436,  435,  792,
 /*   770 */   320,  798,   76,   77,  789,  271,  123,  434,  360,   78,
 /*   780 */   340,  340,  864,   85,   82,  168,  953,    9,  395,  743,
 /*   790 */   360,  432,  253,  358,  252,  933,  933,  953,   30,  889,
 /*   800 */   327,  216,  803,  803,  803,  805,   18,  113,  418,   89,
 /*   810 */    89,   89,   89,   88,   88,   87,   87,   87,   86,  339,
 /*   820 */   810,  113,  436,  435,  792,  185,  798,  288,  388,  385,
 /*   830 */   384,  123,  113,  920,    2,  796,  696,  934,  935,  383,
 /*   840 */    69,  429,  434,    3,  218,  110,  738,  253,  358,  252,
 /*   850 */   434,  737,  933,  933,  892,  359,  222,  803,  803,  803,
 /*   860 */   805,   18,  953,   47,  933,  933,  933,  933,   76,   77,
 /*   870 */   953,    9,  366,  904,  217,   78,  340,  340,  677,  305,
 /*   880 */   304,  303,  206,  301,  224,  259,  664,  432,  337,  336,
 /*   890 */   434,  228,  247,  144,  934,  935,  933,  933,  667,  893,
 /*   900 */   324, 1259,   96,  434,  418,  796,  934,  935,  934,  935,
 /*   910 */   953,   48,  401,  148,  289,  894,  810,  417,  436,  435,
 /*   920 */   677,  759,  798,  953,    9,  314,  220,  162,  161,  170,
 /*   930 */   402,  239,  953,    8,  194,  683,  683,  410,  934,  935,
 /*   940 */   238,  959,  933,  933,  225,  408,  945,  365,  957,  212,
 /*   950 */   958,  172,  757,  803,  803,  803,  805,   18,  173,  365,
 /*   960 */   176,  123,  171,  113,  244,  952,  246,  434,  356,  796,
 /*   970 */   372,  365,  236,  960,  960,  810,  290,  804,  191,  165,
 /*   980 */   852,  798,  259,  316,  934,  935,  237,  953,   34,  404,
 /*   990 */    91,   91,   91,   91,   84,   89,   89,   89,   89,   88,
 /*  1000 */    88,   87,   87,   87,   86,  339,  701,  952,  434,  240,
 /*  1010 */   347,  758,  803,  803,  803,  434,  245, 1179,  434,  389,
 /*  1020 */   434,  376,  434,  895,  167,  434,  405,  702,  953,   35,
 /*  1030 */   673,  321,  221,  434,  333,  953,   11,  434,  953,   26,
 /*  1040 */   953,   36,  953,   37,  251,  953,   38,  434,  259,  434,
 /*  1050 */   757,  434,  329,  953,   27,  434,  223,  953,   28,  434,
 /*  1060 */   690,  434,   67,  434,   65,  434,  862,  953,   39,  953,
 /*  1070 */    40,  953,   41,  423,  434,  953,   10,  434,  772,  953,
 /*  1080 */    42,  953,   98,  953,   43,  953,   44,  773,  434,  346,
 /*  1090 */   434,   75,  434,   73,  953,   31,  434,  953,   45,  434,
 /*  1100 */   259,  434,  690,  434,  757,  434,  887,  434,  953,   46,
 /*  1110 */   953,   32,  953,  115,  434,  266,  953,  116,  951,  953,
 /*  1120 */   117,  953,   52,  953,   33,  953,   99,  953,   49,  726,
 /*  1130 */   434,  909,  434,   19,  953,  100,  434,  344,  434,  113,
 /*  1140 */   434,  258,  692,  434,  259,  434,  670,  434,   20,  434,
 /*  1150 */   953,  101,  953,   97,  434,  259,  953,  114,  953,  112,
 /*  1160 */   953,  105,  113,  953,  104,  953,  102,  953,  103,  953,
 /*  1170 */    51,  434,  148,  434,  953,   53,  167,  434,  259,  113,
 /*  1180 */   300,  307,  912,  363,  311,  860,  248,  261,  209,  264,
 /*  1190 */   416,  953,   50,  953,   25,  420,  727,  953,   29,  430,
 /*  1200 */   321,  424,  757,  428,  322,  124, 1269,  214,  165,  710,
 /*  1210 */   859,  908,  806,  794,  309,  158,  193,  361,  254,  723,
 /*  1220 */   364,   67,  381,  269,  735,  199,   67,   70,  113,  700,
 /*  1230 */   699,  707,  708,  884,  113,  766,  113,  855,  193,  883,
 /*  1240 */   199,  869,  869,  675,  868,  868,  109,  368,  255,  260,
 /*  1250 */   263,  280,  859,  265,  806,  974,  267,  711,  695,  272,
 /*  1260 */   764,  282,  795,  284,  150,  744,  755,  415,  292,  293,
 /*  1270 */   802,  678,  672,  661,  660,  662,  927,    6,  306,  386,
 /*  1280 */   352,  786,  243,  250,  886,  362,  163,  286,  419,  298,
 /*  1290 */   930,  159,  968,  196,  126,  903,  901,  965,   55,   58,
 /*  1300 */   323,  275,  857,  136,  147,  694,  856,  121,   65,  354,
 /*  1310 */   355,  379,  175,   61,  151,  369,  180,  871,  375,  129,
 /*  1320 */   257,  756,  210,  181,  145,  131,  132,  377,  262,  663,
 /*  1330 */   133,  134,  139,  783,  791,  182,  392,  183,  312,  330,
 /*  1340 */   714,  888,  713,  851,  692,  195,  712,  406,  686,  705,
 /*  1350 */   313,  685,   64,  839,  274,   72,  684,  334,  942,   95,
 /*  1360 */   752,  279,  281,  704,  753,  751,  422,  283,  411,  750,
 /*  1370 */   426,   66,  204,  409,   21,  285,  928,  669,  437,  205,
 /*  1380 */   207,  208,  438,  658,  657,  652,  118,  108,  119,  226,
 /*  1390 */   650,  341,  157,  235,  169,  345,  106,  734,  790,  296,
 /*  1400 */   294,  295,  120,  297,  867,  865,  127,  128,  130,  724,
 /*  1410 */   229,  174,  249,  882,  137,  230,  138,  135,  885,  231,
 /*  1420 */    59,   60,  177,  881,    7,  178,   12,  179,  256,  874,
 /*  1430 */   140,  193,  962,  374,  141,  152,  666,  378,  276,  184,
 /*  1440 */   270,  122,  142,  382,  387,   62,   13,   14,  703,   63,
 /*  1450 */   125,  317,  318,  227,  809,  808,  837,  732,   15,  164,
 /*  1460 */   736,    4,  765,  211,  399,  213,  192,  143,  760,   70,
 /*  1470 */    67,   16,   17,  838,  836,  891,  841,  890,  198,  197,
 /*  1480 */   917,  154,  421,  923,  918,  155,  200,  977,  425,  840,
 /*  1490 */   156,  201,  807,  676,   80,  302,  299,  977,  202, 1261,
 /*  1500 */  1260,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    19,   95,   53,   97,   22,   24,   24,  101,   27,   28,
 /*    10 */    29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
 /*    20 */    39,   40,   41,  152,   43,   44,   45,   46,   47,   48,
 /*    30 */    49,   50,   51,   52,   53,   19,   55,   55,  132,  133,
 /*    40 */   134,    1,    2,   27,   28,   29,   30,   31,   32,   33,
 /*    50 */    34,   35,   36,   37,   38,   39,   40,   41,   92,   43,
 /*    60 */    44,   45,   46,   47,   48,   49,   50,   51,   52,   53,
 /*    70 */    47,   48,   49,   50,   51,   52,   53,   61,   97,   97,
 /*    80 */    19,   49,   50,   51,   52,   53,   70,   26,   27,   28,
 /*    90 */    29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
 /*   100 */    39,   40,   41,  152,   43,   44,   45,   46,   47,   48,
 /*   110 */    49,   50,   51,   52,   53,  144,  145,  146,  147,   19,
 /*   120 */   249,   22,  172,  172,  173,   52,   53,   27,   28,   29,
 /*   130 */    30,   31,   32,   33,   34,   35,   36,   37,   38,   39,
 /*   140 */    40,   41,   81,   43,   44,   45,   46,   47,   48,   49,
 /*   150 */    50,   51,   52,   53,   55,   56,   19,  152,  207,  208,
 /*   160 */   115,   24,  117,  118,   27,   28,   29,   30,   31,   32,
 /*   170 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   79,
 /*   180 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*   190 */    53,   19,    0,    1,    2,   23,   97,   98,  193,   27,
 /*   200 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*   210 */    38,   39,   40,   41,  152,   43,   44,   45,   46,   47,
 /*   220 */    48,   49,   50,   51,   52,   53,   19,   22,   23,  172,
 /*   230 */    23,   26,  119,  120,   27,   28,   29,   30,   31,   32,
 /*   240 */    33,   34,   35,   36,   37,   38,   39,   40,   41,  187,
 /*   250 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*   260 */    53,   19,  221,  222,  223,   23,  168,  169,  170,   27,
 /*   270 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*   280 */    38,   39,   40,   41,  152,   43,   44,   45,   46,   47,
 /*   290 */    48,   49,   50,   51,   52,   53,   19,  157,   22,   23,
 /*   300 */    23,   96,   26,  172,   27,   28,   29,   30,   31,   32,
 /*   310 */    33,   34,   35,   36,   37,   38,   39,   40,   41,  187,
 /*   320 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*   330 */    53,   19,  108,  109,  110,  221,  222,  223,  185,   27,
 /*   340 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*   350 */    38,   39,   40,   41,  240,   43,   44,   45,   46,   47,
 /*   360 */    48,   49,   50,   51,   52,   53,   19,  227,   22,   23,
 /*   370 */   230,   22,   96,  152,   27,   28,   29,   30,   31,   32,
 /*   380 */    33,   34,   35,   36,   37,   38,   39,   40,   41,   30,
 /*   390 */    43,   44,   45,   46,   47,   48,   49,   50,   51,   52,
 /*   400 */    53,   19,  190,  191,   55,   56,   24,  190,  191,   27,
 /*   410 */    28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
 /*   420 */    38,   39,   40,   41,  152,   43,   44,   45,   46,   47,
 /*   430 */    48,   49,   50,   51,   52,   53,  168,  169,  170,  179,
 /*   440 */   180,  171,   96,   19,  172,  173,   97,   98,  188,  179,
 /*   450 */   138,   27,   28,   29,   30,   31,   32,   33,   34,   35,
 /*   460 */    36,   37,   38,   39,   40,   41,  107,   43,   44,   45,
 /*   470 */    46,   47,   48,   49,   50,   51,   52,   53,   19,  207,
 /*   480 */   208,   30,   31,   32,   33,  138,   27,   28,   29,   30,
 /*   490 */    31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*   500 */    41,  181,   43,   44,   45,   46,   47,   48,   49,   50,
 /*   510 */    51,   52,   53,   19,  152,    7,    8,    9,   49,   22,
 /*   520 */    19,   24,   28,   29,   30,   31,   32,   33,   34,   35,
 /*   530 */    36,   37,   38,   39,   40,   41,  152,   43,   44,   45,
 /*   540 */    46,   47,   48,   49,   50,   51,   52,   53,   19,  108,
 /*   550 */   109,  110,  101,   55,   53,  193,  172,  173,   29,   30,
 /*   560 */    31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
 /*   570 */    41,  152,   43,   44,   45,   46,   47,   48,   49,   50,
 /*   580 */    51,   52,   53,   19,   20,  116,   22,   23,  169,  170,
 /*   590 */   121,  207,   85,   55,   56,   97,   19,   20,  195,   22,
 /*   600 */    99,  100,  101,  102,  103,  104,  105,   12,  152,  206,
 /*   610 */   210,   47,   48,  112,  152,  108,  109,  110,   54,   55,
 /*   620 */    56,  221,  222,  223,   47,   48,  119,  120,  172,  173,
 /*   630 */    66,   54,   55,   56,  101,   97,   98,   99,  148,  149,
 /*   640 */   102,  103,  104,   66,  154,  112,  156,   83,  229,   47,
 /*   650 */    48,  113,   57,  163,  194,  195,   92,  246,  247,   95,
 /*   660 */    83,   97,   98,  207,  208,  101,  206,   59,   73,   92,
 /*   670 */    75,   63,   95,  163,   97,   98,  194,  195,  101,  219,
 /*   680 */    85,  181,   19,  152,  175,   77,  196,  152,  206,   19,
 /*   690 */    20,  199,   22,   30,  163,   11,  132,  133,  134,  135,
 /*   700 */   136,  209,  152,  172,  173,  152,  196,  172,  173,  132,
 /*   710 */   133,  134,  135,  136,  164,   92,  213,   47,   48,   49,
 /*   720 */    92,  186,  169,  170,   54,   55,   56,  196,  100,  219,
 /*   730 */   221,  222,  223,  243,  132,  133,   66,  175,  207,  208,
 /*   740 */   152,  231,  119,  120,   19,   20,  236,   22,  152,   38,
 /*   750 */    39,   40,   41,   83,   43,   44,   45,   46,   47,   48,
 /*   760 */    49,   50,   51,   52,   53,   95,  152,   97,   98,   85,
 /*   770 */   107,  101,   47,   48,  163,  112,   92,  152,  152,   54,
 /*   780 */    55,   56,  229,  221,  222,  223,  172,  173,  163,   49,
 /*   790 */   152,   66,  108,  109,  110,   55,   56,  172,  173,  163,
 /*   800 */   186,   22,  132,  133,  134,  135,  136,  196,   83,   43,
 /*   810 */    44,   45,   46,   47,   48,   49,   50,   51,   52,   53,
 /*   820 */    95,  196,   97,   98,   85,   99,  101,  152,  102,  103,
 /*   830 */   104,   92,  196,  146,  147,  152,  181,   97,   98,  113,
 /*   840 */    19,   20,  152,   22,  218,   22,  116,  108,  109,  110,
 /*   850 */   152,  121,   55,   56,   12,  219,  218,  132,  133,  134,
 /*   860 */   135,  136,  172,  173,   55,   56,   55,   56,   47,   48,
 /*   870 */   172,  173,  236,  152,    5,   54,   55,   56,   55,   10,
 /*   880 */    11,   12,   13,   14,  186,  152,   17,   66,   47,   48,
 /*   890 */   152,  210,   16,   84,   97,   98,   55,   56,   21,   57,
 /*   900 */   217,  122,   22,  152,   83,  152,   97,   98,   97,   98,
 /*   910 */   172,  173,  152,  152,  224,   73,   95,   75,   97,   98,
 /*   920 */    97,  124,  101,  172,  173,  164,  193,   47,   48,   60,
 /*   930 */   163,   62,  172,  173,   24,   55,   56,  186,   97,   98,
 /*   940 */    71,  100,   55,   56,  183,  207,  185,  152,  107,   23,
 /*   950 */   109,   82,   26,  132,  133,  134,  135,  136,   89,  152,
 /*   960 */    26,   92,   93,  196,   88,   55,   90,  152,   91,  152,
 /*   970 */   217,  152,  152,  132,  133,   95,  152,   97,  211,  212,
 /*   980 */   103,  101,  152,  114,   97,   98,  152,  172,  173,   19,
 /*   990 */    38,   39,   40,   41,   42,   43,   44,   45,   46,   47,
 /*  1000 */    48,   49,   50,   51,   52,   53,   65,   97,  152,  152,
 /*  1010 */   141,  124,  132,  133,  134,  152,  140,  140,  152,   78,
 /*  1020 */   152,  233,  152,  193,   98,  152,   56,   86,  172,  173,
 /*  1030 */   166,  167,  237,  152,  217,  172,  173,  152,  172,  173,
 /*  1040 */   172,  173,  172,  173,  237,  172,  173,  152,  152,  152,
 /*  1050 */   124,  152,  111,  172,  173,  152,  237,  172,  173,  152,
 /*  1060 */    55,  152,   26,  152,  130,  152,  152,  172,  173,  172,
 /*  1070 */   173,  172,  173,  249,  152,  172,  173,  152,   61,  172,
 /*  1080 */   173,  172,  173,  172,  173,  172,  173,   70,  152,  193,
 /*  1090 */   152,  137,  152,  139,  172,  173,  152,  172,  173,  152,
 /*  1100 */   152,  152,   97,  152,   26,  152,  163,  152,  172,  173,
 /*  1110 */   172,  173,  172,  173,  152,   16,  172,  173,   26,  172,
 /*  1120 */   173,  172,  173,  172,  173,  172,  173,  172,  173,  163,
 /*  1130 */   152,  152,  152,   22,  172,  173,  152,  241,  152,  196,
 /*  1140 */   152,  193,  106,  152,  152,  152,  163,  152,   37,  152,
 /*  1150 */   172,  173,  172,  173,  152,  152,  172,  173,  172,  173,
 /*  1160 */   172,  173,  196,  172,  173,  172,  173,  172,  173,  172,
 /*  1170 */   173,  152,  152,  152,  172,  173,   98,  152,  152,  196,
 /*  1180 */   160,   22,   23,   19,  164,  193,  152,   88,  232,   90,
 /*  1190 */   191,  172,  173,  172,  173,  163,  193,  172,  173,  166,
 /*  1200 */   167,  163,  124,  163,  244,  245,   23,  211,  212,   26,
 /*  1210 */    55,   23,   55,   23,   26,  123,   26,  152,   23,  193,
 /*  1220 */    56,   26,   23,   23,   23,   26,   26,   26,  196,  100,
 /*  1230 */   101,    7,    8,  152,  196,   23,  196,   23,   26,  152,
 /*  1240 */    26,  132,  133,   23,  132,  133,   26,  152,  152,  152,
 /*  1250 */   152,  210,   97,  152,   97,   96,  152,  152,  152,  152,
 /*  1260 */   152,  210,  152,  210,  197,  152,  152,  152,  152,  152,
 /*  1270 */   152,  152,  152,  152,  152,  152,  152,  198,  150,  176,
 /*  1280 */   214,  201,  214,  238,  201,  238,  184,  214,  226,  200,
 /*  1290 */   155,  198,   67,  122,  242,  159,  159,   69,  239,  239,
 /*  1300 */   159,  175,  175,   22,  220,  180,  175,   27,  130,   18,
 /*  1310 */   159,   18,  158,  137,  220,  159,  158,  235,   74,  189,
 /*  1320 */   234,  159,  159,  158,   22,  192,  192,  177,  159,  159,
 /*  1330 */   192,  192,  189,  201,  189,  158,  107,  158,  177,   76,
 /*  1340 */   174,  201,  174,  201,  106,  159,  174,  125,  174,  182,
 /*  1350 */   177,  176,  107,  159,  174,  137,  174,   53,  174,  129,
 /*  1360 */   216,  215,  215,  182,  216,  216,  177,  215,  126,  216,
 /*  1370 */   177,  128,   25,  127,   26,  215,   13,  162,  161,  153,
 /*  1380 */   153,    6,  151,  151,  151,  151,  165,  178,  165,  178,
 /*  1390 */     4,    3,   22,  142,   15,   94,   16,  205,  120,  202,
 /*  1400 */   204,  203,  165,  201,   23,   23,  131,  111,  123,   20,
 /*  1410 */   225,  125,   16,    1,  131,  228,  111,  123,   56,  228,
 /*  1420 */    37,   37,   64,    1,    5,  122,   22,  107,  140,   80,
 /*  1430 */    80,   26,   87,   72,  107,   24,   20,   19,  112,  105,
 /*  1440 */    23,   68,   22,   79,   79,   22,   22,   22,   58,   22,
 /*  1450 */   245,  248,  248,   79,   23,   23,   23,  116,   22,  122,
 /*  1460 */    23,   22,   56,   23,   26,   23,   64,   22,  124,   26,
 /*  1470 */    26,   64,   64,   23,   23,   23,   11,   23,   22,   26,
 /*  1480 */    23,   22,   24,    1,   23,   22,   26,  250,   24,   23,
 /*  1490 */    22,  122,   23,   23,   22,   15,   23,  250,  122,  122,
 /*  1500 */   122,
};
#define YY_SHIFT_USE_DFLT (-95)
#define YY_SHIFT_COUNT (439)
#define YY_SHIFT_MIN   (-94)
#define YY_SHIFT_MAX   (1482)
static const short yy_shift_ofst[] = {
 /*     0 */    40,  564,  869,  577,  725,  725,  725,  739,  -19,   16,
 /*    10 */    16,  100,  725,  725,  725,  725,  725,  725,  725,  841,
 /*    20 */   841,  538,  507,  684,  623,   61,  137,  172,  207,  242,
 /*    30 */   277,  312,  347,  382,  424,  424,  424,  424,  424,  424,
 /*    40 */   424,  424,  424,  424,  424,  424,  424,  424,  424,  459,
 /*    50 */   424,  494,  529,  529,  670,  725,  725,  725,  725,  725,
 /*    60 */   725,  725,  725,  725,  725,  725,  725,  725,  725,  725,
 /*    70 */   725,  725,  725,  725,  725,  725,  725,  725,  725,  725,
 /*    80 */   725,  725,  725,  821,  725,  725,  725,  725,  725,  725,
 /*    90 */   725,  725,  725,  725,  725,  725,  725,  952,  711,  711,
 /*   100 */   711,  711,  711,  766,   23,   32,  811,  877,  663,  602,
 /*   110 */   602,  811,   73,  113,  -51,  -95,  -95,  -95,  501,  501,
 /*   120 */   501,  595,  595,  809,  205,  276,  811,  811,  811,  811,
 /*   130 */   811,  811,  811,  811,  811,  811,  811,  811,  811,  811,
 /*   140 */   811,  811,  811,  811,  811,  811,  192,  628,  498,  498,
 /*   150 */   113,  -34,  -34,  -34,  -34,  -34,  -34,  -95,  -95,  -95,
 /*   160 */   880,  -94,  -94,  726,  740,   99,  797,  887,  349,  811,
 /*   170 */   811,  811,  811,  811,  811,  811,  811,  811,  811,  811,
 /*   180 */   811,  811,  811,  811,  811,  811,  941,  941,  941,  811,
 /*   190 */   811,  926,  811,  811,  811,  -18,  811,  811,  842,  811,
 /*   200 */   811,  811,  811,  811,  811,  811,  811,  811,  811,  224,
 /*   210 */   608,  910,  910,  910, 1078,   45,  469,  508,  934,  970,
 /*   220 */   970, 1164,  934, 1164, 1036, 1183,  359, 1017,  970,  954,
 /*   230 */  1017, 1017, 1092,  730,  497, 1225, 1171, 1171, 1228, 1228,
 /*   240 */  1171, 1281, 1280, 1178, 1291, 1291, 1291, 1291, 1171, 1293,
 /*   250 */  1178, 1281, 1280, 1280, 1178, 1171, 1293, 1176, 1244, 1171,
 /*   260 */  1171, 1293, 1302, 1171, 1293, 1171, 1293, 1302, 1229, 1229,
 /*   270 */  1229, 1263, 1302, 1229, 1238, 1229, 1263, 1229, 1229, 1222,
 /*   280 */  1245, 1222, 1245, 1222, 1245, 1222, 1245, 1171, 1171, 1218,
 /*   290 */  1302, 1304, 1304, 1302, 1230, 1242, 1243, 1246, 1178, 1347,
 /*   300 */  1348, 1363, 1363, 1375, 1375, 1375, 1375,  -95,  -95,  -95,
 /*   310 */   -95,  -95,  -95,  -95,  -95,  451,  876,  346, 1159, 1099,
 /*   320 */   441,  823, 1188, 1111, 1190, 1195, 1199, 1200, 1005, 1129,
 /*   330 */  1224,  533, 1201, 1212, 1155, 1214, 1109, 1112, 1220, 1157,
 /*   340 */   779, 1386, 1388, 1370, 1251, 1379, 1301, 1380, 1381, 1382,
 /*   350 */  1278, 1275, 1296, 1285, 1389, 1286, 1396, 1412, 1294, 1283,
 /*   360 */  1383, 1384, 1305, 1362, 1358, 1303, 1422, 1419, 1404, 1320,
 /*   370 */  1288, 1349, 1405, 1350, 1345, 1361, 1327, 1411, 1416, 1418,
 /*   380 */  1326, 1334, 1420, 1364, 1423, 1424, 1417, 1425, 1365, 1390,
 /*   390 */  1427, 1374, 1373, 1431, 1432, 1433, 1341, 1436, 1437, 1439,
 /*   400 */  1438, 1337, 1440, 1442, 1406, 1402, 1445, 1344, 1443, 1407,
 /*   410 */  1444, 1408, 1443, 1450, 1451, 1452, 1453, 1454, 1456, 1465,
 /*   420 */  1457, 1459, 1458, 1460, 1461, 1463, 1464, 1460, 1466, 1468,
 /*   430 */  1469, 1470, 1472, 1369, 1376, 1377, 1378, 1473, 1480, 1482,
};
#define YY_REDUCE_USE_DFLT (-130)
#define YY_REDUCE_COUNT (314)
#define YY_REDUCE_MIN   (-129)
#define YY_REDUCE_MAX   (1237)
static const short yy_reduce_ofst[] = {
 /*     0 */   -29,  531,  490,  625,  -49,  272,  456,  510,  400,  509,
 /*    10 */   562,  114,  535,  614,  698,  384,  738,  751,  690,  419,
 /*    20 */   553,  761,  460,  636,  767,   41,   41,   41,   41,   41,
 /*    30 */    41,   41,   41,   41,   41,   41,   41,   41,   41,   41,
 /*    40 */    41,   41,   41,   41,   41,   41,   41,   41,   41,   41,
 /*    50 */    41,   41,   41,   41,  760,  815,  856,  863,  866,  868,
 /*    60 */   870,  873,  881,  885,  895,  897,  899,  903,  907,  909,
 /*    70 */   911,  913,  922,  925,  936,  938,  940,  944,  947,  949,
 /*    80 */   951,  953,  955,  962,  978,  980,  984,  986,  988,  991,
 /*    90 */   993,  995,  997, 1002, 1019, 1021, 1025,   41,   41,   41,
 /*   100 */    41,   41,   41,   41,   41,   41,  896,  140,  260,   98,
 /*   110 */   268, 1020,   41,  482,   41,   41,   41,   41,  270,  270,
 /*   120 */   270,  212,  217, -129,  411,  411,  550,    5,  626,  362,
 /*   130 */   733,  830,  992, 1003, 1026,  795,  683,  807,  638,  819,
 /*   140 */   753,  948,   62,  817,  824,  132,  687,  611,  864, 1033,
 /*   150 */   403,  943,  966,  983, 1032, 1038, 1040,  960,  996,  492,
 /*   160 */   -50,   57,  131,  153,  221,  462,  588,  596,  675,  721,
 /*   170 */   820,  834,  857,  914,  979, 1034, 1065, 1081, 1087, 1095,
 /*   180 */  1096, 1097, 1098, 1101, 1104, 1105,  320,  500,  655, 1106,
 /*   190 */  1107,  503, 1108, 1110, 1113,  681, 1114, 1115,  999, 1116,
 /*   200 */  1117, 1118,  221, 1119, 1120, 1121, 1122, 1123, 1124,  788,
 /*   210 */   956, 1041, 1051, 1053,  503, 1067, 1079, 1128, 1080, 1066,
 /*   220 */  1068, 1045, 1083, 1047, 1103, 1102, 1125, 1126, 1073, 1062,
 /*   230 */  1127, 1131, 1089, 1093, 1135, 1052, 1136, 1137, 1059, 1060,
 /*   240 */  1141, 1084, 1130, 1132, 1133, 1134, 1138, 1139, 1151, 1154,
 /*   250 */  1140, 1094, 1143, 1145, 1142, 1156, 1158, 1082, 1086, 1162,
 /*   260 */  1163, 1165, 1150, 1169, 1177, 1170, 1179, 1161, 1166, 1168,
 /*   270 */  1172, 1167, 1173, 1174, 1175, 1180, 1181, 1182, 1184, 1144,
 /*   280 */  1146, 1148, 1147, 1149, 1152, 1153, 1160, 1186, 1194, 1185,
 /*   290 */  1189, 1187, 1191, 1193, 1192, 1196, 1198, 1197, 1202, 1215,
 /*   300 */  1217, 1226, 1227, 1231, 1232, 1233, 1234, 1203, 1204, 1205,
 /*   310 */  1221, 1223, 1209, 1211, 1237,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */  1250, 1240, 1240, 1240, 1174, 1174, 1174, 1240, 1071, 1100,
 /*    10 */  1100, 1224, 1301, 1301, 1301, 1301, 1301, 1301, 1173, 1301,
 /*    20 */  1301, 1301, 1301, 1240, 1075, 1106, 1301, 1301, 1301, 1301,
 /*    30 */  1301, 1301, 1301, 1301, 1223, 1225, 1114, 1113, 1206, 1087,
 /*    40 */  1111, 1104, 1108, 1175, 1169, 1170, 1168, 1172, 1176, 1301,
 /*    50 */  1107, 1138, 1153, 1137, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    60 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    70 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    80 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*    90 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1147, 1152, 1159,
 /*   100 */  1151, 1148, 1140, 1139, 1141, 1142, 1301,  994, 1042, 1301,
 /*   110 */  1301, 1301, 1143, 1301, 1144, 1156, 1155, 1154, 1231, 1258,
 /*   120 */  1257, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   130 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   140 */  1301, 1301, 1301, 1301, 1301, 1301, 1250, 1240, 1000, 1000,
 /*   150 */  1301, 1240, 1240, 1240, 1240, 1240, 1240, 1236, 1075, 1066,
 /*   160 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   170 */  1228, 1226, 1301, 1187, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   180 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   190 */  1301, 1301, 1301, 1301, 1301, 1071, 1301, 1301, 1301, 1301,
 /*   200 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1252, 1301,
 /*   210 */  1201, 1071, 1071, 1071, 1073, 1055, 1065,  979, 1110, 1089,
 /*   220 */  1089, 1290, 1110, 1290, 1017, 1272, 1014, 1100, 1089, 1171,
 /*   230 */  1100, 1100, 1072, 1065, 1301, 1293, 1080, 1080, 1292, 1292,
 /*   240 */  1080, 1119, 1045, 1110, 1051, 1051, 1051, 1051, 1080,  991,
 /*   250 */  1110, 1119, 1045, 1045, 1110, 1080,  991, 1205, 1287, 1080,
 /*   260 */  1080,  991, 1180, 1080,  991, 1080,  991, 1180, 1043, 1043,
 /*   270 */  1043, 1032, 1180, 1043, 1017, 1043, 1032, 1043, 1043, 1093,
 /*   280 */  1088, 1093, 1088, 1093, 1088, 1093, 1088, 1080, 1080, 1301,
 /*   290 */  1180, 1184, 1184, 1180, 1105, 1094, 1103, 1101, 1110,  997,
 /*   300 */  1035, 1255, 1255, 1251, 1251, 1251, 1251, 1298, 1298, 1236,
 /*   310 */  1267, 1267, 1019, 1019, 1267, 1301, 1301, 1301, 1301, 1301,
 /*   320 */  1301, 1262, 1301, 1189, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   330 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   340 */  1125, 1301,  975, 1233, 1301, 1301, 1232, 1301, 1301, 1301,
 /*   350 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   360 */  1301, 1301, 1301, 1301, 1301, 1289, 1301, 1301, 1301, 1301,
 /*   370 */  1301, 1301, 1204, 1203, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   380 */  1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   390 */  1301, 1301, 1301, 1301, 1301, 1301, 1057, 1301, 1301, 1301,
 /*   400 */  1276, 1301, 1301, 1301, 1301, 1301, 1301, 1301, 1102, 1301,
 /*   410 */  1095, 1301, 1280, 1301, 1301, 1301, 1301, 1301, 1301, 1301,
 /*   420 */  1301, 1301, 1301, 1242, 1301, 1301, 1301, 1241, 1301, 1301,
 /*   430 */  1301, 1301, 1301, 1127, 1301, 1126, 1130, 1301,  985, 1301,
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
   55,  /*    EXPLAIN => ID */
   55,  /*      QUERY => ID */
   55,  /*       PLAN => ID */
   55,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
   55,  /*   DEFERRED => ID */
   55,  /*  IMMEDIATE => ID */
   55,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
   55,  /*        END => ID */
   55,  /*   ROLLBACK => ID */
   55,  /*  SAVEPOINT => ID */
   55,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   55,  /*         IF => ID */
    0,  /*        NOT => nothing */
    0,  /*     EXISTS => nothing */
   55,  /*       TEMP => ID */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
   55,  /*    WITHOUT => ID */
    0,  /*      COMMA => nothing */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*         IS => nothing */
   55,  /*      MATCH => ID */
   55,  /*    LIKE_KW => ID */
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
   55,  /*      ABORT => ID */
   55,  /*     ACTION => ID */
   55,  /*      AFTER => ID */
   55,  /*    ANALYZE => ID */
   55,  /*        ASC => ID */
   55,  /*     ATTACH => ID */
   55,  /*     BEFORE => ID */
   55,  /*         BY => ID */
   55,  /*    CASCADE => ID */
   55,  /*       CAST => ID */
   55,  /*   COLUMNKW => ID */
   55,  /*   CONFLICT => ID */
   55,  /*   DATABASE => ID */
   55,  /*       DESC => ID */
   55,  /*     DETACH => ID */
   55,  /*       EACH => ID */
   55,  /*       FAIL => ID */
   55,  /*        FOR => ID */
   55,  /*     IGNORE => ID */
   55,  /*  INITIALLY => ID */
   55,  /*    INSTEAD => ID */
   55,  /*         NO => ID */
   55,  /*        KEY => ID */
   55,  /*         OF => ID */
   55,  /*     OFFSET => ID */
   55,  /*     PRAGMA => ID */
   55,  /*      RAISE => ID */
   55,  /*  RECURSIVE => ID */
   55,  /*    REPLACE => ID */
   55,  /*   RESTRICT => ID */
   55,  /*        ROW => ID */
   55,  /*    TRIGGER => ID */
   55,  /*     VACUUM => ID */
   55,  /*       VIEW => ID */
   55,  /*    VIRTUAL => ID */
   55,  /*       WITH => ID */
   55,  /*    REINDEX => ID */
   55,  /*     RENAME => ID */
   55,  /*   CTIME_KW => ID */
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
  "$",             "SEMI",          "EXPLAIN",       "QUERY",       
  "PLAN",          "BEGIN",         "TRANSACTION",   "DEFERRED",    
  "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",        "END",         
  "ROLLBACK",      "SAVEPOINT",     "RELEASE",       "TO",          
  "TABLE",         "CREATE",        "IF",            "NOT",         
  "EXISTS",        "TEMP",          "LP",            "RP",          
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
  "EACH",          "FAIL",          "FOR",           "IGNORE",      
  "INITIALLY",     "INSTEAD",       "NO",            "KEY",         
  "OF",            "OFFSET",        "PRAGMA",        "RAISE",       
  "RECURSIVE",     "REPLACE",       "RESTRICT",      "ROW",         
  "TRIGGER",       "VACUUM",        "VIEW",          "VIRTUAL",     
  "WITH",          "REINDEX",       "RENAME",        "CTIME_KW",    
  "ANY",           "STRING",        "JOIN_KW",       "CONSTRAINT",  
  "DEFAULT",       "NULL",          "PRIMARY",       "UNIQUE",      
  "CHECK",         "REFERENCES",    "AUTOINCR",      "ON",          
  "INSERT",        "DELETE",        "UPDATE",        "SET",         
  "DEFERRABLE",    "FOREIGN",       "DROP",          "UNION",       
  "ALL",           "EXCEPT",        "INTERSECT",     "SELECT",      
  "VALUES",        "DISTINCT",      "DOT",           "FROM",        
  "JOIN",          "USING",         "ORDER",         "GROUP",       
  "HAVING",        "LIMIT",         "WHERE",         "INTO",        
  "INTEGER",       "FLOAT",         "BLOB",          "VARIABLE",    
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "ALTER",         "ADD",           "error",       
  "input",         "cmdlist",       "ecmd",          "explain",     
  "cmdx",          "cmd",           "transtype",     "trans_opt",   
  "nm",            "savepoint_opt",  "create_table",  "create_table_args",
  "createkw",      "temp",          "ifnotexists",   "dbnm",        
  "columnlist",    "conslist_opt",  "table_options",  "select",      
  "columnname",    "carglist",      "typetoken",     "typename",    
  "signed",        "plus_num",      "minus_num",     "ccons",       
  "term",          "expr",          "onconf",        "sortorder",   
  "autoinc",       "eidlist_opt",   "refargs",       "defer_subclause",
  "refarg",        "refact",        "init_deferred_pred_opt",  "conslist",    
  "tconscomma",    "tcons",         "sortlist",      "eidlist",     
  "defer_subclause_opt",  "orconf",        "resolvetype",   "raisetype",   
  "ifexists",      "fullname",      "selectnowith",  "oneselect",   
  "with",          "multiselect_op",  "distinct",      "selcollist",  
  "from",          "where_opt",     "groupby_opt",   "having_opt",  
  "orderby_opt",   "limit_opt",     "values",        "nexprlist",   
  "exprlist",      "sclp",          "as",            "seltablist",  
  "stl_prefix",    "joinop",        "indexed_opt",   "on_opt",      
  "using_opt",     "idlist",        "setlist",       "insert_cmd",  
  "idlist_opt",    "likeop",        "between_op",    "in_op",       
  "case_operand",  "case_exprlist",  "case_else",     "uniqueflag",  
  "collate",       "nmnum",         "trigger_decl",  "trigger_cmd_list",
  "trigger_time",  "trigger_event",  "foreach_clause",  "when_clause", 
  "trigger_cmd",   "trnm",          "tridxby",       "database_kw_opt",
  "key_opt",       "add_column_fullname",  "kwcolumn_opt",  "create_vtab", 
  "vtabarglist",   "vtabarg",       "vtabargtoken",  "lp",          
  "anylist",       "wqlist",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "explain ::= EXPLAIN",
 /*   1 */ "explain ::= EXPLAIN QUERY PLAN",
 /*   2 */ "cmdx ::= cmd",
 /*   3 */ "cmd ::= BEGIN transtype trans_opt",
 /*   4 */ "transtype ::=",
 /*   5 */ "transtype ::= DEFERRED",
 /*   6 */ "transtype ::= IMMEDIATE",
 /*   7 */ "transtype ::= EXCLUSIVE",
 /*   8 */ "cmd ::= COMMIT trans_opt",
 /*   9 */ "cmd ::= END trans_opt",
 /*  10 */ "cmd ::= ROLLBACK trans_opt",
 /*  11 */ "cmd ::= SAVEPOINT nm",
 /*  12 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  13 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  14 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  15 */ "createkw ::= CREATE",
 /*  16 */ "ifnotexists ::=",
 /*  17 */ "ifnotexists ::= IF NOT EXISTS",
 /*  18 */ "temp ::= TEMP",
 /*  19 */ "temp ::=",
 /*  20 */ "create_table_args ::= LP columnlist conslist_opt RP table_options",
 /*  21 */ "create_table_args ::= AS select",
 /*  22 */ "table_options ::=",
 /*  23 */ "table_options ::= WITHOUT nm",
 /*  24 */ "columnname ::= nm typetoken",
 /*  25 */ "typetoken ::=",
 /*  26 */ "typetoken ::= typename LP signed RP",
 /*  27 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  28 */ "typename ::= typename ID|STRING",
 /*  29 */ "ccons ::= CONSTRAINT nm",
 /*  30 */ "ccons ::= DEFAULT term",
 /*  31 */ "ccons ::= DEFAULT LP expr RP",
 /*  32 */ "ccons ::= DEFAULT PLUS term",
 /*  33 */ "ccons ::= DEFAULT MINUS term",
 /*  34 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  35 */ "ccons ::= NOT NULL onconf",
 /*  36 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  37 */ "ccons ::= UNIQUE onconf",
 /*  38 */ "ccons ::= CHECK LP expr RP",
 /*  39 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  40 */ "ccons ::= defer_subclause",
 /*  41 */ "ccons ::= COLLATE ID|STRING",
 /*  42 */ "autoinc ::=",
 /*  43 */ "autoinc ::= AUTOINCR",
 /*  44 */ "refargs ::=",
 /*  45 */ "refargs ::= refargs refarg",
 /*  46 */ "refarg ::= MATCH nm",
 /*  47 */ "refarg ::= ON INSERT refact",
 /*  48 */ "refarg ::= ON DELETE refact",
 /*  49 */ "refarg ::= ON UPDATE refact",
 /*  50 */ "refact ::= SET NULL",
 /*  51 */ "refact ::= SET DEFAULT",
 /*  52 */ "refact ::= CASCADE",
 /*  53 */ "refact ::= RESTRICT",
 /*  54 */ "refact ::= NO ACTION",
 /*  55 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  56 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  57 */ "init_deferred_pred_opt ::=",
 /*  58 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  59 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  60 */ "conslist_opt ::=",
 /*  61 */ "tconscomma ::= COMMA",
 /*  62 */ "tcons ::= CONSTRAINT nm",
 /*  63 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  64 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  65 */ "tcons ::= CHECK LP expr RP onconf",
 /*  66 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  67 */ "defer_subclause_opt ::=",
 /*  68 */ "onconf ::=",
 /*  69 */ "onconf ::= ON CONFLICT resolvetype",
 /*  70 */ "orconf ::=",
 /*  71 */ "orconf ::= OR resolvetype",
 /*  72 */ "resolvetype ::= IGNORE",
 /*  73 */ "resolvetype ::= REPLACE",
 /*  74 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  75 */ "ifexists ::= IF EXISTS",
 /*  76 */ "ifexists ::=",
 /*  77 */ "cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select",
 /*  78 */ "cmd ::= DROP VIEW ifexists fullname",
 /*  79 */ "cmd ::= select",
 /*  80 */ "select ::= with selectnowith",
 /*  81 */ "selectnowith ::= selectnowith multiselect_op oneselect",
 /*  82 */ "multiselect_op ::= UNION",
 /*  83 */ "multiselect_op ::= UNION ALL",
 /*  84 */ "multiselect_op ::= EXCEPT|INTERSECT",
 /*  85 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  86 */ "values ::= VALUES LP nexprlist RP",
 /*  87 */ "values ::= values COMMA LP exprlist RP",
 /*  88 */ "distinct ::= DISTINCT",
 /*  89 */ "distinct ::= ALL",
 /*  90 */ "distinct ::=",
 /*  91 */ "sclp ::=",
 /*  92 */ "selcollist ::= sclp expr as",
 /*  93 */ "selcollist ::= sclp STAR",
 /*  94 */ "selcollist ::= sclp nm DOT STAR",
 /*  95 */ "as ::= AS nm",
 /*  96 */ "as ::=",
 /*  97 */ "from ::=",
 /*  98 */ "from ::= FROM seltablist",
 /*  99 */ "stl_prefix ::= seltablist joinop",
 /* 100 */ "stl_prefix ::=",
 /* 101 */ "seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt",
 /* 102 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt",
 /* 103 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /* 104 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /* 105 */ "dbnm ::=",
 /* 106 */ "dbnm ::= DOT nm",
 /* 107 */ "fullname ::= nm dbnm",
 /* 108 */ "joinop ::= COMMA|JOIN",
 /* 109 */ "joinop ::= JOIN_KW JOIN",
 /* 110 */ "joinop ::= JOIN_KW nm JOIN",
 /* 111 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 112 */ "on_opt ::= ON expr",
 /* 113 */ "on_opt ::=",
 /* 114 */ "indexed_opt ::=",
 /* 115 */ "indexed_opt ::= INDEXED BY nm",
 /* 116 */ "indexed_opt ::= NOT INDEXED",
 /* 117 */ "using_opt ::= USING LP idlist RP",
 /* 118 */ "using_opt ::=",
 /* 119 */ "orderby_opt ::=",
 /* 120 */ "orderby_opt ::= ORDER BY sortlist",
 /* 121 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 122 */ "sortlist ::= expr sortorder",
 /* 123 */ "sortorder ::= ASC",
 /* 124 */ "sortorder ::= DESC",
 /* 125 */ "sortorder ::=",
 /* 126 */ "groupby_opt ::=",
 /* 127 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 128 */ "having_opt ::=",
 /* 129 */ "having_opt ::= HAVING expr",
 /* 130 */ "limit_opt ::=",
 /* 131 */ "limit_opt ::= LIMIT expr",
 /* 132 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 133 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 134 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 135 */ "where_opt ::=",
 /* 136 */ "where_opt ::= WHERE expr",
 /* 137 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 138 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 139 */ "setlist ::= nm EQ expr",
 /* 140 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 141 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 142 */ "insert_cmd ::= INSERT orconf",
 /* 143 */ "insert_cmd ::= REPLACE",
 /* 144 */ "idlist_opt ::=",
 /* 145 */ "idlist_opt ::= LP idlist RP",
 /* 146 */ "idlist ::= idlist COMMA nm",
 /* 147 */ "idlist ::= nm",
 /* 148 */ "expr ::= LP expr RP",
 /* 149 */ "term ::= NULL",
 /* 150 */ "expr ::= ID|INDEXED",
 /* 151 */ "expr ::= JOIN_KW",
 /* 152 */ "expr ::= nm DOT nm",
 /* 153 */ "expr ::= nm DOT nm DOT nm",
 /* 154 */ "term ::= INTEGER|FLOAT|BLOB",
 /* 155 */ "term ::= STRING",
 /* 156 */ "expr ::= VARIABLE",
 /* 157 */ "expr ::= expr COLLATE ID|STRING",
 /* 158 */ "expr ::= CAST LP expr AS typetoken RP",
 /* 159 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 160 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 161 */ "term ::= CTIME_KW",
 /* 162 */ "expr ::= expr AND expr",
 /* 163 */ "expr ::= expr OR expr",
 /* 164 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 165 */ "expr ::= expr EQ|NE expr",
 /* 166 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 167 */ "expr ::= expr PLUS|MINUS expr",
 /* 168 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 169 */ "expr ::= expr CONCAT expr",
 /* 170 */ "likeop ::= LIKE_KW|MATCH",
 /* 171 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 172 */ "expr ::= expr likeop expr",
 /* 173 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 174 */ "expr ::= expr ISNULL|NOTNULL",
 /* 175 */ "expr ::= expr NOT NULL",
 /* 176 */ "expr ::= expr IS expr",
 /* 177 */ "expr ::= expr IS NOT expr",
 /* 178 */ "expr ::= NOT expr",
 /* 179 */ "expr ::= BITNOT expr",
 /* 180 */ "expr ::= MINUS expr",
 /* 181 */ "expr ::= PLUS expr",
 /* 182 */ "between_op ::= BETWEEN",
 /* 183 */ "between_op ::= NOT BETWEEN",
 /* 184 */ "expr ::= expr between_op expr AND expr",
 /* 185 */ "in_op ::= IN",
 /* 186 */ "in_op ::= NOT IN",
 /* 187 */ "expr ::= expr in_op LP exprlist RP",
 /* 188 */ "expr ::= LP select RP",
 /* 189 */ "expr ::= expr in_op LP select RP",
 /* 190 */ "expr ::= expr in_op nm dbnm",
 /* 191 */ "expr ::= EXISTS LP select RP",
 /* 192 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 193 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 194 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 195 */ "case_else ::= ELSE expr",
 /* 196 */ "case_else ::=",
 /* 197 */ "case_operand ::= expr",
 /* 198 */ "case_operand ::=",
 /* 199 */ "exprlist ::=",
 /* 200 */ "nexprlist ::= nexprlist COMMA expr",
 /* 201 */ "nexprlist ::= expr",
 /* 202 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 203 */ "uniqueflag ::= UNIQUE",
 /* 204 */ "uniqueflag ::=",
 /* 205 */ "eidlist_opt ::=",
 /* 206 */ "eidlist_opt ::= LP eidlist RP",
 /* 207 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 208 */ "eidlist ::= nm collate sortorder",
 /* 209 */ "collate ::=",
 /* 210 */ "collate ::= COLLATE ID|STRING",
 /* 211 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 212 */ "cmd ::= VACUUM",
 /* 213 */ "cmd ::= VACUUM nm",
 /* 214 */ "cmd ::= PRAGMA nm dbnm",
 /* 215 */ "cmd ::= PRAGMA nm dbnm EQ nmnum",
 /* 216 */ "cmd ::= PRAGMA nm dbnm LP nmnum RP",
 /* 217 */ "cmd ::= PRAGMA nm dbnm EQ minus_num",
 /* 218 */ "cmd ::= PRAGMA nm dbnm LP minus_num RP",
 /* 219 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 220 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 221 */ "cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END",
 /* 222 */ "trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause",
 /* 223 */ "trigger_time ::= BEFORE",
 /* 224 */ "trigger_time ::= AFTER",
 /* 225 */ "trigger_time ::= INSTEAD OF",
 /* 226 */ "trigger_time ::=",
 /* 227 */ "trigger_event ::= DELETE|INSERT",
 /* 228 */ "trigger_event ::= UPDATE",
 /* 229 */ "trigger_event ::= UPDATE OF idlist",
 /* 230 */ "when_clause ::=",
 /* 231 */ "when_clause ::= WHEN expr",
 /* 232 */ "trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI",
 /* 233 */ "trigger_cmd_list ::= trigger_cmd SEMI",
 /* 234 */ "trnm ::= nm DOT nm",
 /* 235 */ "tridxby ::= INDEXED BY nm",
 /* 236 */ "tridxby ::= NOT INDEXED",
 /* 237 */ "trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt",
 /* 238 */ "trigger_cmd ::= insert_cmd INTO trnm idlist_opt select",
 /* 239 */ "trigger_cmd ::= DELETE FROM trnm tridxby where_opt",
 /* 240 */ "trigger_cmd ::= select",
 /* 241 */ "expr ::= RAISE LP IGNORE RP",
 /* 242 */ "expr ::= RAISE LP raisetype COMMA nm RP",
 /* 243 */ "raisetype ::= ROLLBACK",
 /* 244 */ "raisetype ::= ABORT",
 /* 245 */ "raisetype ::= FAIL",
 /* 246 */ "cmd ::= DROP TRIGGER ifexists fullname",
 /* 247 */ "cmd ::= ATTACH database_kw_opt expr AS expr key_opt",
 /* 248 */ "cmd ::= DETACH database_kw_opt expr",
 /* 249 */ "key_opt ::=",
 /* 250 */ "key_opt ::= KEY expr",
 /* 251 */ "cmd ::= REINDEX",
 /* 252 */ "cmd ::= REINDEX nm dbnm",
 /* 253 */ "cmd ::= ANALYZE",
 /* 254 */ "cmd ::= ANALYZE nm dbnm",
 /* 255 */ "cmd ::= ALTER TABLE fullname RENAME TO nm",
 /* 256 */ "cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist",
 /* 257 */ "add_column_fullname ::= fullname",
 /* 258 */ "cmd ::= create_vtab",
 /* 259 */ "cmd ::= create_vtab LP vtabarglist RP",
 /* 260 */ "create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm",
 /* 261 */ "vtabarg ::=",
 /* 262 */ "vtabargtoken ::= ANY",
 /* 263 */ "vtabargtoken ::= lp anylist RP",
 /* 264 */ "lp ::= LP",
 /* 265 */ "with ::=",
 /* 266 */ "with ::= WITH wqlist",
 /* 267 */ "with ::= WITH RECURSIVE wqlist",
 /* 268 */ "wqlist ::= nm eidlist_opt AS LP select RP",
 /* 269 */ "wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP",
 /* 270 */ "input ::= cmdlist",
 /* 271 */ "cmdlist ::= cmdlist ecmd",
 /* 272 */ "cmdlist ::= ecmd",
 /* 273 */ "ecmd ::= SEMI",
 /* 274 */ "ecmd ::= explain cmdx SEMI",
 /* 275 */ "explain ::=",
 /* 276 */ "trans_opt ::=",
 /* 277 */ "trans_opt ::= TRANSACTION",
 /* 278 */ "trans_opt ::= TRANSACTION nm",
 /* 279 */ "savepoint_opt ::= SAVEPOINT",
 /* 280 */ "savepoint_opt ::=",
 /* 281 */ "cmd ::= create_table create_table_args",
 /* 282 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 283 */ "columnlist ::= columnname carglist",
 /* 284 */ "nm ::= ID|INDEXED",
 /* 285 */ "nm ::= STRING",
 /* 286 */ "nm ::= JOIN_KW",
 /* 287 */ "typetoken ::= typename",
 /* 288 */ "typename ::= ID|STRING",
 /* 289 */ "signed ::= plus_num",
 /* 290 */ "signed ::= minus_num",
 /* 291 */ "carglist ::= carglist ccons",
 /* 292 */ "carglist ::=",
 /* 293 */ "ccons ::= NULL onconf",
 /* 294 */ "conslist_opt ::= COMMA conslist",
 /* 295 */ "conslist ::= conslist tconscomma tcons",
 /* 296 */ "conslist ::= tcons",
 /* 297 */ "tconscomma ::=",
 /* 298 */ "defer_subclause_opt ::= defer_subclause",
 /* 299 */ "resolvetype ::= raisetype",
 /* 300 */ "selectnowith ::= oneselect",
 /* 301 */ "oneselect ::= values",
 /* 302 */ "sclp ::= selcollist COMMA",
 /* 303 */ "as ::= ID|STRING",
 /* 304 */ "expr ::= term",
 /* 305 */ "exprlist ::= nexprlist",
 /* 306 */ "nmnum ::= plus_num",
 /* 307 */ "nmnum ::= nm",
 /* 308 */ "nmnum ::= ON",
 /* 309 */ "nmnum ::= DELETE",
 /* 310 */ "nmnum ::= DEFAULT",
 /* 311 */ "plus_num ::= INTEGER|FLOAT",
 /* 312 */ "foreach_clause ::=",
 /* 313 */ "foreach_clause ::= FOR EACH ROW",
 /* 314 */ "trnm ::= nm",
 /* 315 */ "tridxby ::=",
 /* 316 */ "database_kw_opt ::= DATABASE",
 /* 317 */ "database_kw_opt ::=",
 /* 318 */ "kwcolumn_opt ::=",
 /* 319 */ "kwcolumn_opt ::= COLUMNKW",
 /* 320 */ "vtabarglist ::= vtabarg",
 /* 321 */ "vtabarglist ::= vtabarglist COMMA vtabarg",
 /* 322 */ "vtabarg ::= vtabarg vtabargtoken",
 /* 323 */ "anylist ::=",
 /* 324 */ "anylist ::= anylist LP anylist RP",
 /* 325 */ "anylist ::= anylist ANY",
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
    case 163: /* select */
    case 194: /* selectnowith */
    case 195: /* oneselect */
    case 206: /* values */
{
#line 408 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy159));
#line 1526 "parse.c"
}
      break;
    case 172: /* term */
    case 173: /* expr */
{
#line 829 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy342).pExpr);
#line 1534 "parse.c"
}
      break;
    case 177: /* eidlist_opt */
    case 186: /* sortlist */
    case 187: /* eidlist */
    case 199: /* selcollist */
    case 202: /* groupby_opt */
    case 204: /* orderby_opt */
    case 207: /* nexprlist */
    case 208: /* exprlist */
    case 209: /* sclp */
    case 218: /* setlist */
    case 225: /* case_exprlist */
{
#line 1227 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy442));
#line 1551 "parse.c"
}
      break;
    case 193: /* fullname */
    case 200: /* from */
    case 211: /* seltablist */
    case 212: /* stl_prefix */
{
#line 640 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy347));
#line 1561 "parse.c"
}
      break;
    case 196: /* with */
    case 249: /* wqlist */
{
#line 1504 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy331));
#line 1569 "parse.c"
}
      break;
    case 201: /* where_opt */
    case 203: /* having_opt */
    case 215: /* on_opt */
    case 224: /* case_operand */
    case 226: /* case_else */
    case 235: /* when_clause */
    case 240: /* key_opt */
{
#line 756 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy122));
#line 1582 "parse.c"
}
      break;
    case 216: /* using_opt */
    case 217: /* idlist */
    case 220: /* idlist_opt */
{
#line 674 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy180));
#line 1591 "parse.c"
}
      break;
    case 231: /* trigger_cmd_list */
    case 236: /* trigger_cmd */
{
#line 1341 "parse.y"
sqlite3DeleteTriggerStep(pParse->db, (yypminor->yy327));
#line 1599 "parse.c"
}
      break;
    case 233: /* trigger_event */
{
#line 1327 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy410).b);
#line 1606 "parse.c"
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
#line 1782 "parse.c"
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
  { 147, 1 },
  { 147, 3 },
  { 148, 1 },
  { 149, 3 },
  { 150, 0 },
  { 150, 1 },
  { 150, 1 },
  { 150, 1 },
  { 149, 2 },
  { 149, 2 },
  { 149, 2 },
  { 149, 2 },
  { 149, 3 },
  { 149, 5 },
  { 154, 6 },
  { 156, 1 },
  { 158, 0 },
  { 158, 3 },
  { 157, 1 },
  { 157, 0 },
  { 155, 5 },
  { 155, 2 },
  { 162, 0 },
  { 162, 2 },
  { 164, 2 },
  { 166, 0 },
  { 166, 4 },
  { 166, 6 },
  { 167, 2 },
  { 171, 2 },
  { 171, 2 },
  { 171, 4 },
  { 171, 3 },
  { 171, 3 },
  { 171, 2 },
  { 171, 3 },
  { 171, 5 },
  { 171, 2 },
  { 171, 4 },
  { 171, 4 },
  { 171, 1 },
  { 171, 2 },
  { 176, 0 },
  { 176, 1 },
  { 178, 0 },
  { 178, 2 },
  { 180, 2 },
  { 180, 3 },
  { 180, 3 },
  { 180, 3 },
  { 181, 2 },
  { 181, 2 },
  { 181, 1 },
  { 181, 1 },
  { 181, 2 },
  { 179, 3 },
  { 179, 2 },
  { 182, 0 },
  { 182, 2 },
  { 182, 2 },
  { 161, 0 },
  { 184, 1 },
  { 185, 2 },
  { 185, 7 },
  { 185, 5 },
  { 185, 5 },
  { 185, 10 },
  { 188, 0 },
  { 174, 0 },
  { 174, 3 },
  { 189, 0 },
  { 189, 2 },
  { 190, 1 },
  { 190, 1 },
  { 149, 4 },
  { 192, 2 },
  { 192, 0 },
  { 149, 9 },
  { 149, 4 },
  { 149, 1 },
  { 163, 2 },
  { 194, 3 },
  { 197, 1 },
  { 197, 2 },
  { 197, 1 },
  { 195, 9 },
  { 206, 4 },
  { 206, 5 },
  { 198, 1 },
  { 198, 1 },
  { 198, 0 },
  { 209, 0 },
  { 199, 3 },
  { 199, 2 },
  { 199, 4 },
  { 210, 2 },
  { 210, 0 },
  { 200, 0 },
  { 200, 2 },
  { 212, 2 },
  { 212, 0 },
  { 211, 7 },
  { 211, 9 },
  { 211, 7 },
  { 211, 7 },
  { 159, 0 },
  { 159, 2 },
  { 193, 2 },
  { 213, 1 },
  { 213, 2 },
  { 213, 3 },
  { 213, 4 },
  { 215, 2 },
  { 215, 0 },
  { 214, 0 },
  { 214, 3 },
  { 214, 2 },
  { 216, 4 },
  { 216, 0 },
  { 204, 0 },
  { 204, 3 },
  { 186, 4 },
  { 186, 2 },
  { 175, 1 },
  { 175, 1 },
  { 175, 0 },
  { 202, 0 },
  { 202, 3 },
  { 203, 0 },
  { 203, 2 },
  { 205, 0 },
  { 205, 2 },
  { 205, 4 },
  { 205, 4 },
  { 149, 6 },
  { 201, 0 },
  { 201, 2 },
  { 149, 8 },
  { 218, 5 },
  { 218, 3 },
  { 149, 6 },
  { 149, 7 },
  { 219, 2 },
  { 219, 1 },
  { 220, 0 },
  { 220, 3 },
  { 217, 3 },
  { 217, 1 },
  { 173, 3 },
  { 172, 1 },
  { 173, 1 },
  { 173, 1 },
  { 173, 3 },
  { 173, 5 },
  { 172, 1 },
  { 172, 1 },
  { 173, 1 },
  { 173, 3 },
  { 173, 6 },
  { 173, 5 },
  { 173, 4 },
  { 172, 1 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 173, 3 },
  { 221, 1 },
  { 221, 2 },
  { 173, 3 },
  { 173, 5 },
  { 173, 2 },
  { 173, 3 },
  { 173, 3 },
  { 173, 4 },
  { 173, 2 },
  { 173, 2 },
  { 173, 2 },
  { 173, 2 },
  { 222, 1 },
  { 222, 2 },
  { 173, 5 },
  { 223, 1 },
  { 223, 2 },
  { 173, 5 },
  { 173, 3 },
  { 173, 5 },
  { 173, 4 },
  { 173, 4 },
  { 173, 5 },
  { 225, 5 },
  { 225, 4 },
  { 226, 2 },
  { 226, 0 },
  { 224, 1 },
  { 224, 0 },
  { 208, 0 },
  { 207, 3 },
  { 207, 1 },
  { 149, 12 },
  { 227, 1 },
  { 227, 0 },
  { 177, 0 },
  { 177, 3 },
  { 187, 5 },
  { 187, 3 },
  { 228, 0 },
  { 228, 2 },
  { 149, 4 },
  { 149, 1 },
  { 149, 2 },
  { 149, 3 },
  { 149, 5 },
  { 149, 6 },
  { 149, 5 },
  { 149, 6 },
  { 169, 2 },
  { 170, 2 },
  { 149, 5 },
  { 230, 11 },
  { 232, 1 },
  { 232, 1 },
  { 232, 2 },
  { 232, 0 },
  { 233, 1 },
  { 233, 1 },
  { 233, 3 },
  { 235, 0 },
  { 235, 2 },
  { 231, 3 },
  { 231, 2 },
  { 237, 3 },
  { 238, 3 },
  { 238, 2 },
  { 236, 7 },
  { 236, 5 },
  { 236, 5 },
  { 236, 1 },
  { 173, 4 },
  { 173, 6 },
  { 191, 1 },
  { 191, 1 },
  { 191, 1 },
  { 149, 4 },
  { 149, 6 },
  { 149, 3 },
  { 240, 0 },
  { 240, 2 },
  { 149, 1 },
  { 149, 3 },
  { 149, 1 },
  { 149, 3 },
  { 149, 6 },
  { 149, 7 },
  { 241, 1 },
  { 149, 1 },
  { 149, 4 },
  { 243, 8 },
  { 245, 0 },
  { 246, 1 },
  { 246, 3 },
  { 247, 1 },
  { 196, 0 },
  { 196, 2 },
  { 196, 3 },
  { 249, 6 },
  { 249, 8 },
  { 144, 1 },
  { 145, 2 },
  { 145, 1 },
  { 146, 1 },
  { 146, 3 },
  { 147, 0 },
  { 151, 0 },
  { 151, 1 },
  { 151, 2 },
  { 153, 1 },
  { 153, 0 },
  { 149, 2 },
  { 160, 4 },
  { 160, 2 },
  { 152, 1 },
  { 152, 1 },
  { 152, 1 },
  { 166, 1 },
  { 167, 1 },
  { 168, 1 },
  { 168, 1 },
  { 165, 2 },
  { 165, 0 },
  { 171, 2 },
  { 161, 2 },
  { 183, 3 },
  { 183, 1 },
  { 184, 0 },
  { 188, 1 },
  { 190, 1 },
  { 194, 1 },
  { 195, 1 },
  { 209, 2 },
  { 210, 1 },
  { 173, 1 },
  { 208, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 229, 1 },
  { 169, 1 },
  { 234, 0 },
  { 234, 3 },
  { 237, 1 },
  { 238, 0 },
  { 239, 1 },
  { 239, 0 },
  { 242, 0 },
  { 242, 1 },
  { 244, 1 },
  { 244, 3 },
  { 245, 2 },
  { 248, 0 },
  { 248, 4 },
  { 248, 2 },
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
      case 0: /* explain ::= EXPLAIN */
#line 127 "parse.y"
{ pParse->explain = 1; }
#line 2242 "parse.c"
        break;
      case 1: /* explain ::= EXPLAIN QUERY PLAN */
#line 128 "parse.y"
{ pParse->explain = 2; }
#line 2247 "parse.c"
        break;
      case 2: /* cmdx ::= cmd */
#line 130 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 2252 "parse.c"
        break;
      case 3: /* cmd ::= BEGIN transtype trans_opt */
#line 135 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy392);}
#line 2257 "parse.c"
        break;
      case 4: /* transtype ::= */
#line 140 "parse.y"
{yymsp[1].minor.yy392 = TK_DEFERRED;}
#line 2262 "parse.c"
        break;
      case 5: /* transtype ::= DEFERRED */
      case 6: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==6);
      case 7: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==7);
#line 141 "parse.y"
{yymsp[0].minor.yy392 = yymsp[0].major; /*A-overwrites-X*/}
#line 2269 "parse.c"
        break;
      case 8: /* cmd ::= COMMIT trans_opt */
      case 9: /* cmd ::= END trans_opt */ yytestcase(yyruleno==9);
#line 144 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 2275 "parse.c"
        break;
      case 10: /* cmd ::= ROLLBACK trans_opt */
#line 146 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 2280 "parse.c"
        break;
      case 11: /* cmd ::= SAVEPOINT nm */
#line 150 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2287 "parse.c"
        break;
      case 12: /* cmd ::= RELEASE savepoint_opt nm */
#line 153 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2294 "parse.c"
        break;
      case 13: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 156 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2301 "parse.c"
        break;
      case 14: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
#line 163 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,yymsp[-4].minor.yy392,0,0,yymsp[-2].minor.yy392);
}
#line 2308 "parse.c"
        break;
      case 15: /* createkw ::= CREATE */
#line 166 "parse.y"
{disableLookaside(pParse);}
#line 2313 "parse.c"
        break;
      case 16: /* ifnotexists ::= */
      case 19: /* temp ::= */ yytestcase(yyruleno==19);
      case 22: /* table_options ::= */ yytestcase(yyruleno==22);
      case 42: /* autoinc ::= */ yytestcase(yyruleno==42);
      case 57: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==57);
      case 67: /* defer_subclause_opt ::= */ yytestcase(yyruleno==67);
      case 76: /* ifexists ::= */ yytestcase(yyruleno==76);
      case 90: /* distinct ::= */ yytestcase(yyruleno==90);
      case 209: /* collate ::= */ yytestcase(yyruleno==209);
#line 169 "parse.y"
{yymsp[1].minor.yy392 = 0;}
#line 2326 "parse.c"
        break;
      case 17: /* ifnotexists ::= IF NOT EXISTS */
#line 170 "parse.y"
{yymsp[-2].minor.yy392 = 1;}
#line 2331 "parse.c"
        break;
      case 18: /* temp ::= TEMP */
      case 43: /* autoinc ::= AUTOINCR */ yytestcase(yyruleno==43);
#line 173 "parse.y"
{yymsp[0].minor.yy392 = 1;}
#line 2337 "parse.c"
        break;
      case 20: /* create_table_args ::= LP columnlist conslist_opt RP table_options */
#line 176 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,yymsp[0].minor.yy392,0);
}
#line 2344 "parse.c"
        break;
      case 21: /* create_table_args ::= AS select */
#line 179 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy159);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
}
#line 2352 "parse.c"
        break;
      case 23: /* table_options ::= WITHOUT nm */
#line 185 "parse.y"
{
  if( yymsp[0].minor.yy0.n==5 && sqlite3_strnicmp(yymsp[0].minor.yy0.z,"rowid",5)==0 ){
    yymsp[-1].minor.yy392 = TF_WithoutRowid | TF_NoVisibleRowid;
  }else{
    yymsp[-1].minor.yy392 = 0;
    sqlite3ErrorMsg(pParse, "unknown table option: %.*s", yymsp[0].minor.yy0.n, yymsp[0].minor.yy0.z);
  }
}
#line 2364 "parse.c"
        break;
      case 24: /* columnname ::= nm typetoken */
#line 195 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2369 "parse.c"
        break;
      case 25: /* typetoken ::= */
      case 60: /* conslist_opt ::= */ yytestcase(yyruleno==60);
      case 96: /* as ::= */ yytestcase(yyruleno==96);
#line 260 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2376 "parse.c"
        break;
      case 26: /* typetoken ::= typename LP signed RP */
#line 262 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2383 "parse.c"
        break;
      case 27: /* typetoken ::= typename LP signed COMMA signed RP */
#line 265 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2390 "parse.c"
        break;
      case 28: /* typename ::= typename ID|STRING */
#line 270 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2395 "parse.c"
        break;
      case 29: /* ccons ::= CONSTRAINT nm */
      case 62: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==62);
#line 279 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2401 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT term */
      case 32: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==32);
#line 280 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy342);}
#line 2407 "parse.c"
        break;
      case 31: /* ccons ::= DEFAULT LP expr RP */
#line 281 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy342);}
#line 2412 "parse.c"
        break;
      case 33: /* ccons ::= DEFAULT MINUS term */
#line 283 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy342.pExpr, 0, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy342.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2423 "parse.c"
        break;
      case 34: /* ccons ::= DEFAULT ID|INDEXED */
#line 290 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2432 "parse.c"
        break;
      case 35: /* ccons ::= NOT NULL onconf */
#line 300 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy392);}
#line 2437 "parse.c"
        break;
      case 36: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 302 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy392,yymsp[0].minor.yy392,yymsp[-2].minor.yy392);}
#line 2442 "parse.c"
        break;
      case 37: /* ccons ::= UNIQUE onconf */
#line 303 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,0,yymsp[0].minor.yy392,0,0,0,0);}
#line 2447 "parse.c"
        break;
      case 38: /* ccons ::= CHECK LP expr RP */
#line 304 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy342.pExpr);}
#line 2452 "parse.c"
        break;
      case 39: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 306 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy442,yymsp[0].minor.yy392);}
#line 2457 "parse.c"
        break;
      case 40: /* ccons ::= defer_subclause */
#line 307 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy392);}
#line 2462 "parse.c"
        break;
      case 41: /* ccons ::= COLLATE ID|STRING */
#line 308 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2467 "parse.c"
        break;
      case 44: /* refargs ::= */
#line 321 "parse.y"
{ yymsp[1].minor.yy392 = OE_None*0x0101; /* EV: R-19803-45884 */}
#line 2472 "parse.c"
        break;
      case 45: /* refargs ::= refargs refarg */
#line 322 "parse.y"
{ yymsp[-1].minor.yy392 = (yymsp[-1].minor.yy392 & ~yymsp[0].minor.yy207.mask) | yymsp[0].minor.yy207.value; }
#line 2477 "parse.c"
        break;
      case 46: /* refarg ::= MATCH nm */
#line 324 "parse.y"
{ yymsp[-1].minor.yy207.value = 0;     yymsp[-1].minor.yy207.mask = 0x000000; }
#line 2482 "parse.c"
        break;
      case 47: /* refarg ::= ON INSERT refact */
#line 325 "parse.y"
{ yymsp[-2].minor.yy207.value = 0;     yymsp[-2].minor.yy207.mask = 0x000000; }
#line 2487 "parse.c"
        break;
      case 48: /* refarg ::= ON DELETE refact */
#line 326 "parse.y"
{ yymsp[-2].minor.yy207.value = yymsp[0].minor.yy392;     yymsp[-2].minor.yy207.mask = 0x0000ff; }
#line 2492 "parse.c"
        break;
      case 49: /* refarg ::= ON UPDATE refact */
#line 327 "parse.y"
{ yymsp[-2].minor.yy207.value = yymsp[0].minor.yy392<<8;  yymsp[-2].minor.yy207.mask = 0x00ff00; }
#line 2497 "parse.c"
        break;
      case 50: /* refact ::= SET NULL */
#line 329 "parse.y"
{ yymsp[-1].minor.yy392 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2502 "parse.c"
        break;
      case 51: /* refact ::= SET DEFAULT */
#line 330 "parse.y"
{ yymsp[-1].minor.yy392 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2507 "parse.c"
        break;
      case 52: /* refact ::= CASCADE */
#line 331 "parse.y"
{ yymsp[0].minor.yy392 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2512 "parse.c"
        break;
      case 53: /* refact ::= RESTRICT */
#line 332 "parse.y"
{ yymsp[0].minor.yy392 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2517 "parse.c"
        break;
      case 54: /* refact ::= NO ACTION */
#line 333 "parse.y"
{ yymsp[-1].minor.yy392 = OE_None;     /* EV: R-33326-45252 */}
#line 2522 "parse.c"
        break;
      case 55: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 335 "parse.y"
{yymsp[-2].minor.yy392 = 0;}
#line 2527 "parse.c"
        break;
      case 56: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 71: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==71);
      case 142: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==142);
#line 336 "parse.y"
{yymsp[-1].minor.yy392 = yymsp[0].minor.yy392;}
#line 2534 "parse.c"
        break;
      case 58: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 75: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==75);
      case 183: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==183);
      case 186: /* in_op ::= NOT IN */ yytestcase(yyruleno==186);
      case 210: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==210);
#line 339 "parse.y"
{yymsp[-1].minor.yy392 = 1;}
#line 2543 "parse.c"
        break;
      case 59: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 340 "parse.y"
{yymsp[-1].minor.yy392 = 0;}
#line 2548 "parse.c"
        break;
      case 61: /* tconscomma ::= COMMA */
#line 346 "parse.y"
{pParse->constraintName.n = 0;}
#line 2553 "parse.c"
        break;
      case 63: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 350 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy442,yymsp[0].minor.yy392,yymsp[-2].minor.yy392,0);}
#line 2558 "parse.c"
        break;
      case 64: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 352 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[-2].minor.yy442,yymsp[0].minor.yy392,0,0,0,0);}
#line 2563 "parse.c"
        break;
      case 65: /* tcons ::= CHECK LP expr RP onconf */
#line 354 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy342.pExpr);}
#line 2568 "parse.c"
        break;
      case 66: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 356 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy442, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy442, yymsp[-1].minor.yy392);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy392);
}
#line 2576 "parse.c"
        break;
      case 68: /* onconf ::= */
      case 70: /* orconf ::= */ yytestcase(yyruleno==70);
#line 370 "parse.y"
{yymsp[1].minor.yy392 = OE_Default;}
#line 2582 "parse.c"
        break;
      case 69: /* onconf ::= ON CONFLICT resolvetype */
#line 371 "parse.y"
{yymsp[-2].minor.yy392 = yymsp[0].minor.yy392;}
#line 2587 "parse.c"
        break;
      case 72: /* resolvetype ::= IGNORE */
#line 375 "parse.y"
{yymsp[0].minor.yy392 = OE_Ignore;}
#line 2592 "parse.c"
        break;
      case 73: /* resolvetype ::= REPLACE */
      case 143: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==143);
#line 376 "parse.y"
{yymsp[0].minor.yy392 = OE_Replace;}
#line 2598 "parse.c"
        break;
      case 74: /* cmd ::= DROP TABLE ifexists fullname */
#line 380 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy347, 0, yymsp[-1].minor.yy392);
}
#line 2605 "parse.c"
        break;
      case 77: /* cmd ::= createkw temp VIEW ifnotexists nm dbnm eidlist_opt AS select */
#line 391 "parse.y"
{
  sqlite3CreateView(pParse, &yymsp[-8].minor.yy0, &yymsp[-4].minor.yy0, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy442, yymsp[0].minor.yy159, yymsp[-7].minor.yy392, yymsp[-5].minor.yy392);
}
#line 2612 "parse.c"
        break;
      case 78: /* cmd ::= DROP VIEW ifexists fullname */
#line 394 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy347, 1, yymsp[-1].minor.yy392);
}
#line 2619 "parse.c"
        break;
      case 79: /* cmd ::= select */
#line 401 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy159, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy159);
}
#line 2628 "parse.c"
        break;
      case 80: /* select ::= with selectnowith */
#line 438 "parse.y"
{
  Select *p = yymsp[0].minor.yy159;
  if( p ){
    p->pWith = yymsp[-1].minor.yy331;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy331);
  }
  yymsp[-1].minor.yy159 = p; /*A-overwrites-W*/
}
#line 2642 "parse.c"
        break;
      case 81: /* selectnowith ::= selectnowith multiselect_op oneselect */
#line 451 "parse.y"
{
  Select *pRhs = yymsp[0].minor.yy159;
  Select *pLhs = yymsp[-2].minor.yy159;
  if( pRhs && pRhs->pPrior ){
    SrcList *pFrom;
    Token x;
    x.n = 0;
    parserDoubleLinkSelect(pParse, pRhs);
    pFrom = sqlite3SrcListAppendFromTerm(pParse,0,0,0,&x,pRhs,0,0);
    pRhs = sqlite3SelectNew(pParse,0,pFrom,0,0,0,0,0,0,0);
  }
  if( pRhs ){
    pRhs->op = (u8)yymsp[-1].minor.yy392;
    pRhs->pPrior = pLhs;
    if( ALWAYS(pLhs) ) pLhs->selFlags &= ~SF_MultiValue;
    pRhs->selFlags &= ~SF_MultiValue;
    if( yymsp[-1].minor.yy392!=TK_ALL ) pParse->hasCompound = 1;
  }else{
    sqlite3SelectDelete(pParse->db, pLhs);
  }
  yymsp[-2].minor.yy159 = pRhs;
}
#line 2668 "parse.c"
        break;
      case 82: /* multiselect_op ::= UNION */
      case 84: /* multiselect_op ::= EXCEPT|INTERSECT */ yytestcase(yyruleno==84);
#line 474 "parse.y"
{yymsp[0].minor.yy392 = yymsp[0].major; /*A-overwrites-OP*/}
#line 2674 "parse.c"
        break;
      case 83: /* multiselect_op ::= UNION ALL */
#line 475 "parse.y"
{yymsp[-1].minor.yy392 = TK_ALL;}
#line 2679 "parse.c"
        break;
      case 85: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 479 "parse.y"
{
#if SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy159 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy442,yymsp[-5].minor.yy347,yymsp[-4].minor.yy122,yymsp[-3].minor.yy442,yymsp[-2].minor.yy122,yymsp[-1].minor.yy442,yymsp[-7].minor.yy392,yymsp[0].minor.yy64.pLimit,yymsp[0].minor.yy64.pOffset);
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
  if( yymsp[-8].minor.yy159!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy159->zSelName), yymsp[-8].minor.yy159->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy159->zSelName), yymsp[-8].minor.yy159->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2713 "parse.c"
        break;
      case 86: /* values ::= VALUES LP nexprlist RP */
#line 513 "parse.y"
{
  yymsp[-3].minor.yy159 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy442,0,0,0,0,0,SF_Values,0,0);
}
#line 2720 "parse.c"
        break;
      case 87: /* values ::= values COMMA LP exprlist RP */
#line 516 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy159;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy442,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy159 = pRight;
  }else{
    yymsp[-4].minor.yy159 = pLeft;
  }
}
#line 2736 "parse.c"
        break;
      case 88: /* distinct ::= DISTINCT */
#line 533 "parse.y"
{yymsp[0].minor.yy392 = SF_Distinct;}
#line 2741 "parse.c"
        break;
      case 89: /* distinct ::= ALL */
#line 534 "parse.y"
{yymsp[0].minor.yy392 = SF_All;}
#line 2746 "parse.c"
        break;
      case 91: /* sclp ::= */
      case 119: /* orderby_opt ::= */ yytestcase(yyruleno==119);
      case 126: /* groupby_opt ::= */ yytestcase(yyruleno==126);
      case 199: /* exprlist ::= */ yytestcase(yyruleno==199);
      case 205: /* eidlist_opt ::= */ yytestcase(yyruleno==205);
#line 547 "parse.y"
{yymsp[1].minor.yy442 = 0;}
#line 2755 "parse.c"
        break;
      case 92: /* selcollist ::= sclp expr as */
#line 548 "parse.y"
{
   yymsp[-2].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy442, yymsp[-1].minor.yy342.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy442, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy442,&yymsp[-1].minor.yy342);
}
#line 2764 "parse.c"
        break;
      case 93: /* selcollist ::= sclp STAR */
#line 553 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy442, p);
}
#line 2772 "parse.c"
        break;
      case 94: /* selcollist ::= sclp nm DOT STAR */
#line 557 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0, &yymsp[0].minor.yy0);
  Expr *pLeft = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight, 0);
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442, pDot);
}
#line 2782 "parse.c"
        break;
      case 95: /* as ::= AS nm */
      case 106: /* dbnm ::= DOT nm */ yytestcase(yyruleno==106);
      case 219: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==219);
      case 220: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==220);
#line 568 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2790 "parse.c"
        break;
      case 97: /* from ::= */
#line 582 "parse.y"
{yymsp[1].minor.yy347 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy347));}
#line 2795 "parse.c"
        break;
      case 98: /* from ::= FROM seltablist */
#line 583 "parse.y"
{
  yymsp[-1].minor.yy347 = yymsp[0].minor.yy347;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy347);
}
#line 2803 "parse.c"
        break;
      case 99: /* stl_prefix ::= seltablist joinop */
#line 591 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy347 && yymsp[-1].minor.yy347->nSrc>0) ) yymsp[-1].minor.yy347->a[yymsp[-1].minor.yy347->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy392;
}
#line 2810 "parse.c"
        break;
      case 100: /* stl_prefix ::= */
#line 594 "parse.y"
{yymsp[1].minor.yy347 = 0;}
#line 2815 "parse.c"
        break;
      case 101: /* seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt */
#line 596 "parse.y"
{
  yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,&yymsp[-5].minor.yy0,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  sqlite3SrcListIndexedBy(pParse, yymsp[-6].minor.yy347, &yymsp[-2].minor.yy0);
}
#line 2823 "parse.c"
        break;
      case 102: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt */
#line 601 "parse.y"
{
  yymsp[-8].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-8].minor.yy347,&yymsp[-7].minor.yy0,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  sqlite3SrcListFuncArgs(pParse, yymsp[-8].minor.yy347, yymsp[-4].minor.yy442);
}
#line 2831 "parse.c"
        break;
      case 103: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 607 "parse.y"
{
    yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy159,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
  }
#line 2838 "parse.c"
        break;
      case 104: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 611 "parse.y"
{
    if( yymsp[-6].minor.yy347==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy122==0 && yymsp[0].minor.yy180==0 ){
      yymsp[-6].minor.yy347 = yymsp[-4].minor.yy347;
    }else if( yymsp[-4].minor.yy347->nSrc==1 ){
      yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
      if( yymsp[-6].minor.yy347 ){
        struct SrcList_item *pNew = &yymsp[-6].minor.yy347->a[yymsp[-6].minor.yy347->nSrc-1];
        struct SrcList_item *pOld = yymsp[-4].minor.yy347->a;
        pNew->zName = pOld->zName;
        pNew->zDatabase = pOld->zDatabase;
        pNew->pSelect = pOld->pSelect;
        pOld->zName = pOld->zDatabase = 0;
        pOld->pSelect = 0;
      }
      sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy347);
    }else{
      Select *pSubquery;
      sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy347);
      pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy347,0,0,0,0,SF_NestedFrom,0,0);
      yymsp[-6].minor.yy347 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy347,0,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy122,yymsp[0].minor.yy180);
    }
  }
#line 2864 "parse.c"
        break;
      case 105: /* dbnm ::= */
      case 114: /* indexed_opt ::= */ yytestcase(yyruleno==114);
#line 636 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2870 "parse.c"
        break;
      case 107: /* fullname ::= nm dbnm */
#line 642 "parse.y"
{yymsp[-1].minor.yy347 = sqlite3SrcListAppend(pParse->db,0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2875 "parse.c"
        break;
      case 108: /* joinop ::= COMMA|JOIN */
#line 645 "parse.y"
{ yymsp[0].minor.yy392 = JT_INNER; }
#line 2880 "parse.c"
        break;
      case 109: /* joinop ::= JOIN_KW JOIN */
#line 647 "parse.y"
{yymsp[-1].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2885 "parse.c"
        break;
      case 110: /* joinop ::= JOIN_KW nm JOIN */
#line 649 "parse.y"
{yymsp[-2].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2890 "parse.c"
        break;
      case 111: /* joinop ::= JOIN_KW nm nm JOIN */
#line 651 "parse.y"
{yymsp[-3].minor.yy392 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2895 "parse.c"
        break;
      case 112: /* on_opt ::= ON expr */
      case 129: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==129);
      case 136: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==136);
      case 195: /* case_else ::= ELSE expr */ yytestcase(yyruleno==195);
#line 655 "parse.y"
{yymsp[-1].minor.yy122 = yymsp[0].minor.yy342.pExpr;}
#line 2903 "parse.c"
        break;
      case 113: /* on_opt ::= */
      case 128: /* having_opt ::= */ yytestcase(yyruleno==128);
      case 135: /* where_opt ::= */ yytestcase(yyruleno==135);
      case 196: /* case_else ::= */ yytestcase(yyruleno==196);
      case 198: /* case_operand ::= */ yytestcase(yyruleno==198);
#line 656 "parse.y"
{yymsp[1].minor.yy122 = 0;}
#line 2912 "parse.c"
        break;
      case 115: /* indexed_opt ::= INDEXED BY nm */
#line 670 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2917 "parse.c"
        break;
      case 116: /* indexed_opt ::= NOT INDEXED */
#line 671 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2922 "parse.c"
        break;
      case 117: /* using_opt ::= USING LP idlist RP */
#line 675 "parse.y"
{yymsp[-3].minor.yy180 = yymsp[-1].minor.yy180;}
#line 2927 "parse.c"
        break;
      case 118: /* using_opt ::= */
      case 144: /* idlist_opt ::= */ yytestcase(yyruleno==144);
#line 676 "parse.y"
{yymsp[1].minor.yy180 = 0;}
#line 2933 "parse.c"
        break;
      case 120: /* orderby_opt ::= ORDER BY sortlist */
      case 127: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==127);
#line 690 "parse.y"
{yymsp[-2].minor.yy442 = yymsp[0].minor.yy442;}
#line 2939 "parse.c"
        break;
      case 121: /* sortlist ::= sortlist COMMA expr sortorder */
#line 691 "parse.y"
{
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442,yymsp[-1].minor.yy342.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy442,yymsp[0].minor.yy392);
}
#line 2947 "parse.c"
        break;
      case 122: /* sortlist ::= expr sortorder */
#line 695 "parse.y"
{
  yymsp[-1].minor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy342.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy442,yymsp[0].minor.yy392);
}
#line 2955 "parse.c"
        break;
      case 123: /* sortorder ::= ASC */
#line 702 "parse.y"
{yymsp[0].minor.yy392 = SQLITE_SO_ASC;}
#line 2960 "parse.c"
        break;
      case 124: /* sortorder ::= DESC */
#line 703 "parse.y"
{yymsp[0].minor.yy392 = SQLITE_SO_DESC;}
#line 2965 "parse.c"
        break;
      case 125: /* sortorder ::= */
#line 704 "parse.y"
{yymsp[1].minor.yy392 = SQLITE_SO_UNDEFINED;}
#line 2970 "parse.c"
        break;
      case 130: /* limit_opt ::= */
#line 729 "parse.y"
{yymsp[1].minor.yy64.pLimit = 0; yymsp[1].minor.yy64.pOffset = 0;}
#line 2975 "parse.c"
        break;
      case 131: /* limit_opt ::= LIMIT expr */
#line 730 "parse.y"
{yymsp[-1].minor.yy64.pLimit = yymsp[0].minor.yy342.pExpr; yymsp[-1].minor.yy64.pOffset = 0;}
#line 2980 "parse.c"
        break;
      case 132: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 732 "parse.y"
{yymsp[-3].minor.yy64.pLimit = yymsp[-2].minor.yy342.pExpr; yymsp[-3].minor.yy64.pOffset = yymsp[0].minor.yy342.pExpr;}
#line 2985 "parse.c"
        break;
      case 133: /* limit_opt ::= LIMIT expr COMMA expr */
#line 734 "parse.y"
{yymsp[-3].minor.yy64.pOffset = yymsp[-2].minor.yy342.pExpr; yymsp[-3].minor.yy64.pLimit = yymsp[0].minor.yy342.pExpr;}
#line 2990 "parse.c"
        break;
      case 134: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 748 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy331, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy347, &yymsp[-1].minor.yy0);
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy347,yymsp[0].minor.yy122);
}
#line 2999 "parse.c"
        break;
      case 137: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 775 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy331, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy347, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy442,"set list"); 
  sqlite3Update(pParse,yymsp[-4].minor.yy347,yymsp[-1].minor.yy442,yymsp[0].minor.yy122,yymsp[-5].minor.yy392);
}
#line 3009 "parse.c"
        break;
      case 138: /* setlist ::= setlist COMMA nm EQ expr */
#line 786 "parse.y"
{
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy442, yymsp[0].minor.yy342.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy442, &yymsp[-2].minor.yy0, 1);
}
#line 3017 "parse.c"
        break;
      case 139: /* setlist ::= nm EQ expr */
#line 790 "parse.y"
{
  yylhsminor.yy442 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy342.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy442, &yymsp[-2].minor.yy0, 1);
}
#line 3025 "parse.c"
  yymsp[-2].minor.yy442 = yylhsminor.yy442;
        break;
      case 140: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 797 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy331, 1);
  sqlite3Insert(pParse, yymsp[-2].minor.yy347, yymsp[0].minor.yy159, yymsp[-1].minor.yy180, yymsp[-4].minor.yy392);
}
#line 3034 "parse.c"
        break;
      case 141: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 802 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy331, 1);
  sqlite3Insert(pParse, yymsp[-3].minor.yy347, 0, yymsp[-2].minor.yy180, yymsp[-5].minor.yy392);
}
#line 3042 "parse.c"
        break;
      case 145: /* idlist_opt ::= LP idlist RP */
#line 817 "parse.y"
{yymsp[-2].minor.yy180 = yymsp[-1].minor.yy180;}
#line 3047 "parse.c"
        break;
      case 146: /* idlist ::= idlist COMMA nm */
#line 819 "parse.y"
{yymsp[-2].minor.yy180 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy180,&yymsp[0].minor.yy0);}
#line 3052 "parse.c"
        break;
      case 147: /* idlist ::= nm */
#line 821 "parse.y"
{yymsp[0].minor.yy180 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 3057 "parse.c"
        break;
      case 148: /* expr ::= LP expr RP */
#line 854 "parse.y"
{spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy342.pExpr = yymsp[-1].minor.yy342.pExpr;}
#line 3062 "parse.c"
        break;
      case 149: /* term ::= NULL */
      case 154: /* term ::= INTEGER|FLOAT|BLOB */ yytestcase(yyruleno==154);
      case 155: /* term ::= STRING */ yytestcase(yyruleno==155);
#line 855 "parse.y"
{spanExpr(&yymsp[0].minor.yy342,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 3069 "parse.c"
        break;
      case 150: /* expr ::= ID|INDEXED */
      case 151: /* expr ::= JOIN_KW */ yytestcase(yyruleno==151);
#line 856 "parse.y"
{spanExpr(&yymsp[0].minor.yy342,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 3075 "parse.c"
        break;
      case 152: /* expr ::= nm DOT nm */
#line 858 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2, 0);
}
#line 3085 "parse.c"
        break;
      case 153: /* expr ::= nm DOT nm DOT nm */
#line 864 "parse.y"
{
  Expr *temp1 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-4].minor.yy0);
  Expr *temp2 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *temp3 = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[0].minor.yy0);
  Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3, 0);
  spanSet(&yymsp[-4].minor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4, 0);
}
#line 3097 "parse.c"
        break;
      case 156: /* expr ::= VARIABLE */
#line 874 "parse.y"
{
  if( !(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1])) ){
    spanExpr(&yymsp[0].minor.yy342, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy342.pExpr);
  }else{
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
    assert( t.n>=2 );
    spanSet(&yymsp[0].minor.yy342, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy342.pExpr = 0;
    }else{
      yymsp[0].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0, &t);
      if( yymsp[0].minor.yy342.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy342.pExpr->iTable);
    }
  }
}
#line 3121 "parse.c"
        break;
      case 157: /* expr ::= expr COLLATE ID|STRING */
#line 894 "parse.y"
{
  yymsp[-2].minor.yy342.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy342.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 3129 "parse.c"
        break;
      case 158: /* expr ::= CAST LP expr AS typetoken RP */
#line 899 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy342,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-5].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_CAST, yymsp[-3].minor.yy342.pExpr, 0, &yymsp[-1].minor.yy0);
}
#line 3137 "parse.c"
        break;
      case 159: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 904 "parse.y"
{
  if( yymsp[-1].minor.yy442 && yymsp[-1].minor.yy442->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy442, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy392==SF_Distinct && yylhsminor.yy342.pExpr ){
    yylhsminor.yy342.pExpr->flags |= EP_Distinct;
  }
}
#line 3151 "parse.c"
  yymsp[-4].minor.yy342 = yylhsminor.yy342;
        break;
      case 160: /* expr ::= ID|INDEXED LP STAR RP */
#line 914 "parse.y"
{
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 3160 "parse.c"
  yymsp[-3].minor.yy342 = yylhsminor.yy342;
        break;
      case 161: /* term ::= CTIME_KW */
#line 918 "parse.y"
{
  yylhsminor.yy342.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy342, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 3169 "parse.c"
  yymsp[0].minor.yy342 = yylhsminor.yy342;
        break;
      case 162: /* expr ::= expr AND expr */
      case 163: /* expr ::= expr OR expr */ yytestcase(yyruleno==163);
      case 164: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==164);
      case 165: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==165);
      case 166: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==166);
      case 167: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==167);
      case 168: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==168);
      case 169: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==169);
#line 947 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);}
#line 3182 "parse.c"
        break;
      case 170: /* likeop ::= LIKE_KW|MATCH */
#line 960 "parse.y"
{yymsp[0].minor.yy318.eOperator = yymsp[0].minor.yy0; yymsp[0].minor.yy318.bNot = 0;/*A-overwrites-X*/}
#line 3187 "parse.c"
        break;
      case 171: /* likeop ::= NOT LIKE_KW|MATCH */
#line 961 "parse.y"
{yymsp[-1].minor.yy318.eOperator = yymsp[0].minor.yy0; yymsp[-1].minor.yy318.bNot = 1;}
#line 3192 "parse.c"
        break;
      case 172: /* expr ::= expr likeop expr */
#line 962 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy342.pExpr);
  yymsp[-2].minor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy318.eOperator);
  exprNot(pParse, yymsp[-1].minor.yy318.bNot, &yymsp[-2].minor.yy342);
  yymsp[-2].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
  if( yymsp[-2].minor.yy342.pExpr ) yymsp[-2].minor.yy342.pExpr->flags |= EP_InfixFunc;
}
#line 3205 "parse.c"
        break;
      case 173: /* expr ::= expr likeop expr ESCAPE expr */
#line 971 "parse.y"
{
  ExprList *pList;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
  yymsp[-4].minor.yy342.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy318.eOperator);
  exprNot(pParse, yymsp[-3].minor.yy318.bNot, &yymsp[-4].minor.yy342);
  yymsp[-4].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
  if( yymsp[-4].minor.yy342.pExpr ) yymsp[-4].minor.yy342.pExpr->flags |= EP_InfixFunc;
}
#line 3219 "parse.c"
        break;
      case 174: /* expr ::= expr ISNULL|NOTNULL */
#line 996 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy342,&yymsp[0].minor.yy0);}
#line 3224 "parse.c"
        break;
      case 175: /* expr ::= expr NOT NULL */
#line 997 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy0);}
#line 3229 "parse.c"
        break;
      case 176: /* expr ::= expr IS expr */
#line 1018 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy342,&yymsp[0].minor.yy342);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yymsp[-2].minor.yy342.pExpr, TK_ISNULL);
}
#line 3237 "parse.c"
        break;
      case 177: /* expr ::= expr IS NOT expr */
#line 1022 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy342,&yymsp[0].minor.yy342);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy342.pExpr, yymsp[-3].minor.yy342.pExpr, TK_NOTNULL);
}
#line 3245 "parse.c"
        break;
      case 178: /* expr ::= NOT expr */
      case 179: /* expr ::= BITNOT expr */ yytestcase(yyruleno==179);
#line 1046 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,yymsp[-1].major,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3251 "parse.c"
        break;
      case 180: /* expr ::= MINUS expr */
#line 1050 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,TK_UMINUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3256 "parse.c"
        break;
      case 181: /* expr ::= PLUS expr */
#line 1052 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy342,pParse,TK_UPLUS,&yymsp[0].minor.yy342,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3261 "parse.c"
        break;
      case 182: /* between_op ::= BETWEEN */
      case 185: /* in_op ::= IN */ yytestcase(yyruleno==185);
#line 1055 "parse.y"
{yymsp[0].minor.yy392 = 0;}
#line 3267 "parse.c"
        break;
      case 184: /* expr ::= expr between_op expr AND expr */
#line 1057 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy342.pExpr);
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy342.pExpr, 0, 0);
  if( yymsp[-4].minor.yy342.pExpr ){
    yymsp[-4].minor.yy342.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
  yymsp[-4].minor.yy342.zEnd = yymsp[0].minor.yy342.zEnd;
}
#line 3283 "parse.c"
        break;
      case 187: /* expr ::= expr in_op LP exprlist RP */
#line 1073 "parse.y"
{
    if( yymsp[-1].minor.yy442==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy342.pExpr);
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_INTEGER, 0, 0, &sqlite3IntTokens[yymsp[-3].minor.yy392]);
    }else if( yymsp[-1].minor.yy442->nExpr==1 ){
      /* Expressions of the form:
      **
      **      expr1 IN (?1)
      **      expr1 NOT IN (?2)
      **
      ** with exactly one value on the RHS can be simplified to something
      ** like this:
      **
      **      expr1 == ?1
      **      expr1 <> ?2
      **
      ** But, the RHS of the == or <> is marked with the EP_Generic flag
      ** so that it may not contribute to the computation of comparison
      ** affinity or the collating sequence to use for comparison.  Otherwise,
      ** the semantics would be subtly different from IN or NOT IN.
      */
      Expr *pRHS = yymsp[-1].minor.yy442->a[0].pExpr;
      yymsp[-1].minor.yy442->a[0].pExpr = 0;
      sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy442);
      /* pRHS cannot be NULL because a malloc error would have been detected
      ** before now and control would have never reached this point */
      if( ALWAYS(pRHS) ){
        pRHS->flags &= ~EP_Collate;
        pRHS->flags |= EP_Generic;
      }
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy392 ? TK_NE : TK_EQ, yymsp[-4].minor.yy342.pExpr, pRHS, 0);
    }else{
      yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
      if( yymsp[-4].minor.yy342.pExpr ){
        yymsp[-4].minor.yy342.pExpr->x.pList = yymsp[-1].minor.yy442;
        sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy342.pExpr);
      }else{
        sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy442);
      }
      exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
    }
    yymsp[-4].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3338 "parse.c"
        break;
      case 188: /* expr ::= LP select RP */
#line 1124 "parse.y"
{
    spanSet(&yymsp[-2].minor.yy342,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    yymsp[-2].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-2].minor.yy342.pExpr, yymsp[-1].minor.yy159);
  }
#line 3347 "parse.c"
        break;
      case 189: /* expr ::= expr in_op LP select RP */
#line 1129 "parse.y"
{
    yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy342.pExpr, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy342.pExpr, yymsp[-1].minor.yy159);
    exprNot(pParse, yymsp[-3].minor.yy392, &yymsp[-4].minor.yy342);
    yymsp[-4].minor.yy342.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3357 "parse.c"
        break;
      case 190: /* expr ::= expr in_op nm dbnm */
#line 1135 "parse.y"
{
    SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);
    Select *pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
    yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-3].minor.yy342.pExpr, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-3].minor.yy342.pExpr, pSelect);
    exprNot(pParse, yymsp[-2].minor.yy392, &yymsp[-3].minor.yy342);
    yymsp[-3].minor.yy342.zEnd = yymsp[0].minor.yy0.z ? &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] : &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n];
  }
#line 3369 "parse.c"
        break;
      case 191: /* expr ::= EXISTS LP select RP */
#line 1143 "parse.y"
{
    Expr *p;
    spanSet(&yymsp[-3].minor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    p = yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0, 0);
    sqlite3PExprAddSelect(pParse, p, yymsp[-1].minor.yy159);
  }
#line 3379 "parse.c"
        break;
      case 192: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1152 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy342,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy122, 0, 0);
  if( yymsp[-4].minor.yy342.pExpr ){
    yymsp[-4].minor.yy342.pExpr->x.pList = yymsp[-1].minor.yy122 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[-1].minor.yy122) : yymsp[-2].minor.yy442;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy342.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy442);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy122);
  }
}
#line 3394 "parse.c"
        break;
      case 193: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1165 "parse.y"
{
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, yymsp[-2].minor.yy342.pExpr);
  yymsp[-4].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy442, yymsp[0].minor.yy342.pExpr);
}
#line 3402 "parse.c"
        break;
      case 194: /* case_exprlist ::= WHEN expr THEN expr */
#line 1169 "parse.y"
{
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy342.pExpr);
  yymsp[-3].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy442, yymsp[0].minor.yy342.pExpr);
}
#line 3410 "parse.c"
        break;
      case 197: /* case_operand ::= expr */
#line 1179 "parse.y"
{yymsp[0].minor.yy122 = yymsp[0].minor.yy342.pExpr; /*A-overwrites-X*/}
#line 3415 "parse.c"
        break;
      case 200: /* nexprlist ::= nexprlist COMMA expr */
#line 1190 "parse.y"
{yymsp[-2].minor.yy442 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy442,yymsp[0].minor.yy342.pExpr);}
#line 3420 "parse.c"
        break;
      case 201: /* nexprlist ::= expr */
#line 1192 "parse.y"
{yymsp[0].minor.yy442 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy342.pExpr); /*A-overwrites-Y*/}
#line 3425 "parse.c"
        break;
      case 202: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
#line 1198 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy442, yymsp[-10].minor.yy392,
                      &yymsp[-11].minor.yy0, yymsp[0].minor.yy122, SQLITE_SO_ASC, yymsp[-8].minor.yy392);
}
#line 3434 "parse.c"
        break;
      case 203: /* uniqueflag ::= UNIQUE */
      case 244: /* raisetype ::= ABORT */ yytestcase(yyruleno==244);
#line 1205 "parse.y"
{yymsp[0].minor.yy392 = OE_Abort;}
#line 3440 "parse.c"
        break;
      case 204: /* uniqueflag ::= */
#line 1206 "parse.y"
{yymsp[1].minor.yy392 = OE_None;}
#line 3445 "parse.c"
        break;
      case 206: /* eidlist_opt ::= LP eidlist RP */
#line 1255 "parse.y"
{yymsp[-2].minor.yy442 = yymsp[-1].minor.yy442;}
#line 3450 "parse.c"
        break;
      case 207: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1256 "parse.y"
{
  yymsp[-4].minor.yy442 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy442, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy392, yymsp[0].minor.yy392);
}
#line 3457 "parse.c"
        break;
      case 208: /* eidlist ::= nm collate sortorder */
#line 1259 "parse.y"
{
  yymsp[-2].minor.yy442 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy392, yymsp[0].minor.yy392); /*A-overwrites-Y*/
}
#line 3464 "parse.c"
        break;
      case 211: /* cmd ::= DROP INDEX ifexists fullname */
#line 1270 "parse.y"
{sqlite3DropIndex(pParse, yymsp[0].minor.yy347, yymsp[-1].minor.yy392);}
#line 3469 "parse.c"
        break;
      case 212: /* cmd ::= VACUUM */
      case 213: /* cmd ::= VACUUM nm */ yytestcase(yyruleno==213);
#line 1276 "parse.y"
{sqlite3Vacuum(pParse);}
#line 3475 "parse.c"
        break;
      case 214: /* cmd ::= PRAGMA nm dbnm */
#line 1284 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,0,0);}
#line 3480 "parse.c"
        break;
      case 215: /* cmd ::= PRAGMA nm dbnm EQ nmnum */
#line 1285 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,0);}
#line 3485 "parse.c"
        break;
      case 216: /* cmd ::= PRAGMA nm dbnm LP nmnum RP */
#line 1286 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,0);}
#line 3490 "parse.c"
        break;
      case 217: /* cmd ::= PRAGMA nm dbnm EQ minus_num */
#line 1288 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0,1);}
#line 3495 "parse.c"
        break;
      case 218: /* cmd ::= PRAGMA nm dbnm LP minus_num RP */
#line 1290 "parse.y"
{sqlite3Pragma(pParse,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,&yymsp[-1].minor.yy0,1);}
#line 3500 "parse.c"
        break;
      case 221: /* cmd ::= createkw trigger_decl BEGIN trigger_cmd_list END */
#line 1306 "parse.y"
{
  Token all;
  all.z = yymsp[-3].minor.yy0.z;
  all.n = (int)(yymsp[0].minor.yy0.z - yymsp[-3].minor.yy0.z) + yymsp[0].minor.yy0.n;
  sqlite3FinishTrigger(pParse, yymsp[-1].minor.yy327, &all);
}
#line 3510 "parse.c"
        break;
      case 222: /* trigger_decl ::= temp TRIGGER ifnotexists nm dbnm trigger_time trigger_event ON fullname foreach_clause when_clause */
#line 1315 "parse.y"
{
  sqlite3BeginTrigger(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, yymsp[-5].minor.yy392, yymsp[-4].minor.yy410.a, yymsp[-4].minor.yy410.b, yymsp[-2].minor.yy347, yymsp[0].minor.yy122, yymsp[-10].minor.yy392, yymsp[-8].minor.yy392);
  yymsp[-10].minor.yy0 = (yymsp[-6].minor.yy0.n==0?yymsp[-7].minor.yy0:yymsp[-6].minor.yy0); /*A-overwrites-T*/
}
#line 3518 "parse.c"
        break;
      case 223: /* trigger_time ::= BEFORE */
#line 1321 "parse.y"
{ yymsp[0].minor.yy392 = TK_BEFORE; }
#line 3523 "parse.c"
        break;
      case 224: /* trigger_time ::= AFTER */
#line 1322 "parse.y"
{ yymsp[0].minor.yy392 = TK_AFTER;  }
#line 3528 "parse.c"
        break;
      case 225: /* trigger_time ::= INSTEAD OF */
#line 1323 "parse.y"
{ yymsp[-1].minor.yy392 = TK_INSTEAD;}
#line 3533 "parse.c"
        break;
      case 226: /* trigger_time ::= */
#line 1324 "parse.y"
{ yymsp[1].minor.yy392 = TK_BEFORE; }
#line 3538 "parse.c"
        break;
      case 227: /* trigger_event ::= DELETE|INSERT */
      case 228: /* trigger_event ::= UPDATE */ yytestcase(yyruleno==228);
#line 1328 "parse.y"
{yymsp[0].minor.yy410.a = yymsp[0].major; /*A-overwrites-X*/ yymsp[0].minor.yy410.b = 0;}
#line 3544 "parse.c"
        break;
      case 229: /* trigger_event ::= UPDATE OF idlist */
#line 1330 "parse.y"
{yymsp[-2].minor.yy410.a = TK_UPDATE; yymsp[-2].minor.yy410.b = yymsp[0].minor.yy180;}
#line 3549 "parse.c"
        break;
      case 230: /* when_clause ::= */
      case 249: /* key_opt ::= */ yytestcase(yyruleno==249);
#line 1337 "parse.y"
{ yymsp[1].minor.yy122 = 0; }
#line 3555 "parse.c"
        break;
      case 231: /* when_clause ::= WHEN expr */
      case 250: /* key_opt ::= KEY expr */ yytestcase(yyruleno==250);
#line 1338 "parse.y"
{ yymsp[-1].minor.yy122 = yymsp[0].minor.yy342.pExpr; }
#line 3561 "parse.c"
        break;
      case 232: /* trigger_cmd_list ::= trigger_cmd_list trigger_cmd SEMI */
#line 1342 "parse.y"
{
  assert( yymsp[-2].minor.yy327!=0 );
  yymsp[-2].minor.yy327->pLast->pNext = yymsp[-1].minor.yy327;
  yymsp[-2].minor.yy327->pLast = yymsp[-1].minor.yy327;
}
#line 3570 "parse.c"
        break;
      case 233: /* trigger_cmd_list ::= trigger_cmd SEMI */
#line 1347 "parse.y"
{ 
  assert( yymsp[-1].minor.yy327!=0 );
  yymsp[-1].minor.yy327->pLast = yymsp[-1].minor.yy327;
}
#line 3578 "parse.c"
        break;
      case 234: /* trnm ::= nm DOT nm */
#line 1358 "parse.y"
{
  yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;
  sqlite3ErrorMsg(pParse, 
        "qualified table names are not allowed on INSERT, UPDATE, and DELETE "
        "statements within triggers");
}
#line 3588 "parse.c"
        break;
      case 235: /* tridxby ::= INDEXED BY nm */
#line 1370 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the INDEXED BY clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3597 "parse.c"
        break;
      case 236: /* tridxby ::= NOT INDEXED */
#line 1375 "parse.y"
{
  sqlite3ErrorMsg(pParse,
        "the NOT INDEXED clause is not allowed on UPDATE or DELETE statements "
        "within triggers");
}
#line 3606 "parse.c"
        break;
      case 237: /* trigger_cmd ::= UPDATE orconf trnm tridxby SET setlist where_opt */
#line 1388 "parse.y"
{yymsp[-6].minor.yy327 = sqlite3TriggerUpdateStep(pParse->db, &yymsp[-4].minor.yy0, yymsp[-1].minor.yy442, yymsp[0].minor.yy122, yymsp[-5].minor.yy392);}
#line 3611 "parse.c"
        break;
      case 238: /* trigger_cmd ::= insert_cmd INTO trnm idlist_opt select */
#line 1392 "parse.y"
{yymsp[-4].minor.yy327 = sqlite3TriggerInsertStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy180, yymsp[0].minor.yy159, yymsp[-4].minor.yy392);/*A-overwrites-R*/}
#line 3616 "parse.c"
        break;
      case 239: /* trigger_cmd ::= DELETE FROM trnm tridxby where_opt */
#line 1396 "parse.y"
{yymsp[-4].minor.yy327 = sqlite3TriggerDeleteStep(pParse->db, &yymsp[-2].minor.yy0, yymsp[0].minor.yy122);}
#line 3621 "parse.c"
        break;
      case 240: /* trigger_cmd ::= select */
#line 1400 "parse.y"
{yymsp[0].minor.yy327 = sqlite3TriggerSelectStep(pParse->db, yymsp[0].minor.yy159); /*A-overwrites-X*/}
#line 3626 "parse.c"
        break;
      case 241: /* expr ::= RAISE LP IGNORE RP */
#line 1403 "parse.y"
{
  spanSet(&yymsp[-3].minor.yy342,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-3].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, 0); 
  if( yymsp[-3].minor.yy342.pExpr ){
    yymsp[-3].minor.yy342.pExpr->affinity = OE_Ignore;
  }
}
#line 3637 "parse.c"
        break;
      case 242: /* expr ::= RAISE LP raisetype COMMA nm RP */
#line 1410 "parse.y"
{
  spanSet(&yymsp[-5].minor.yy342,&yymsp[-5].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-X*/
  yymsp[-5].minor.yy342.pExpr = sqlite3PExpr(pParse, TK_RAISE, 0, 0, &yymsp[-1].minor.yy0); 
  if( yymsp[-5].minor.yy342.pExpr ) {
    yymsp[-5].minor.yy342.pExpr->affinity = (char)yymsp[-3].minor.yy392;
  }
}
#line 3648 "parse.c"
        break;
      case 243: /* raisetype ::= ROLLBACK */
#line 1420 "parse.y"
{yymsp[0].minor.yy392 = OE_Rollback;}
#line 3653 "parse.c"
        break;
      case 245: /* raisetype ::= FAIL */
#line 1422 "parse.y"
{yymsp[0].minor.yy392 = OE_Fail;}
#line 3658 "parse.c"
        break;
      case 246: /* cmd ::= DROP TRIGGER ifexists fullname */
#line 1427 "parse.y"
{
  sqlite3DropTrigger(pParse,yymsp[0].minor.yy347,yymsp[-1].minor.yy392);
}
#line 3665 "parse.c"
        break;
      case 247: /* cmd ::= ATTACH database_kw_opt expr AS expr key_opt */
#line 1434 "parse.y"
{
  sqlite3Attach(pParse, yymsp[-3].minor.yy342.pExpr, yymsp[-1].minor.yy342.pExpr, yymsp[0].minor.yy122);
}
#line 3672 "parse.c"
        break;
      case 248: /* cmd ::= DETACH database_kw_opt expr */
#line 1437 "parse.y"
{
  sqlite3Detach(pParse, yymsp[0].minor.yy342.pExpr);
}
#line 3679 "parse.c"
        break;
      case 251: /* cmd ::= REINDEX */
#line 1452 "parse.y"
{sqlite3Reindex(pParse, 0, 0);}
#line 3684 "parse.c"
        break;
      case 252: /* cmd ::= REINDEX nm dbnm */
#line 1453 "parse.y"
{sqlite3Reindex(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
#line 3689 "parse.c"
        break;
      case 253: /* cmd ::= ANALYZE */
#line 1458 "parse.y"
{sqlite3Analyze(pParse, 0, 0);}
#line 3694 "parse.c"
        break;
      case 254: /* cmd ::= ANALYZE nm dbnm */
#line 1459 "parse.y"
{sqlite3Analyze(pParse, &yymsp[-1].minor.yy0, &yymsp[0].minor.yy0);}
#line 3699 "parse.c"
        break;
      case 255: /* cmd ::= ALTER TABLE fullname RENAME TO nm */
#line 1464 "parse.y"
{
  sqlite3AlterRenameTable(pParse,yymsp[-3].minor.yy347,&yymsp[0].minor.yy0);
}
#line 3706 "parse.c"
        break;
      case 256: /* cmd ::= ALTER TABLE add_column_fullname ADD kwcolumn_opt columnname carglist */
#line 1468 "parse.y"
{
  yymsp[-1].minor.yy0.n = (int)(pParse->sLastToken.z-yymsp[-1].minor.yy0.z) + pParse->sLastToken.n;
  sqlite3AlterFinishAddColumn(pParse, &yymsp[-1].minor.yy0);
}
#line 3714 "parse.c"
        break;
      case 257: /* add_column_fullname ::= fullname */
#line 1472 "parse.y"
{
  disableLookaside(pParse);
  sqlite3AlterBeginAddColumn(pParse, yymsp[0].minor.yy347);
}
#line 3722 "parse.c"
        break;
      case 258: /* cmd ::= create_vtab */
#line 1482 "parse.y"
{sqlite3VtabFinishParse(pParse,0);}
#line 3727 "parse.c"
        break;
      case 259: /* cmd ::= create_vtab LP vtabarglist RP */
#line 1483 "parse.y"
{sqlite3VtabFinishParse(pParse,&yymsp[0].minor.yy0);}
#line 3732 "parse.c"
        break;
      case 260: /* create_vtab ::= createkw VIRTUAL TABLE ifnotexists nm dbnm USING nm */
#line 1485 "parse.y"
{
    sqlite3VtabBeginParse(pParse, &yymsp[-3].minor.yy0, &yymsp[-2].minor.yy0, &yymsp[0].minor.yy0, yymsp[-4].minor.yy392);
}
#line 3739 "parse.c"
        break;
      case 261: /* vtabarg ::= */
#line 1490 "parse.y"
{sqlite3VtabArgInit(pParse);}
#line 3744 "parse.c"
        break;
      case 262: /* vtabargtoken ::= ANY */
      case 263: /* vtabargtoken ::= lp anylist RP */ yytestcase(yyruleno==263);
      case 264: /* lp ::= LP */ yytestcase(yyruleno==264);
#line 1492 "parse.y"
{sqlite3VtabArgExtend(pParse,&yymsp[0].minor.yy0);}
#line 3751 "parse.c"
        break;
      case 265: /* with ::= */
#line 1507 "parse.y"
{yymsp[1].minor.yy331 = 0;}
#line 3756 "parse.c"
        break;
      case 266: /* with ::= WITH wqlist */
#line 1509 "parse.y"
{ yymsp[-1].minor.yy331 = yymsp[0].minor.yy331; }
#line 3761 "parse.c"
        break;
      case 267: /* with ::= WITH RECURSIVE wqlist */
#line 1510 "parse.y"
{ yymsp[-2].minor.yy331 = yymsp[0].minor.yy331; }
#line 3766 "parse.c"
        break;
      case 268: /* wqlist ::= nm eidlist_opt AS LP select RP */
#line 1512 "parse.y"
{
  yymsp[-5].minor.yy331 = sqlite3WithAdd(pParse, 0, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy442, yymsp[-1].minor.yy159); /*A-overwrites-X*/
}
#line 3773 "parse.c"
        break;
      case 269: /* wqlist ::= wqlist COMMA nm eidlist_opt AS LP select RP */
#line 1515 "parse.y"
{
  yymsp[-7].minor.yy331 = sqlite3WithAdd(pParse, yymsp[-7].minor.yy331, &yymsp[-5].minor.yy0, yymsp[-4].minor.yy442, yymsp[-1].minor.yy159);
}
#line 3780 "parse.c"
        break;
      default:
      /* (270) input ::= cmdlist */ yytestcase(yyruleno==270);
      /* (271) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==271);
      /* (272) cmdlist ::= ecmd */ yytestcase(yyruleno==272);
      /* (273) ecmd ::= SEMI */ yytestcase(yyruleno==273);
      /* (274) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==274);
      /* (275) explain ::= */ yytestcase(yyruleno==275);
      /* (276) trans_opt ::= */ yytestcase(yyruleno==276);
      /* (277) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==277);
      /* (278) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==278);
      /* (279) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==279);
      /* (280) savepoint_opt ::= */ yytestcase(yyruleno==280);
      /* (281) cmd ::= create_table create_table_args */ yytestcase(yyruleno==281);
      /* (282) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==282);
      /* (283) columnlist ::= columnname carglist */ yytestcase(yyruleno==283);
      /* (284) nm ::= ID|INDEXED */ yytestcase(yyruleno==284);
      /* (285) nm ::= STRING */ yytestcase(yyruleno==285);
      /* (286) nm ::= JOIN_KW */ yytestcase(yyruleno==286);
      /* (287) typetoken ::= typename */ yytestcase(yyruleno==287);
      /* (288) typename ::= ID|STRING */ yytestcase(yyruleno==288);
      /* (289) signed ::= plus_num */ yytestcase(yyruleno==289);
      /* (290) signed ::= minus_num */ yytestcase(yyruleno==290);
      /* (291) carglist ::= carglist ccons */ yytestcase(yyruleno==291);
      /* (292) carglist ::= */ yytestcase(yyruleno==292);
      /* (293) ccons ::= NULL onconf */ yytestcase(yyruleno==293);
      /* (294) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==294);
      /* (295) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==295);
      /* (296) conslist ::= tcons */ yytestcase(yyruleno==296);
      /* (297) tconscomma ::= */ yytestcase(yyruleno==297);
      /* (298) defer_subclause_opt ::= defer_subclause */ yytestcase(yyruleno==298);
      /* (299) resolvetype ::= raisetype */ yytestcase(yyruleno==299);
      /* (300) selectnowith ::= oneselect */ yytestcase(yyruleno==300);
      /* (301) oneselect ::= values */ yytestcase(yyruleno==301);
      /* (302) sclp ::= selcollist COMMA */ yytestcase(yyruleno==302);
      /* (303) as ::= ID|STRING */ yytestcase(yyruleno==303);
      /* (304) expr ::= term */ yytestcase(yyruleno==304);
      /* (305) exprlist ::= nexprlist */ yytestcase(yyruleno==305);
      /* (306) nmnum ::= plus_num */ yytestcase(yyruleno==306);
      /* (307) nmnum ::= nm */ yytestcase(yyruleno==307);
      /* (308) nmnum ::= ON */ yytestcase(yyruleno==308);
      /* (309) nmnum ::= DELETE */ yytestcase(yyruleno==309);
      /* (310) nmnum ::= DEFAULT */ yytestcase(yyruleno==310);
      /* (311) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==311);
      /* (312) foreach_clause ::= */ yytestcase(yyruleno==312);
      /* (313) foreach_clause ::= FOR EACH ROW */ yytestcase(yyruleno==313);
      /* (314) trnm ::= nm */ yytestcase(yyruleno==314);
      /* (315) tridxby ::= */ yytestcase(yyruleno==315);
      /* (316) database_kw_opt ::= DATABASE */ yytestcase(yyruleno==316);
      /* (317) database_kw_opt ::= */ yytestcase(yyruleno==317);
      /* (318) kwcolumn_opt ::= */ yytestcase(yyruleno==318);
      /* (319) kwcolumn_opt ::= COLUMNKW */ yytestcase(yyruleno==319);
      /* (320) vtabarglist ::= vtabarg */ yytestcase(yyruleno==320);
      /* (321) vtabarglist ::= vtabarglist COMMA vtabarg */ yytestcase(yyruleno==321);
      /* (322) vtabarg ::= vtabarg vtabargtoken */ yytestcase(yyruleno==322);
      /* (323) anylist ::= */ yytestcase(yyruleno==323);
      /* (324) anylist ::= anylist LP anylist RP */ yytestcase(yyruleno==324);
      /* (325) anylist ::= anylist ANY */ yytestcase(yyruleno==325);
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
#line 3898 "parse.c"
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
