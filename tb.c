#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN64
#include <Windows.h>
#else
#include <unistd.h>
#endif

//-----------------------------------------------------------------------------

#ifndef CLK_TCK
#define CLK_TICK CLOCKS_PER_SEC
#endif

bool streql(const char *s1, const char *s2) { return strcmp(s1, s2) == 0; }

bool strneql(const char *s1, const char *s2) { return strcmp(s1, s2) != 0; }

char *m_strdup(char *cp)
{
  char *mp = malloc(strlen(cp) + 1);
  if (mp == NULL)
  {
    return NULL;
  }
#ifdef _WIN64
  strncpy_s(mp, strlen(cp), cp, strlen(cp));
#else
  strncpy(mp, cp, strlen(cp));
#endif
  return mp;
}

// グローバル変数の配列を初期化するためマクロにしている
#define c_maxlines 7000 // プログラムMAX行
#define c_at_max 1000   // 配列変数の長さ
#define c_maxvars 26    // 変数の個数
#define c_g_stack 100   // GOSUBスタックのサイズ

//-----------------------------------------------------------------------------

// トークン種別
typedef enum
{
  kNONE,   // NONE
  kPUNCT,  // 句読文字 #()*+,-/:;<=>?@\\^
  kIDENT,  // 変数割り当て
  kNUMBER, // 数値
  kSTRING, // 文字列
} toktype_t;

//-----------------------------------------------------------------------------

char tok[1024];            // 現在読んでいるトークン
toktype_t toktype;         // 現在読んでいるトークン種別
int num;                   // 現在読んでいるプログラムの行番号
unsigned textp;            // 現在読んでいるプログラムのテキストを指すのポインタ
char *thelin;              // 現在のプログラム行?
char thech;                // 現在のプログラム文字?
char *pgm[c_maxlines + 1]; // BASICソースコード
int curline;               // 今読んでいるソースコードの行
bool errors;               // エラー発生中フラグ
bool tracing;              // デバッグモードフラグ
bool need_colon;           // コロンが必用フラグ
int gsp;                   // GOSUB 管理用インデックス
int gstackln[c_g_stack];   // GOSUB 行番号
int gstacktp[c_g_stack];   // GOSUB プログラム文字列
clock_t timestart;         // 開始時間

int vars[c_maxvars + 1]; // 変数
int atarry[c_at_max];    // 配列変数

int forvar[c_maxvars];   // FOR文の変数?
int forlimit[c_maxvars]; // FORROF文の限界?
int forline[c_maxvars];  // FOR文の行数?
int forpos[c_maxvars];   // FOR文の位置?

//----------------------------------------------------------------------------

bool tb_accept(const char *s);
void arrassn(void);
void assign(void);
void clearvars(void);
void docmd(void);
bool expect(const char *s);
int expression(int minprec);
void forstmt(void);
void getch(void);
char *getfilename(char action[]);
int getvarindex(void);
void gosubstmt(void);
void gotostmt(void);
void help(void);
void ifstmt(void);
void initlex(int n);
void initlex2(void);
void inputstmt(void);
void liststmt(void);
void loadstmt(void);
char *mygetline(FILE *fp);
void newstmt(void);
void nextstmt(void);
void nexttok(void);
int parenexpr(void);
void printstmt(void);
void readident(void);
void readint(void);
void readstr(void);
void returnstmt(void);
int rnd(int range);
void runstmt(void);
void savestmt(void);
void showtime(bool running);
void skiptoeol(void);
bool validlinenum(void);
void debugLog(const char *msg);
void sleepstmt(void);

//----------------------------------------------------------------------------

// MAIN PROGRAM
int main(int argc, char *argv[])
{
  if (argc > 1)
  {
    // 入力をファイル名を表す文字列と解釈
    toktype = kSTRING;
    sprintf(tok, "\"%s", argv[1]);
    loadstmt();
    // RUNコマンドを実行
    toktype = kIDENT;

#ifdef _WIN64
    strncpy_s(tok, sizeof(tok), "run", sizeof(tok) - 1);
#else
    strncpy(tok, "run", strlen(tok) - 1);
#endif
    docmd();
  }
  else
  {
    newstmt();
    help();
  }
  while (true)
  {
    errors = false;
    printf("c> ");
    // 前回のデータを削除
    if (pgm[0])
    {
      free(pgm[0]);
    }
    pgm[0] = mygetline(stdin);
    if (pgm[0] && pgm[0][0] != '\0')
    {
      initlex(0);
      if (toktype == kNUMBER)
      {
        if (validlinenum())
        {
          pgm[num] = m_strdup(pgm[0] + textp);
        }
      }
      else
      {
        docmd();
      }
    }
  }
  return 0;
}

