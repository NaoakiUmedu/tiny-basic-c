```mermaid
---
title: main program
---
sequenceDiagram
    alt: argc > 1
        note over main: プログラムをとりこみRUNコマンドを実行
    else
        main ->> newstmt: newstmt()
        main ->> help: help()
    end

    loop true
        main ->> mygetline: mygetline()
        mygetline -->> main: 入力
        opt pgm[0] が空白ではない
            main ->> initlex: initlex()
            alt toktype == kNUMBER
                note over main: 行数の位置にプログラムを記録
            else
                main ->> docmd: docmd()
            end
        end
    end
```

```mermaid
---
titile initlex
---
sequenceDiagram
    participant initlex
    participant initlex2

    note over initlex: curline = n / textp = 0
    initlex ->> initlex2: initlex2()
    note over initlex2: need_colon = fales / thelin = pgm / [curline] / thech = ' '
    initlex2 ->> nexttok: nexttok()
```

```mermaid
---
title: nexttok
---
sequenceDiagram
    note over nexttok: toktype = kNONE / tok[0] = thech
    nexttok ->> getch: getch()
    alt tok[0] = '\0'
        note over nexttok: 何もしない
    else : isspace(tok[0])
        nexttok ->> nexttok: nexttok()
    else : isalpha(tok[0])
        nexttok ->> readident: readident()
        opt : streql(tok, "rem")
            nexttok ->> skiptoeol: skiptoeol()
        end
    else : isdigit(tok[0])
        nexttok ->> readint: readint()
    else : tok[0] == '"'
        nexttok ->> readstr: readstr()
    else : strchr("#()*+,-/:;<=>?@\\^", tok[0]) != NULL
        note over nexttok : tok[1] = '\0' / toktype = kPUNCT
        opt (tok[0] == '<' && (thech == '>' || thech == '=')) ||  (tok[0] == '>' && thech == '=')
            note over nexttok: tok[1] = thech tok[2] = '\0
            nexttok ->> getch: getch()
        end
    else
        nexttok ->> getch: getch()
        note over nexttok: errors = true
    end
```

* getch => thechとtextpを更新する
* readxxx() => tokとtoktypeを設定
* skiptoeol() => 終端までスキップ(toktype=kNONE)

```mermaid
---
title: docmd()
---
sequenceDiagram
    participant docmd
    note over docmd: need_colon = true

    alt : accept("bye") || accept("quit") || accept("exit")
        note over docmd: exit(0)
    else : accept("end") || accept("stp")
        docmd ->> showtime: showtime()
        note over docmd: return()
    else : accept("clear")
        docmd ->> clearvars : clearvars()
        note over docmd: return()
    else : accept("help")
        docmd ->> help : help()
        note over docmd: return()
    else : accept("list")
        docmd ->> liststmt : liststmt()
        note over docmd: return()
    else : accept("load")
        docmd ->> loadstmt : loadstmt()
        note over docmd: return()
    else : accept("run")
        docmd ->> runstmt : runstmt()
        note over docmd: running = true / need_colon = false
    else : accept("save")
        docmd ->> savestmt : savestmt()
        note over docmd: return()
    else : accept("tron")
        note over docmd: tracing = true
    else : accept("troff")
        note over docmd: tracing = false
    else : accept("cls")
        note over docmd: TODO 実装する
    else : accept("gosub")
        docmd ->> gosubstmt: gosubstmt()
    else : accept("goto")
        docmd ->> gotostmt: gotostmt()
    else : accept("if")
        docmd ->> ifstmt: ifstmt()
    else : accept("input")
        docmd ->> inputstmt: inputstmt()
    else : accept("next")
        docmd ->> nextstmt: nextstmt()
    else : accept("let")
        docmd ->> assign: assign
    else : accept("print") || accept("?")
        docmd ->> printstmt: printstmt()
    else : accept("return")
        docmd ->> returnstmt : returnstmt()
    else : accept("@")
        docmd ->> arassn : arassn()
    else : toktype == kIDENT
        docmd ->> assign: assign()
    else : tok[0] == ':' || tok[0] == '\0'
        note over docmd: あとで処理する(elseでエラーキャッチされちゃわない用の分岐
    else
        note over docmd: errors = true
    end
    
    opt errors
        note over docmd: return()
    end

    alt : tok[0] == '\0'
        opt : curline == 0 || curline >= c_maxlines
            docmd ->> showtime: showtime()
            note over docmd: return()
        end
        docmd ->> initlex: initlex()
    else : tok[0] == ':'
        docmd ->> nexttok: nexttok()
    else : need_colon && !expect(":")
        note over docmd: return()
    end
```

* accept => 期待した文字が来ていればnexttok()
* showtime => 時刻を表示
* help => help
* clearvars => 変数を初期化
* liststmt => プログラム全文を表示