// コマンド実行
void docmd(void)
{
  debugLog("@@@ docmd");
  bool running = false;
  while (true)
  {
    need_colon = true;
    if (tracing && tok[0] != ':' && thelin && textp < strlen(thelin))
    {
      printf("%d |  ", __LINE__);
      printf("[%d] %s\n", curline, &thelin[textp - 1]);
    }
    if (tb_accept("bye") || tb_accept("quit") || tb_accept("exit"))
    {
      exit(0);
    }
    else if (tb_accept("end") || tb_accept("stop"))
    {
      showtime(running);
      return;
    }
    else if (tb_accept("end") || tb_accept("stop"))
    {
      showtime(running);
      return;
    }
    else if (tb_accept("clear"))
    {
      clearvars();
      return;
    }
    else if (tb_accept("help"))
    {
      help();
      return;
    }
    else if (tb_accept("sleep"))
    {
      sleepstmt();
      return;
    }
    else if (tb_accept("list"))
    {
      liststmt();
      return;
    }
    else if (tb_accept("load"))
    {
      loadstmt();
      return;
    }
    else if (tb_accept("new"))
    {
      newstmt();
      return;
    }
    else if (tb_accept("run"))
    {
      runstmt();
      running = true;
      need_colon = false;
    }
    else if (tb_accept("save"))
    {
      savestmt();
      return;
    }
    else if (tb_accept("tron"))
    {
      tracing = true;
    }
    else if (tb_accept("troff"))
    {
      tracing = false;
    }
    else if (tb_accept("cls"))
    {
      ;
    }
    else if (tb_accept("for"))
    {
      forstmt();
    }
    else if (tb_accept("gosub"))
    {
      gosubstmt();
    }
    else if (tb_accept("goto"))
    {
      gotostmt();
    }
    else if (tb_accept("if"))
    {
      ifstmt();
    }
    else if (tb_accept("input"))
    {
      inputstmt();
    }
    else if (tb_accept("next"))
    {
      nextstmt();
    }
    else if (tb_accept("let"))
    {
      assign();
    }
    else if (tb_accept("print") || tb_accept("?"))
    {
      printstmt();
    }
    else if (tb_accept("return"))
    {
      returnstmt();
    }
    else if (tb_accept("@"))
    {
      arrassn();
    }
    else if (toktype == kIDENT)
    {
      assign();
    }
    else if (tok[0] == ':' || tok[0] == '\0')
    {
      /* handled below */
    }
    else
    {
      printf("%d |  ", __LINE__);
      printf("(%d, %d) Unknown token %s: %s\n", curline, textp, tok,
             pgm[curline]);
      errors = true;
    }

    if (errors)
    {
      return;
    }
    if (tok[0] == '\0')
    {
      if (curline == 0 || curline >= c_maxlines)
      {
        showtime(running);
        return;
      }
      initlex(curline + 1);
    }
    else if (tok[0] == ':')
    {
      nexttok();
    }
    else if (need_colon && !expect(":"))
    {
      return;
    }
  }
}

// 時刻を表示
void showtime(bool running)
{
  if (running)
  {
    clock_t tt = clock() - timestart;
#ifdef _WIN64
    printf("Took %.2f seconds\n", (float)(tt) / (float)CLK_TCK);
#else
    printf("Took %.2f seconds\n", (float)(tt) / (float)CLOCKS_PER_SEC);
#endif
  }
}

const char *HELP = "+------------------------------------------------------------------------------+\n"
                   "| bye, exit, clear, cls, end/stop, help, list, load/save, new, run, tron/troff |\n"
                   "| for <var> = <expr1> to <expr2> ... next <var>                                |\n"
                   "| gosub <expr> ... return                                                      |\n"
                   "| goto <expr>                                                                  |\n"
                   "| if <expr> then <statement>                                                   |\n"
                   "| input [prompt,] <var>                                                        |\n"
                   "| <var>=<exp>                                                                  |\n"
                   "| print <expr|string>[, <expr|string>][;]                                      |\n"
                   "| rem <anystring> or ' <anystring>                                             |\n"
                   "| Operators: ^, + / \\\\ mod + - < <= > >= = <>, not, and, or                    |\n"
                   "| Integer variables a..z, and array @(expr)                                    |\n"
                   "| Functions: abs(expr), asc(ch), rnd(expr), sgn(expr)                          |\n"
                   "| Sleep <millisec>                                                             |\n"
                   "+------------------------------------------------------------------------------+";
// HELP
void help(void) { puts(HELP); }

// GOSUB文
void gosubstmt(void)
{
  debugLog("@@@ goto");
  gsp++;
  gstackln[gsp] = curline;
  gstacktp[gsp] = textp;
  gotostmt();
}

// 変数割り当て
void assign(void)
{
  debugLog("@@@ assign");
  int var = getvarindex();
  nexttok();
  expect("=");
  vars[var] = expression(0);
  if (tracing)
  {
    printf("%d |  ", __LINE__);
    printf("*** %c = %d\n", var + 'a', vars[var]);
  }
}

// 配列割り当て
void arrassn(void)
{
  debugLog("@@@ arrassn");
  int atndx = parenexpr();
  if (!tb_accept("="))
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) Array Assign: Expectiong '=', found: %s", curline, textp,
           tok);
    errors = true;
  }
  else
  {
    int n = expression(0);
    atarry[atndx] = n;
    if (tracing)
    {
      printf("%d |  ", __LINE__);
      printf("*** @(%d) = %d\n", atndx, n);
    }
  }
}

// FOR文
// TODO これどうなってる?
void forstmt(void)
{
  debugLog("@@@ for");
  int var, forndx, n;

  var = getvarindex();
  assign();
  // vars(var)has the valude; var has the number valude of the variable in 0..25
  forndx = var;
  forvar[forndx] = vars[var];
  if (!tb_accept("to"))
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) For: Expecting 'to, found: %s\n", curline, textp, tok);
    errors = true;
  }
  else
  {
    n = expression(0);
    forlimit[forndx] = n;
    // need to store iter, limit, line, and col
    forline[forndx] = curline;
    if (tok[0] == '\0')
    {
      forpos[forndx] = textp;
    }
    else
    {
      forpos[forndx] = textp - 2;
    }
  }
}

// IF文
void ifstmt(void)
{
  debugLog("@@@ if");
  need_colon = false;
  if (expression(0) == 0)
  {
    // スルー
    skiptoeol();
  }
  else
  {
    // THENはなくてもよい
    tb_accept("then");
    if (toktype == kNUMBER)
    {
      gotostmt();
    }
    // 行番号じゃなければ、次の解釈で普通に文を実行する。
  }
}

// INPUT文
void inputstmt(void)
{
  debugLog("@@@ input");
  if (toktype == kSTRING)
  {
    printf("%s", &tok[1]);
    nexttok();
    expect(",");
  }
  else
  {
    printf("? ");
  }
  int var = getvarindex();
  nexttok();
  char *st = mygetline(stdin);
  if (!st || st[0] == '\0')
  {
    vars[var] = 0;
  }
  else if (isdigit(st[0]))
  {
    char *endp; // strtolに使うだけ
    vars[var] = strtol(st, &endp, 10);
  }
  else
  {
    vars[var] = st[0]; // 数値として格納
  }
  free(st);
}

// LIST文
void liststmt(void)
{
  debugLog("@@@ list");
  for (int i = 0; i < c_maxlines; ++i)
  {
    if (pgm[i])
    {
      printf("%d %s \n", i, pgm[i]);
    }
    printf("\n");
  }
}

// LOAD文
void loadstmt(void)
{
  debugLog("@@@ load");
  newstmt();

  char *filename;
  if ((filename = getfilename("Load")) == NULL)
  {
    goto load_free;
  }

  FILE *fp = fopen(filename, "r");
  if (fp == NULL)
  {
    printf("%d |  ", __LINE__);
    printf("File %s not found\n", filename);
    goto load_free;
  }

  int n = 0;
  // ライン数でそのままインデックスを引き当てられるように、pgm[0]ではなくpgm[1]から格納している
  while ((pgm[0] = mygetline(fp)) != NULL)
  {
    initlex(0);
    if (toktype == kNUMBER && validlinenum())
    {
      // 行数ありの場合
      pgm[num] = strdup(pgm[0] + textp);
      n = num;
    }
    else
    {
      n++;
      pgm[n] = strdup(pgm[0]);
    }
    free(pgm[0]);
  }
  fclose(fp);

load_free:
  free(filename);
  curline = 0;
}

// NEW文
void newstmt(void)
{
  debugLog("@@@ new");
  clearvars();
  for (int i = 0; i < c_maxlines; ++i)
  {
    if (pgm[i])
    {
      free(pgm[i]);
      pgm[i] = NULL;
    }
  }
}

// NEXT文
// TODO これどうなってる?
void nextstmt(void)
{
  debugLog("@@@ next");
  int forndx = getvarindex();
  forvar[forndx] = forvar[forndx] + 1;
  vars[forndx] = forvar[forndx];
  if (tracing)
  {
    printf("%d |  ", __LINE__);
    printf("*** %c = %d\n", forndx + 'a', vars[forndx]);
  }

  if (forvar[forndx] <= forlimit[forndx])
  {
    curline = forline[forndx];
    textp = forpos[forndx];
    initlex2();
  }

  nexttok();
}

// PRINT文
void printstmt(void)
{
  debugLog("@@@ print");
  bool printnl = true; // 次の行にPRINTする?
  while (strneql(tok, ":") && tok[0] != '\0')
  {
    printnl = true;
    int printwidth = 0;

    // PRINT幅
    if (tb_accept("#"))
    {
      if (num <= 0)
      {
        printf("%d |  ", __LINE__);
        printf("Expecting a print width, found: %s\n", pgm[curline]);
        return;
      }
      printwidth = num;
      nexttok();
      if (!tb_accept(","))
      {
        printf("%d |  ", __LINE__);
        printf("Print: Expecting a ',', found: %s\n", pgm[curline]);
        return;
      }
    }

    if (toktype == kSTRING)
    {
      printf("%*s", printwidth, &tok[1]);
      nexttok();
    }
    else
    {
      printf("%*d", printwidth, expression(0));
    }

    if (tb_accept(",") || tb_accept(";"))
    {
      printnl = false;
    }
    else
    {
      break;
    }
  }
  if (printnl)
  {
    printf("\n");
  }
}

// RETURN文
void returnstmt(void)
{
  debugLog("@@@ return");
  curline = gstackln[gsp];
  textp = gstacktp[gsp];
  --gsp;
  initlex2();
}

// RUN文
void runstmt(void)
{
  debugLog("@@@ run");
  timestart = clock();
  clearvars();
  initlex(1);
}

// GOTO文
void gotostmt(void)
{
  debugLog("@@@ goto");
  num = expression(0);
  ;
  if (validlinenum())
  {
    initlex(num);
  }
}

// SAVE文
void savestmt(void)
{
  debugLog("@@@ save");
  char *filename;
  if ((filename = getfilename("Save")) == NULL)
  {
    goto save_free;
  }

  FILE *fp = fopen(filename, "w");
  if (fp == NULL)
  {
    printf("%d |  ", __LINE__);
    printf("Fiel %s could not be opend for wriring\n", filename);
    goto save_free;
  }

  for (int i = 1; i < c_maxlines; ++i)
  {
    if (pgm[i])
    {
      fprintf(fp, "%d, %s\n", i, pgm[i]);
    }
  }
  fclose(fp);

save_free:
  free(filename);
}

// SLEEP
void sleepstmt(void)
{
  debugLog("@@@ sleep");
  num = expression(0);
  Sleep(num);
}

// ファイル名取得
char *getfilename(char action[])
{
  char *filename;

  if (toktype == kSTRING)
  {
    filename = strdup(&tok[1]);
  }
  else
  {
    printf("%s: ", action);
    filename = mygetline(stdin);
  }
  if (!filename)
  {
    return NULL;
  }
  if (filename[0] == '\0')
  {
    free(filename);
    return NULL;
  }
  if (strchr(filename, '.') == NULL)
  {
    filename = realloc(filename, strlen(filename) + 5);
    strcat(filename, ".bas");
  }
  return filename;
}

// 有効な行番号か?
bool validlinenum(void)
{
  if (num <= 0 || num > c_maxlines)
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) Line number out of range", curline, textp);
    errors = true;
    return false;
  }
  return true;
}

// 変数初期化
void clearvars(void)
{
  for (int i = 0; i < c_maxvars; ++i)
  {
    vars[i] = 0;
  }
  gsp = 0;
}

// 変数のインデックスを取得
int getvarindex(void)
{
  if (toktype != kIDENT)
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) Not a variable: %s\n", curline, textp, thelin);
    errors = true;
    return 0;
  }
  return tok[0] - 'a';
}

// 期待した文字列が来ているか確認
// accept()と同じ理由でfalseを返している
bool expect(const char *s)
{
  if (!tb_accept(s))
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) Expecting %s, buf ound %s, %s", curline, textp, s, tok,
           thelin);
    errors = true;
    return true;
  }
  return false;
}

// 期待したトークンだったら次のトークンを読み進める
// if(!accept(xxx)) { エラー }みたいにするのでOKだったらfalseで返してる
bool tb_accept(const char *s)
{
  if (streql(tok, s))
  {
    nexttok();
    return true;
  }
  return false;
}

// 式を実行する(数値の被演算子、単項演算子、関数、変数)
int expression(int minprec)
{
  int n = 0;

  // handle numeric operands, unary operators, functions, variables
  if (toktype == kNUMBER)
  {
    n = num;
    nexttok();
  }
  else if (tb_accept("-"))
  {
    n = -expression(7);
  }
  else if (tb_accept("+"))
  {
    n = expression(7);
  }
  else if (tb_accept("not"))
  {
    n = !expression(3);
  }
  else if (tb_accept("abs"))
  {
    n = abs(parenexpr());
  }
  else if (tb_accept("asc"))
  {
    expect("(");
    n = tok[1];
    nexttok();
    expect(")");
  }
  else if (tb_accept("rnd") || tb_accept("irnd"))
  {
    n = rnd(parenexpr());
  }
  else if (tb_accept("sgn"))
  {
    n = parenexpr();
    n = (n > 0) - (n < 0);
  }
  else if (toktype == kIDENT)
  {
    n = vars[getvarindex()];
    nexttok();
  }
  else if (tb_accept("@"))
  {
    n = atarry[parenexpr()];
  }
  else if (tok[0] == '(')
  {
    n = parenexpr();
  }
  else
  {
    printf(
        "(%d, %d) Syntax error: expecting an operand, found: %s toktype: %d\n",
        curline, textp, tok, toktype);
    return n;
  }

  for (;;)
  { // while binary operator and precedence of tok >= minprec
    if (minprec <= 1 && tb_accept("or"))
    {
      n = n | expression(2);
    }
    else if (minprec <= 2 && tb_accept("and"))
    {
      n = n & expression(3);
    }
    else if (minprec <= 4 && tb_accept("="))
    {
      n = n == expression(5);
    }
    else if (minprec <= 4 && tb_accept("<"))
    {
      n = n < expression(5);
    }
    else if (minprec <= 4 && tb_accept(">"))
    {
      n = n > expression(5);
    }
    else if (minprec <= 4 && tb_accept("<>"))
    {
      n = n != expression(5);
    }
    else if (minprec <= 4 && tb_accept("<="))
    {
      n = n <= expression(5);
    }
    else if (minprec <= 4 && tb_accept(">="))
    {
      n = n >= expression(5);
    }
    else if (minprec <= 5 && tb_accept("+"))
    {
      n += expression(6);
    }
    else if (minprec <= 5 && tb_accept("-"))
    {
      n -= expression(6);
    }
    else if (minprec <= 6 && tb_accept("*"))
    {
      n *= expression(7);
    }
    else if (minprec <= 6 && tb_accept("/"))
    {
      n /= expression(7);
    }
    else if (minprec <= 6 && tb_accept("\\"))
    {
      n /= expression(7);
    }
    else if (minprec <= 6 && tb_accept("mod"))
    {
      n %= expression(7);
    }
    else if (minprec <= 8 && tb_accept("^"))
    {
      n = pow(n, expression(9));
    }
    else
    {
      break;
    }
  }
  return n;
}
// 括弧の処理
int parenexpr(void)
{
  int n = 0;

  if (!tb_accept("("))
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) Paren Expr: Expectiong '(' found %s\n", curline, textp,
           tok);
  }
  else
  {
    n = expression(0); // 括弧の真ん中
    if (!tb_accept(")"))
    {
      printf("%d |  ", __LINE__);
      printf("(%dm %d) Paren Expr: Expecting ')', found: %s\n", curline, textp,
             tok);
    }
  }
  return n;
}

// 乱数生成
int rnd(int range) { return rand() & range + 1; }

// 読むものがなかったらnullを返す
char *mygetline(FILE *fp)
{
  char *buf, *p;
  int size = BUFSIZ;

  p = buf = malloc(size);
  while (true)
  {
    int c = fgetc(fp);
    if (c == EOF || c == '\n' || c == '\r')
    {
      if (c == EOF && p == buf)
      {
        free(buf);
        return NULL;
      }
      *p = '\0';
      return buf;
    }
    if (p - buf >= size)
    {
      size += BUFSIZ;
      buf = realloc(buf, size);
    }
    *p++ = (char)c;
  }

  return NULL;
}

// 字句の初期化
void initlex(int n)
{
  curline = n;
  textp = 0;
  initlex2();
}

// 字句の初期化2
void initlex2(void)
{
  need_colon = false;
  thelin = pgm[curline];
  thech = ' ';
  nexttok();
}

void nexttok(void)
{
  toktype = kNONE;
  // begin:
  tok[0] = thech;
  getch();
  if (tok[0] == '\0')
  {
    // プログラム行末 なにもしない
  }
  else if (isspace(tok[0]))
  {
    nexttok();
  }
  else if (isalpha(tok[0]))
  {
    readident();
    if (streql(tok, "rem"))
    {
      // 変数の後ろにコメントを書けるようにしている
      skiptoeol();
    }
  }
  else if (isdigit(tok[0]))
  {
    readint();
  }
  else if (tok[0] == '"')
  {
    readstr();
  }
  else if (tok[0] == '\'')
  {
    skiptoeol();
  }
  else if (strchr("#()*+,-/:;<=>?@\\^", tok[0]) != NULL)
  {
    tok[1] = '\0';
    toktype = kPUNCT;
    if ((tok[0] == '<' && (thech == '>' || thech == '=')) ||
        (tok[0] == '>' && thech == '='))
    {
      tok[1] = thech;
      tok[2] = '\0';
      getch();
    }
  }
  else
  {
    printf("%d |  ", __LINE__);
    printf("(%d, %d) What? %c (%d) %s\n", curline, textp, tok[0], tok[0],
           thelin);
    getch();
    errors = true;
  }
}

void skiptoeol(void)
{
  tok[0] = '\0';
  toktype = kNONE;
  textp = strlen(thelin) + 1;
}

// 変数名と区別するためにダブルクォートを文字列の先頭に保持する
void readstr(void)
{
  char *p = &tok[1];
  toktype = kSTRING;
  while (thech != '"')
  {
    if (thech == '\0')
    {
      printf("%d |  ", __LINE__);
      printf("(%d, %d) String not terminated\n", curline, textp);
      errors = true;
      return;
    }
    *p++ = thech;
    getch();
  }
  *p = '\0';
  getch();
}

void readident(void)
{
  char *p = &tok[1];
  tok[0] = tolower(tok[0]);
  toktype = kIDENT;
  while (isalnum(thech))
  {
    *p++ = tolower((char)thech);
    getch();
  }
  *p = '\0';
}

void readint(void)
{
  char *p = &tok[1];
  char *endp;
  toktype = kNUMBER;
  while (isdigit(thech))
  {
    *p++ = thech;
    getch();
  }
  *p = '\0';
  num = strtol(tok, &endp, 10);
}

void getch(void)
{
  if (!thelin)
  {
    thech = '\0';
  }
  else
  {
    thech = thelin[textp];
    if (thech != '\0')
    {
      ++textp;
    }
  }
}

void debugLog(const char *msg)
{
  if (tracing)
  {
    puts(msg);
  }
}
