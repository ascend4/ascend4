;;;  ascend-mode.el, a mode for editing ASCEND code in emacs
;;;
;;;  This file is part of the Ascend modeling library.
;;;
;;;  Copyright (C) 1994,1995,1996,1997,1998
;;;
;;;  Carnegie Mellon University
;;;
;;;  The Ascend modeling library is free software; you can redistribute it
;;;  and/or modify it under the terms of the GNU General Public License as
;;;  published by the Free Software Foundation; either version 2 of the
;;;  License, or (at your option) any later version.
;;;
;;;  The Ascend Language Interpreter is distributed in hope that it will
;;;  be useful, but WITHOUT ANY WARRANTY; without even the implied
;;;  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
;;;  the GNU General Public License for more details.
;;;
;;;  You should have received a copy of the GNU General Public License
;;;  along with the program; if not, write to the Free Software
;;;  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139 USA.  Check the
;;;  file named COPYING.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;;  1994 Aug 26 Mark Thomas <mthomas+@cmu.edu>
;;;              - Initial version created in Lucid Emacs 19.10
;;;
;;;  1995 Sep 13 Mark Thomas <mthomas+@cmu.edu>
;;;              - Updated for use with XEmacs 19.12
;;;              - Added functions to support Gnu Emacs Menus
;;;
;;;  1996 May 29 Mark Thomas <mthomas+@cmu.edu>
;;;              - Changing keywords:
;;;                * INITIALIZATION --> METHODS
;;;                * PROCEDURE  -->  METHOD
;;;
;;;  1997 Nov    Mark Thomas <mthomas+@cmu.edu>
;;;              - Major rewrite to make ascend-mode compatible with the
;;;                latest releases of
;;;                * ASCEND IV (0.8)
;;;                * XEmacs (19.16)
;;;                * FSF Emacs (19.34)
;;;              - Most of the code was made more general to be easily
;;;                expandable as ASCEND IV grows
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;;  Make sure we get the 'cl package
;;;
(require 'cl)
;;;
;;;  User Friendly Constants  -------------------------------------------------
;;;

(defvar ascend-block-indent-level 4
  "*Indentation of ASCEND statements in a block with respect to the
statement that starts the block.")

(defvar ascend-continuation-indent-level 4
  "*Indentation of ASCEND statement continuations with respect to
statement start.  CURRENTLY NOT SUPPORTED.")

(defvar ascend-auto-newline nil
  "*If nonnil, a newline is automatically inserted when semicolon(;)
is entered.  Implies ascend-semicolon-auto-indent.")

(defvar ascend-semicolon-auto-indent t
  "*If nonnil, entering a semicolon(;) will always indent the current
line.")

(defvar ascend-auto-add-end-statement t
  "*If nonnil, pressing RETURN \(or semicolon if ascend-auto-newline
is nonnil\) on a line that starts a block will cause the matching end
statement to be automatically added to the buffer.")

(defvar ascend-tab-always-indent nil
  "*If nonnil, pressing TAB will always indent the current line;
otherwise, TAB will only indent if in the left margin.")

(defvar ascend-expand-abbrevs-in-comments nil
  "If nil, abbrevs are not expanded in comments, strings, and
notes.")

(defvar ascend-mode-hook nil
  "*Mode hook for ASCEND mode buffers.")

;;;
;;;  Internal Variables  ------------------------------------------------------
;;;

(defvar ascend-abbrev-table nil
  "Abbrev table for use in ascend-mode buffers")

(defvar ascend-mode-map nil
  "Keymap for use in ascend-mode buffers")

(defvar ascend-mode-syntax-table nil
  "Syntax table for use in ascend-mode buffers")

(defvar ascend-font-lock-keywords nil
  "ASCEND keywords for font-lock mode.
See the documentation for font-lock-keywords.")

(defvar ascend-menu nil
  "A menu for ASCEND mode buffers.")

(defconst ascend-mode-version "1.14"
  "Version number for this release of ASCEND mode.")

;;;
;;;  Do the actual dirty deed  ------------------------------------------------
;;;

;;;  ABBREV TABLE
;;;    abbrev-list is a list of sub-lists; the car of each sublist is
;;;    the expansion text; the cdr are the strings that expand into
;;;    that text.
(if (not ascend-abbrev-table)
    (let ((abbrev-list '(("ADD" "add")
                         ("ALIASES" "aliases" "alii" "al")
                         ("AND" "and")
                         ("ANY" "any")
                         ("ARE_ALIKE" "are_alike" "arealike" "aa")
                         ("ARE_THE_SAME" "are_the_same" "arethesame" "ats")
                         ("ARRAY" "array")
                         ("ATOM" "atom")
                         ("BREAK" "break")
                         ("CALL" "call")
                         ("CARD" "card")
                         ("CASE" "case")
                         ("CHOICE" "choice")
                         ("CONDITIONAL" "conditional")
                         ("CONSTANT" "constant" "const")
                         ("CONTINUE" "continue")
                         ("CREATE" "create")
                         ("DATA" "data")
                         ("DECREASING" "descreasing")
                         ("DEFAULT" "default")
                         ("DEFINITION" "definition")
                         ("DIMENSION" "dimension")
                         ("DIMENSIONLESS" "dimensionless")
                         ("DO" "do")
                         ("ELSE" "else")
                         ("END" "end")
                         ("EXTERNAL" "external")
                         ("FALSE" "false")
                         ("FALL_THROUGH" "fall_through" "fallthrough" "fall")
                         ("FOR" "for")
                         ("FROM" "from")
                         ("GLOBAL" "global")
                         ("IF" "if")
                         ("IMPORT" "import")
                         ("IN" "in")
                         ("INCREASING" "increasing")
                         ("INPUT" "input")
                         ("INTERACTIVE" "interactive")
                         ("INTERSECTION" "intersection")
                         ("IS_A" "is_a" "isa")
                         ("IS_REFINED_TO" "is_refined_to" "isrefinedto" "irt")
                         ("MAXIMIZE" "maximize" "max")
                         ("MAX_INTEGER" "max_integer" "maxinteger" "maxint")
                         ("MAX_REAL" "max_real" "maxreal")
                         ("METHOD" "method")
                         ("METHODS" "methods")
                         ("MINIMIZE" "minimize" "min")
                         ("MODEL" "model")
                         ("NOT" "not")
                         ("NOTES" "notes")
                         ("OF" "of")
                         ("OR" "or")
                         ("OTHERWISE" "otherwise")
                         ("OUTPUT" "output")
                         ("PROD" "prod")
                         ("PROVIDE" "provide")
                         ("REFINES" "refines")
                         ("REPLACE" "replace")
                         ("REQUIRE" "require")
                         ("RETURN" "return")
                         ("RUN" "run")
                         ("SATISFIED" "satisfied")
                         ("SELECT" "select")
                         ("SIZE" "size")
                         ("STOP" "stop")
                         ("SUCH_THAT" "such_that" "suchthat")
                         ("SUM" "sum")
                         ("SWITCH" "switch")
                         ("THEN" "then")
                         ("TRUE" "true")
                         ("UNION" "union")
                         ("UNITS" "units")
                         ("UNIVERSAL" "universal")
                         ("USE" "use")
                         ("VALUE" "value")
                         ("WHEN" "when")
                         ("WHERE" "where")
                         ("WHILE" "while")
                         ("WILL_BE" "will_be" "willbe" "wb")
                         ("WILL_BE_THE_SAME" "willbethesame" "wbts")
                         ("WILL_NOT_BE_THE_SAME" "willnotbethesame" "wnbts")
                         ("WITH" "with")
                         ("WITH_VALUE" "with_value" "withvalue" "wv"))
                       ))
      (define-abbrev-table 'ascend-abbrev-table ())
      (mapcar '(lambda (abbrev)
                 (let ((expansion (car abbrev)))
                   (mapcar '(lambda (name)
                              (define-abbrev ascend-abbrev-table
                                             name
                                             expansion
                                             'ascend-undo-abbrev-in-comment))
                           (cdr abbrev))))
              abbrev-list)))



;;;  MODE MAP
;;;    The ascend-mode-map is minimal.
(if (not ascend-mode-map)
    (progn
      (setq ascend-mode-map (make-sparse-keymap))
      (define-key ascend-mode-map "\t"        'electric-ascend-tab)
      (define-key ascend-mode-map "\C-m"      'electric-ascend-newline-indent)
      (define-key ascend-mode-map "\C-j"      'electric-ascend-newline)
      (define-key ascend-mode-map ";"         'electric-ascend-semicolon)
      (define-key ascend-mode-map "\M-\C-b"   'ascend-backward-block)
      (define-key ascend-mode-map "\M-\C-f"   'ascend-forward-block)
      (define-key ascend-mode-map "\M-\C-a"   'ascend-beginning-of-block)
      (define-key ascend-mode-map "\M-\C-e"   'ascend-end-of-block)
      (define-key ascend-mode-map "\M-\C-h"   'ascend-mark-block)
      ))



;;;  SYNTAX TABLE
;;;    For the syntax table, we have to make the math characters into
;;;    punctuation and define the comment characters.  We make braces
;;;    into matching string delimiters when running under XEmacs
(if (not ascend-mode-syntax-table)
    (progn
      (setq ascend-mode-syntax-table (make-syntax-table))
      ;; the following are the same as the (standard-syntax-table)
      ;;(modify-syntax-entry ?\\ "\\"   ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\" "\""   ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\t " "    ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\n " "    ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\r " "    ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\f " "    ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\v " "    ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\[ "(]"   ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\] ")["   ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\{ "<}"   ascend-mode-syntax-table)
      ;;(modify-syntax-entry ?\} ">{"   ascend-mode-syntax-table)
      ;;
      ;; the following differ from the (standard-syntax-table)
      (modify-syntax-entry ?\( "()1"  ascend-mode-syntax-table)
      (modify-syntax-entry ?\) ")(4"  ascend-mode-syntax-table)
      (modify-syntax-entry ?*  ". 23" ascend-mode-syntax-table)
      (modify-syntax-entry ?+  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?-  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?=  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?%  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?\/ "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?^  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?<  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?>  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?&  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?|  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?.  "."    ascend-mode-syntax-table)
      (modify-syntax-entry ?,  "."    ascend-mode-syntax-table)
      ;; treat _ as a word for abbrev mode and for search strings.  If
      ;; _ is not a word char, then when sitting on the b in foo_bar
      ;; (looking-at "\\<") returns t
      (modify-syntax-entry ?_  "w"    ascend-mode-syntax-table)
      (modify-syntax-entry ?'  "\""   ascend-mode-syntax-table)
      ;; treat braces as matching string delimters in XEmacs
      (if (string-match "XEmacs" emacs-version)
          (progn
            (modify-syntax-entry ?\{ "\"}"  ascend-mode-syntax-table)
            (modify-syntax-entry ?\} "\"{"  ascend-mode-syntax-table)))
      ))


;;;  FONT LOCK
;;;    The font lock stuff looks a lot worse than it is...
(if ascend-font-lock-keywords
    ()
  (let* ((keywords '("ADD"
                     "ALIASES"
                     "AND"
                     "ANY"
                     "ARE_ALIKE"
                     "ARE_THE_SAME"
                     "ARRAY"
                     "ATOM"
                     "BREAK"
                     "CALL"
                     "CARD"
                     "CASE"
                     "CHOICE"
                     "CONDITIONAL"
                     "CONSTANT"
                     "CONTINUE"
                     "CREATE"
                     "DATA"
                     "DECREASING"
                     "DEFAULT"
                     "DEFINITION"
                     "DIMENSION"
                     "DIMENSIONLESS"
                     "DO"
                     "ELSE"
                     "END"
                     "EXTERNAL"
                     "FALSE"
                     "FALL_THROUGH"
                     "FOR"
                     "FROM"
                     "GLOBAL"
                     "IF"
                     "IMPORT"
                     "IN"
                     "INCREASING"
                     "INPUT"
                     "INTERACTIVE"
                     "INTERSECTION"
                     "IS_A"
                     "IS_REFINED_TO"
                     "MAXIMIZE"
                     "MAX_INTEGER"
                     "MAX_REAL"
                     "METHOD"
                     "METHODS"
                     "MINIMIZE"
                     "MODEL"
                     "NOT"
                     "NOTES"
                     "OF"
                     "OR"
                     "OTHERWISE"
                     "OUTPUT"
                     "PROD"
                     "PROVIDE"
                     "REFINES"
                     "REPLACE"
                     "REQUIRE"
                     "RETURN"
                     "RUN"
                     "SATISFIED"
                     "SELECT"
                     "SIZE"
                     "STOP"
                     "SUCH_THAT"
                     "SUM"
                     "SWITCH"
                     "THEN"
                     "TRUE"
                     "UNION"
                     "UNITS"
                     "UNIVERSAL"
                     "USE"
                     "VALUE"
                     "WHEN"
                     "WHERE"
                     "WHILE"
                     "WILL_BE"
                     "WILL_BE_THE_SAME"
                     "WILL_NOT_BE_THE_SAME"
                     "WITH"
                     "WITH_VALUE"))
         (keyword-regexp
          (if (fboundp 'make-regexp)
              (concat "\\<" (make-regexp keywords t) "\\>")
            (concat "\\<\\(A\\(DD\\|LIASES\\|N[DY]\\|R\\(E_\\(ALIKE\\|"
                    "THE_SAME\\)\\|RAY\\)\\|TOM\\)\\|BREAK\\|C\\(A\\(LL"
                    "\\|RD\\|SE\\)\\|HOICE\\|ON\\(DITIONAL\\|STANT\\|"
                    "TINUE\\)\\|REATE\\)\\|D\\(ATA\\|E\\(CREASING\\|F"
                    "\\(AULT\\|INITION\\)\\)\\|IMENSION\\(\\|LESS\\)\\|O"
                    "\\)\\|E\\(LSE\\|ND\\|XTERNAL\\)\\|F\\(AL\\(L_THROUGH"
                    "\\|SE\\)\\|OR\\|ROM\\)\\|GLOBAL\\|I\\([FN]\\|MPORT"
                    "\\|N\\(CREASING\\|PUT\\|TER\\(ACTIVE\\|SECTION\\)\\)"
                    "\\|S_\\(A\\|REFINED_TO\\)\\)\\|M\\(AX\\(IMIZE\\|_"
                    "\\(INTEGER\\|REAL\\)\\)\\|ETHODS?\\|INIMIZE\\|ODEL"
                    "\\)\\|NOT\\(\\|ES\\)\\|O\\([FR]\\|THERWISE\\|UTPUT\\)"
                    "\\|PRO\\(D\\|VIDE\\)\\|R\\(E\\(FINES\\|PLACE\\|QUIRE"
                    "\\|TURN\\)\\|UN\\)\\|S\\(ATISFIED\\|ELECT\\|IZE\\|TOP"
                    "\\|U\\(CH_THAT\\|M\\)\\|WITCH\\)\\|T\\(HEN\\|RUE\\)"
                    "\\|U\\(NI\\(ON\\|TS\\|VERSAL\\)\\|SE\\)\\|VALUE\\|W"
                    "\\(H\\(E\\(N\\|RE\\)\\|ILE\\)\\|I\\(LL_\\(BE\\(\\|"
                    "_THE_SAME\\)\\|NOT_BE_THE_SAME\\)\\|TH\\(\\|_VALUE"
                    "\\)\\)\\)\\)\\>")))
	 (method-regexp "\\<METHOD[ \t]+\\(\w+\\)")
	 (type-regexp
	  (concat "\\<\\(CONSTANT\\|DEFINITION\\|ATOM\\|MODEL\\)[ \t]+"
		  "\\(\w+\\)"))
         )
    (setq ascend-font-lock-keywords
	  (purecopy
	   (list keyword-regexp
		 (list method-regexp 1 'font-lock-function-name-face)
		 (list type-regexp 2 'font-lock-type-face)
                 )))))

(put 'ascend-mode 'font-lock-defaults '(ascend-font-lock-keywords))


;;;  MENU
;;;    The only tricky thing about the menu is using either
;;;    zmacs-regions in XEmacs or mark-active in FSF Emacs
(if ascend-menu
    ()
  (setq ascend-menu
        (list
         '["Goto Block Start"
           (ascend-beginning-of-block)
           t]
         '["Mark Current Block"
           (ascend-mark-block 1)
           t]
         "---"
         (vector
          "Comment Out Region"
          'comment-region
          (if (boundp 'mark-active)
              '(identity mark-active)
            '(or (not zmacs-regions) (mark))))
         (vector
          "Indent Region"
          'indent-region
          (if (boundp 'mark-active)
              '(identity mark-active)
            '(or (not zmacs-regions) (mark))))
         '["Indent Line"
           ascend-indent-line
           t]
         "---"
         '["Auto Newline On ;"
           (setq ascend-auto-newline (not ascend-auto-newline))
           :style toggle
           :selected ascend-auto-newline]
         '["Auto Indent On ;"
           (setq ascend-semicolon-auto-indent
                 (not ascend-semicolon-auto-indent))
           :active (not ascend-auto-newline)
           :style toggle
           :selected ascend-semicolon-auto-indent]
         '["Auto Add Matching END"
           (setq ascend-auto-add-end-statement
                 (not ascend-auto-add-end-statement))
           :style toggle
           :selected ascend-auto-add-end-statement]
         '["Always Indent On TAB"
           (setq ascend-tab-always-indent (not ascend-tab-always-indent))
           :style toggle
           :selected ascend-tab-always-indent]
         '["Auto Indent On RETURN"
           ascend-toggle-newline-linefeed
           :style toggle
           :selected (eq (key-binding "\C-m") 'electric-ascend-newline-indent)]
         '["Auto Expand Abbreviations"
           (setq abbrev-mode (not abbrev-mode))
           :style toggle
           :selected abbrev-mode]
         '["Expand Abbrevs in Comments"
           (setq ascend-expand-abbrevs-in-comments
                 (not ascend-expand-abbrevs-in-comments))
           :active abbrev-mode
           :style toggle
           :selected ascend-expand-abbrevs-in-comments]
         '"---"
         '["Describe ASCEND mode"
           describe-mode
           t]
         )))


;;;  ASCEND MODE
;;;###autoload
(defun ascend-mode ()
  "Major mode for editing ASCEND Code.
TAB indents for ASCEND code.
DELETE converts tabs to spaces as it moves back.
\\{ascend-mode-map}
Variables controlling indentation style:
    ascend-auto-newline (default nil)
      If nonnil, a newline is automatically inserted when
      semicolon(;) is entered.  Implies ascend-semicolon-auto-indent.
    ascend-semicolon-auto-indent (default t)
      If nonnil, entering a semicolon(;) will always indent
      the current line.
    ascend-tab-always-indent (default nil)
      If nonnil, pressing TAB will always indent the current line;
      otherwise, TAB will only indent if in the left margin.
    ascend-block-indent-level (default 4)
      Indentation of ASCEND statements in an block with
      respect to the statement that starts the block.
    ascend-continuation-indent-level (default 4)
      Indentation of ASCEND statement continuations with respect
      to statement start.

Comments delimited by (* .. *).  The statement separator is the semicolon.

Turning on ASCEND-mode calls the value of the variable ascend-mode-hook
with no args, if that value is non-nil."
  (interactive)
  ;;
  (kill-all-local-variables)
  (use-local-map ascend-mode-map)
  (setq major-mode 'ascend-mode)
  (setq mode-name "ASCEND")
  (setq local-abbrev-table ascend-abbrev-table)
  (set-syntax-table ascend-mode-syntax-table)
  ;;
  (make-local-variable 'indent-line-function)
  (setq indent-line-function 'ascend-indent-line)
  ;;
  (make-local-variable 'comment-start)
  (setq comment-start "(*")
  (make-local-variable 'comment-end)
  (setq comment-end "*)")
  (make-local-variable 'ascend-comment-start)
  (setq ascend-comment-start (regexp-quote comment-start))
  (make-local-variable 'ascend-comment-end)
  (setq ascend-comment-end (regexp-quote comment-end))
  (make-local-variable 'ascend-comment-start-end)
  (setq ascend-comment-start-end (concat ascend-comment-start "\\|"
                                         ascend-comment-end))
  ;;
  (make-local-variable 'ascend-notes-start)
  (setq ascend-notes-start "{")
  (make-local-variable 'ascend-notes-end)
  (setq ascend-notes-end "}")
  ;;
  (make-local-variable 'ascend-block-start-regexp-list)
  (setq ascend-block-start-regexp-list '("FOR"
					 "METHOD"
					 "\\(UNIVERSAL[ \t]+\\)?MODEL"
					 "\\(UNIVERSAL[ \t]+\\)?ATOM"
					 "IF"
					 "NOTES"
					 "SELECT"
					 "WHEN"
					 "SWITCH"
					 "WHILE"
					 "DEFINITION"
					 "UNITS"
					 "CONDITIONAL"
					 "DATA"
					 ))
  (make-local-variable 'ascend-block-start-regexp)
  (setq ascend-block-start-regexp
	(concat "\\<\\("
		(mapconcat 'identity ascend-block-start-regexp-list "\\|") 
		"\\)\\>"))
  (make-local-variable 'ascend-block-end-regexp-list)
  (setq ascend-block-end-regexp-list '("END"))
  (make-local-variable 'ascend-block-end-regexp)
  (setq ascend-block-end-regexp
	(concat "\\<\\("
		(mapconcat 'identity ascend-block-end-regexp-list "\\|")
		"\\)\\>"))
  (make-local-variable 'ascend-outdent-regexp-list)
  (setq ascend-outdent-regexp-list '("\\<CASE\\>"
                                     "\\<OTHERWISE\\>"
                                     ")[ \t]*REFINES\\>"
				     "\\(\)[ \t]*\\)?WHERE[ \t\n]*\("
				     "\\<ELSE\\>"))
  (make-local-variable 'ascend-outdent-regexp)
  (setq ascend-outdent-regexp
	(concat "\\("
		(mapconcat 'identity ascend-outdent-regexp-list "\\|")
		"\\)"))
  (make-local-variable 'ascend-no-semi-regexp-list)
  (setq ascend-no-semi-regexp-list '("METHODS"))
  (make-local-variable 'ascend-no-semi-regexp)
  (setq ascend-no-semi-regexp 
	(concat "\\<\\("
		(mapconcat 'identity ascend-no-semi-regexp-list "\\|")
		"\\)\\>"))
  ;;
  (ascend-create-menu)
  (if (null (string-match "XEmacs" emacs-version))
      (progn
	;; Do the font magic for FSF Emacs
	(make-local-variable 'font-lock-keywords)
	(setq font-lock-keywords
	      (append (list (list (car ascend-font-lock-keywords)
				  '(0 font-lock-keyword-face)))
		      (mapcar '(lambda (x) (list (car x) (cdr x)))
			      (cdr ascend-font-lock-keywords))))))
  (run-hooks 'ascend-mode-hook))


;;;
;;;  Electric functions  ------------------------------------------------------
;;;

(defun electric-ascend-newline (count)
  "Insert COUNT newlines."
  (interactive "P")
  (delete-horizontal-space)
  (ascend-indent-line)
  (if ascend-auto-add-end-statement
      (ascend-add-matching-end-block))
  (newline (prefix-numeric-value count)))


(defun electric-ascend-newline-indent (count)
  "Insert COUNT newlines then indent final line."
  (interactive "P")
  (electric-ascend-newline count)
  (indent-to (ascend-calculate-indentation)))


(defun electric-ascend-tab (count)
  "Called when TAB is pressed.  If COUNT is specified,
insert COUNT tabs; if ascend-tab-always-indent is t, indent line;
otherwise, only indent if before first character on line."
  (interactive "P")
  (cond (count
	 (self-insert-command (prefix-numeric-value count)))
	(ascend-tab-always-indent
	 (ascend-indent-line))
	((ascend-point-in-left-margin-p)
	 (ascend-indent-line))
	((ascend-point-in-string-p)
	 ())
	(t
	 (self-insert-command 1))))


(defun electric-ascend-semicolon (count)
  "Called when semicolon(;) is pressed.  If COUNT is
specified, insert COUNT semicolons; otherwise insert a semicolon and
correct line's indentation.  If ascend-auto-newline is t, insert newline."
  (interactive "P")
  (cond (count
	 (self-insert-command (prefix-numeric-value count)))
	((ascend-point-in-comment-p)
	 (self-insert-command 1))
	((ascend-point-in-string-p)
	 ())
	(ascend-auto-newline
	 (self-insert-command 1)
	 (ascend-indent-line)
         (if ascend-auto-add-end-statement
             (ascend-add-matching-end-block))
	 (newline-and-indent))
	(ascend-semicolon-auto-indent
	 (self-insert-command 1)
	 (ascend-indent-line))
	(t
	 (self-insert-command 1))))


;;;
;;;  Interactive functions  ---------------------------------------------------
;;;


(defun ascend-toggle-newline-linefeed (arg)
  "Toggle the meanings of newline (C-m) and linefeed (C-j).
By default in ASCEND-mode, NEWLINE runs 'electric-ascend-newline-indent
---which inserts a newline and indents---and LINEFEED calls
'electric-ascend-newline---which inserts a newline but does not indent).
Calling this function without an argument toggles the meanings; if ARG is
specified and is positive, newline is set to 'electric-ascend-newline;
otherwise newline is set to 'electric-ascend-newline-indent."
  (interactive "P")
  (if (or (and (not arg)
	       (eq (key-binding "\C-m") 'electric-ascend-newline-indent))
	  (< 0 (prefix-numeric-value arg)))
      (progn
	(define-key ascend-mode-map "\C-m" 'electric-ascend-newline)
	(define-key ascend-mode-map "\C-j" 'electric-ascend-newline-indent))
    (define-key ascend-mode-map "\C-m" 'electric-ascend-newline-indent)
    (define-key ascend-mode-map "\C-j" 'electric-ascend-newline)))


;;;
;;;  Indenting functions (interactive)  ---------------------------------------
;;;


(defun ascend-calculate-indentation ()
  "Calculate the indentation of the current ASCEND line without
modifying the buffer."
  (let ((case-fold-search nil)
        tmp
	;; tmp is the point where the current comment starts--when we
	;; are in a comment; if we are not in a comment, it is nil
	)
    (save-excursion
      ;; The next two lines put us on the first nonwhitespace character on
      ;; the line---if we've been called from indent-line, then this
      ;; should have already been done.
      (beginning-of-line)
      (skip-chars-forward " \t")
      (cond (;; handle comments
             (setq tmp (ascend-point-in-comment-p))
	     (let (;; count-stars is the number of asterisks that start the
		   ;; current line--ignoring any leading whitespace
		   (count-stars (if (looking-at "\\*+")
				    (apply '- (match-data))
				  0)))
	       ;; The next line searches for the previous nonblank line
	       ;; within the current comment; if there isn't one, it
	       ;; leaves point on the comment start character
	       (re-search-backward "^[ \t]*\\S-" tmp 1)
	       (if (looking-at ascend-comment-start)
		   (progn
		     ;; There was no nonblank line--we are current looking
		     ;; at "(" followed be one or more "*"s.  Set the
		     ;; indentation to the column after the last asterisk,
		     ;; except any asterisks that start the line we are
		     ;; indenting should fall under asterisks in the
		     ;; previous line.  For example
		     ;;     (******
		     ;;          ** foo
		     (forward-char 1)              ;; skip the "("
		     (skip-chars-forward "\\*")    ;; skip the "*"s
		     (max 0 (+ count-stars (current-column))))
		 (progn
		   ;; We found a nonblank line.  Set the indentation to
		   ;; the column containing the first nonblank character.
		   ;; Ignore asterisks in the previous and current lines.
		   (skip-syntax-forward " ")
		   (current-column)))))
	    (;; handle notes
             (ascend-point-in-note-p)
	     (current-column))
	    (;; handle the end of a block
             (looking-at ascend-block-end-regexp)
	     (if (setq tmp (ascend-get-matching-block-start))
		 (goto-char tmp))
	     (current-column))
	    (;; handle outdented keywords like ELSE or CASE
             (remove-if-not 'looking-at ascend-outdent-regexp-list)
	     (if (setq tmp (ascend-pos-beginning-of-block 1))
		 (goto-char tmp))
	     (current-column))
	    (;; Lines that begin with a close-paren should indent to the
	     ;; column of the first nonblank character on the line
	     ;; containing the matching open-paren.
	     (looking-at ")")
	     (forward-char 1)
	     (backward-sexp)
	     (beginning-of-line)
	     (skip-syntax-forward " ")
	     (current-column))
	    (;; This statement puts us on the previous nonblank line.  If
	     ;; this branch fires, then no previous nonblank line exists,
	     ;; and we should set the indentation to 0.
	     (null (re-search-backward "^[ \t]*\\S-" nil t))
	     0)
	    (;; Going backward may have put us in a comment; if so, go to
	     ;; the beginning of the comment and start the process all
	     ;; over again.
	     (setq tmp (ascend-point-in-comment-p))
	     (goto-char tmp)
	     (ascend-calculate-indentation))
	    (;; This statement puts us on the first nonblank character on
	     ;; the previous nonblank line.  We then check to see if it is
	     ;; a block-start statement; if so, we need to indent by the
	     ;; block-indent-level.
	     (and (progn
		    (beginning-of-line)
		    (skip-chars-forward " \t"))
		  (looking-at ascend-block-start-regexp))
	     (+ (current-column) ascend-block-indent-level))
            (;; check if it is an outdented statement
             (looking-at ascend-outdent-regexp)
	     (+ (current-column) ascend-block-indent-level))
	    (;; just return the current column
             t
	     (current-column))
	    ;; The following will not get invoked because of the 
	    ;; t on the previous condition.  We need to do something
	    ;; here to get continuation lines to work.
            ;;;;;;;;;;;;;;;;;;;;;;;;;;
            ((looking-at ascend-comment-start)
             ;; Code should line up with comments
             (looking-at ascend-comment-start)
             (current-column))
	    (;; This line does not require a semicolon, so it should be
	     ;; considered complete, and the next line should NOT be
	     ;; considered a continuation line.
	     (remove-if-not 'looking-at ascend-no-semi-regexp-list)
	     (current-column))
	    ((save-excursion
	       (end-of-line)
	       (if (setq tmp (ascend-point-in-comment-p))
		   (goto-char tmp))
	       (skip-syntax-backward " ")
	       (forward-char -1)
	       (looking-at ";"))
	     ;; This line ends in a semicolon, so the next line should NOT
	     ;; be considered a continuation line.
	     (current-column))
	    (t
	     ;; Consider the line to be a continuation line
	     (+ (current-column) ascend-continuation-indent-level)))
      )))


(defun ascend-indent-line ()
  "Indent the current line relative to the current block."
  (interactive)
  (let ((m (point-marker)))
    (beginning-of-line)
    (if (ascend-point-in-note-p)
	nil
      (delete-horizontal-space)
      (indent-to (ascend-calculate-indentation)))
    (if (> m (point))
	(goto-char m))))


;;;
;;;  Marking functions (interactive)  -----------------------------------------
;;;


(defun ascend-mark-block (count)
  "If point is inside a block, mark the current block by putting mark
at the beginning and point at the end.  If point is outside a block,
mark the first complete block we find \(designated by the first END
statement\).  With argument COUNT, mark COUNT blocks outward or
forward."
  (interactive "p")
  (if (re-search-forward ascend-block-end-regexp nil t count)
      ;; if this fails, we haven't moved.  if it succeeds, we are
      ;; sitting just after END.  Go backward one word to the start of
      ;; END, then get the position where of the matching block start.
      ;; Go to the end of the line, push the mark to the beginning of
      ;; the block, and then activate the region.
      (let (beg-defun)
        (skip-syntax-backward "w_")
        (setq beg-defun (ascend-get-matching-block-start))
        (end-of-line)
        (push-mark beg-defun nil t))))


;;;
;;;  Movement functions (interactive)  ---------------------------------------
;;;


(defun ascend-backward-block (count)
  "Move backward to the next statement that begins an ASCEND block.
With argument COUNT, move backward COUNT begin statements.  Treats
comments and NOTES as whitespace."
  (interactive "p")
  (goto-char (or (ascend-pos-block-backward count) (point-min))))


(defun ascend-forward-block (count)
  "Move forward to the next statement that ends an ASCEND block.  With
argument COUNT, move forward COUNT end statements.  Treats comments
and NOTES as whitespace."
  (interactive "p")
  (goto-char (or (ascend-pos-block-forward count) (point-min)))
  (end-of-line))


(defun ascend-beginning-of-block (count)
  "Go to the beginning of the current block.  With argument COUNT,
move outward COUNT blocks.  Treats comments and NOTES as whitespace.

This function differs from ascend-backward-block in that matching
begin-block/end-block pairs are skipped, so that point moves to the
beginning of the block that contains point, not to the beginning of
the first begin-block statement we find."
  (interactive "p")
  (goto-char (or (ascend-pos-beginning-of-block count) (point-min))))


(defun ascend-end-of-block (count)
  "Go to the end of the current block.  With argument COUNT, move
outward COUNT blocks.  Treats comments and NOTES as whitespace.

This function differs from ascend-forward-block in that matching
begin-block/end-block pairs are skipped, so that point moves to the
end of the block that contains point, not to the end of the first
end-block statement we find."
  (interactive "p")
  (goto-char (or (ascend-pos-end-of-block count) (point-max)))
  (end-of-line))

;;;
;;;  Is point here?  ----------------------------------------------------------
;;;


(defun ascend-point-in-comment-p ()
  "If point is in an ASCEND comment, return the character
position where the comment begins; otherwise return nil."
  ;; Search backward for the first ascend-comment-start or
  ;; ascend-comment-end expression we see; if we find a
  ;; ascend-comment-start, we are in a comment and return point;
  ;; otherwise, we are not in a comment and return nil.  NOTE: Does
  ;; not handle nested comments; does not handle "(*)" correctly; does
  ;; not process ascend-comment-start/ascend-comment-end characters in
  ;; symbols and notes correctly.
  (save-match-data
    (save-excursion
      (and (re-search-backward ascend-comment-start-end nil t)
           (looking-at ascend-comment-start)
           (point)))))


;;;(defun ascend-point-in-nested-comment-p ()
;;;  "Return t if point is in a nested ASCEND comment."
;;;  ;; Set c to zero; search backward for ascend-comment-start and
;;;  ;; ascend-comment-end expressions; increment/decrement c for each
;;;  ;; ascend-comment-start/-end; if c is > 0 when we reach the
;;;  ;; beginning of the buffer, we are in a comment.  NOTE: Handles
;;;  ;; nested comments, but does not handle "(*)" correctly; does not
;;;  ;; process ascend-comment-start/ascend-comment-end characters in
;;;  ;; symbols and notes correctly.
;;;  (let ((c 0))
;;;    (save-excursion
;;;      (while (re-search-backward ascend-comment-start-end
;;;                                 nil t)
;;;        (setq c (if (looking-at ascend-comment-start) (1+ c) (1- c)))))
;;;    (> c 0)))


(defun ascend-point-in-note-p ()
  "Return the position of the starting character if point
is in an ASCEND notes block."
  (save-match-data
    (save-excursion
      (and (re-search-backward (concat ascend-notes-start "\\|"
                                       ascend-notes-end)
                               nil t)
           (looking-at ascend-notes-start)
           (point)))))


(defun ascend-point-in-string-p ()
  "Return the position of the starting character if point
is within an ASCEND string.
Assumes strings never contain newlines."
  (save-match-data
    (save-excursion
      (let (;; parse from the beginning-of-line to point
            (p (point))
            ;; the character that begins the string
            c)
        (beginning-of-line)
        ;; since {} are treated as string delimiters in XEmacs, we
        ;; have to watch for them in the call to parse-partial-sexp.
        ;; If we get \} as a close string character, pretend we are
        ;; not in a string
        (if (and (setq c (nth 3 (parse-partial-sexp (point) p)))
                 (null (eq c ?\})))
            (progn
              (search-backward (char-to-string c))
              (point)))))))


(defun ascend-point-in-left-margin-p ()
  "Return t if point is in left margin; the left margin is
the whitespace between the left edge of the page and the start of text
on the line."
  (save-excursion
    (skip-chars-backward " \t")
    (bolp)))


;;;
;;;  Where is some text?  -----------------------------------------------------
;;;


(defun ascend-pos-block-backward (count)
  "Return the position of the next unprotected
ascend-block-start-statement.  Return nil if we do not find a block
start statement."
  (save-excursion
    (let (;; match case inside of this function
          (case-fold-search nil)
          ;; tmp holds the beginning of the current comment, string,
          ;; or note
          tmp)
      (while (and (> count 0)
                  (re-search-backward ascend-block-start-regexp nil t))
        (cond (;; ignore strings, comments, and notes
               (setq tmp (or (ascend-point-in-string-p)
                             (ascend-point-in-comment-p)
                             (ascend-point-in-note-p)))
               (goto-char tmp))
              (;; ignore block-starts that following block-ends: go
               ;; backward one token to make sure we are not sitting
               ;; on the "FOR" of an "END FOR" statement
               (progn (skip-syntax-backward " ")
                      (skip-syntax-backward "w_")
		      (looking-at ascend-block-end-regexp)))
              (t
               (setq count (1- count)))))
        (match-beginning 0))))


(defun ascend-pos-block-forward (count)
  "Return the position of the next unprotected
ascend-block-end-statement.  Return nil if we do not find a block end
statement."
  (save-excursion
    (let (;; match case inside of this function
          (case-fold-search nil))
      (while (and (> count 0)
                  (re-search-forward ascend-block-end-regexp nil t))
        (if (not (or (ascend-point-in-string-p)
                     (ascend-point-in-comment-p)
                     (ascend-point-in-note-p)))
            (setq count (1- count))))
      (match-beginning 0))))


(defun ascend-pos-beginning-of-block (count)
  "Return the position of the start of the block that currently
contains point.  Return nil if we do not find a block-start statement.
Comments and notes are treated as whitespace.

This function searches backward for ascend-block-start-regexp.  This
function differs from ascend-pos-block-backward in that matching
block-end/block-start statements are ignored."
  (let (;; match case inside of this function
        (case-fold-search nil)
	;; level is used to keep track of the begin-blocks and end-blocks;
	;;       it is initially count, indicating we are inside count
	;;       levels of nested blocks
	(level count)
	;; tmp is used for random values, such as the start of a comment,
	;;     the current value of point, etc.
        tmp
        ;; the regexp to match the begins and ends of blocks
	(regex (concat "\\(" ascend-block-start-regexp "\\|"
		       ascend-block-end-regexp "\\)")))
    (save-excursion
      ;;  ;; move to the beginning of the line and see if we are looking
      ;;  ;; at a block-start, if so adjust the level.  we have to do
      ;;  ;; this, otherwise sitting just after the END keyword will
      ;;  ;; behave as if we're not in that block.
      ;;  (skip-syntax-backward " ")
      ;;  (beginning-of-line)
      ;;  (skip-chars-forward " \t")
      ;;  (if (and (looking-at ascend-block-start-regexp)
      ;;           (not (or (ascend-point-in-string-p)
      ;;                    (ascend-point-in-comment-p)
      ;;                    (ascend-point-in-note-p))))
      ;;      (setq level (1- level)))
      (while (and (> level 0)
		  (re-search-backward regex nil t))
	(cond (;; get out of the string, comment, or not
               (setq tmp (or (ascend-point-in-string-p)
                             (ascend-point-in-comment-p)
                             (ascend-point-in-note-p)))
	       (goto-char tmp))
	      (;; we're at block-end position; increase the level
               (looking-at ascend-block-end-regexp)
	       (setq level (1+ level)))
	      ;; at this point we know we are on a block-start.  go
	      ;; backward one token to make sure we are not sitting on the
	      ;; "FOR" of an "END FOR" statement
	      ((progn (setq tmp (- 0 (skip-syntax-backward " ")
				   (skip-syntax-backward "w_")))
		      (looking-at ascend-block-end-regexp))
	       (setq level (1+ level)))
	      (t
	       (forward-char tmp)
	       (setq level (1- level)))))
      (if (zerop level)
          (point)
        nil))))


(defun ascend-pos-end-of-block (count)
  "Return the position of the end of the block that currently contains
point.  Return nil if we do not find a block-end statement.  Comments
and notes are treated as whitespace.

This function searches forward for ascend-block-end-regexp.  This
function differs from ascend-pos-block-forward in that matching
block-end/block-start statements are ignored."
  (let (;; match case inside of this function
        (case-fold-search nil)
	;; level is used to keep track of the begin-blocks and end-blocks;
	;;       it is initially count, indicating we are inside coun
        ;;       levels of nested blocks
	(level count)
	;; tmp is used for random values, such as the start of a comment,
	;;     the current value of point, etc.
        tmp
        ;; the regexp to match the begins and ends of blocks
	(regex (concat "\\(" ascend-block-start-regexp "\\|"
		       ascend-block-end-regexp "\\)")))
    (save-excursion
      ;;  ;; move to the first non-whitespace character on the line and
      ;;  ;; see if we are looking at a block-end, if so adjust the level.
      ;;  ;; we have to do this, otherwise sitting just before a
      ;;  ;; block-start keyword will behave as if we're not in that
      ;;  ;; block.
      ;;  (skip-syntax-forward " ")
      ;;  (if (and (looking-at ascend-block-end-regexp)
      ;;           (not (or (ascend-point-in-string-p)
      ;;                    (ascend-point-in-comment-p)
      ;;                    (ascend-point-in-note-p))))
      ;;      (setq level (1- level)))
      (while (and (> level 0)
		  (re-search-forward regex nil t))
        ;; save point: since we move backward below, we need to return
        ;; here before our next time through the loop so we don't
        ;; match the same regexp again
        (setq tmp (point))
        ;; we're at the end of the regexp, move to the front
        (goto-char (match-beginning 0))
        (cond (;; if we are in a string, comment, or note, we need to
               ;; keep searching.  unfortunately, there is no quick
               ;; way to jump out the end of a string, comment, or
               ;; note like there is for the beginning.  Move to tmp
               ;; before we continue.
               (or (ascend-point-in-string-p)
                   (ascend-point-in-comment-p)
                   (ascend-point-in-note-p))
               (goto-char tmp))
              (;; we're at block-end position; decrease the level and
	       ;; move to the end of the line, otherwise, if sitting
	       ;; on ``END FOR;'' we will match the FOR as a
	       ;; block-start
               (looking-at ascend-block-end-regexp)
               (setq level (1- level))
               (end-of-line))
              (;; at this point we know we are on a block-start.  go
               ;; backward one token to make sure we are not sitting
               ;; on the "FOR" of an "END FOR" statement.  If we are
               ;; on END FOR, decrease the level and move to the end
               ;; of the line
               (progn (skip-syntax-backward " ")
                      (skip-syntax-backward "w_")
                      (looking-at ascend-block-end-regexp))
               (setq level (1- level))
               (end-of-line))
              (;; we know we are on a block start that is really a
               ;; block start.  increase the level and move to the end
               ;; of our orignial match
               t
               (setq level (1+ level))
               (goto-char tmp))))
      (if (zerop level)
	  (point)
	nil))))


(defun ascend-get-matching-block-start ()
  "Return the position of the start of the block which the
current token ends.  Signals an error if the matching block
start is not found.

This function expects to point to be sitting on an entry in the
ascend-block-end-regexp-list.  To find the beginning of the current
block from inside the block, call ascend-pos-beginning-of-block."
  (let (;; match case inside of this function
        (case-fold-search nil)
        ;; end-line is the line number where we are; it is used
	;;          when signaling an error.
	(end-line (1+ (count-lines (point-min) (point))))
	;; level is used to keep track of the begin-blocks and end-blocks;
	;;       it is initially 1, indicating we are inside a block
	(level 1)
	;; blocktype holds the token after the END
	blocktype
	;; tmp is used for random values, such as the start of a comment,
	;;     the current value of point, etc.
	tmp)
    (save-excursion
      ;; Signal an error if we are not where we expect to be
      (if (null (looking-at ascend-block-end-regexp))
	  (error "%s" "Not on an block-end-line"))
      ;; Skip over the block-end-regexp and any whitespace
      (goto-char (match-end 0))
      (skip-syntax-forward " ")
      ;; The next token is the type of the block
      (setq blocktype (buffer-substring (point) (progn
						  (skip-syntax-forward "w_")
						  (point))))
      ;; Signal an error if no token follows END
      (if (string= blocktype "")
	  (error "%s" "Missing token after END"))
      ;; See if we recognize the token after "END"
      (if (string-match (concat "\\`" ascend-block-start-regexp "\\'")
			blocktype)
	  (progn
	    ;; The word after "END"---blocktype---is a block-start
	    ;; keyword, so search backward for blocktype, adding a level
	    ;; if it is preceeded by "END" and subtracting a level if not.
	    ;; Return when the level is zero.
	    (beginning-of-line)
	    (while (and (> level 0)
			(re-search-backward (concat "\\b" blocktype "\\b")
					    nil t))
	      (if (setq tmp (or (ascend-point-in-string-p)
                                (ascend-point-in-comment-p)
                                (ascend-point-in-note-p)))
		  (goto-char tmp)
		;(forward-char 1)
		(setq tmp (point))
		(skip-syntax-backward " ")
		(skip-syntax-backward "w_")
		(if (looking-at ascend-block-end-regexp)
		    (setq level (1+ level))
		  (setq level (1- level)))))
	    (if (zerop level)
		tmp
	      (error "%s%s%s%d" "Cannot find beginning of " blocktype
		     " block that ends on line " end-line)))
	(progn
	  ;; We do not recognize the word after "END"---blocktype---as a
	  ;; block-start, so look for a block-start followed by
	  ;; blocktype.
	  (while (and (re-search-backward (concat ascend-block-start-regexp
						  "\\s-+" blocktype
						  "\\>")
					  nil t)
		      (setq tmp (or (ascend-point-in-string-p)
                                    (ascend-point-in-comment-p)
                                    (ascend-point-in-note-p))))
	    (goto-char tmp))
	  (if (looking-at (concat ascend-block-start-regexp "\\s-+"
				  blocktype "\\>"))
	      (point)
	    (error "%s%s%s%d" "Cannot find beginning of block " blocktype
		   " that ends on line " end-line)))))))


;;;
;;;  Misc functions  ----------------------------------------------------------
;;;


(defun ascend-undo-abbrev-in-comment ()
  "If point is in an ascend comment,
undo the previous abbrev expansion."
  (if (and (not ascend-expand-abbrevs-in-comments)
	   (or (ascend-point-in-string-p)
               (ascend-point-in-comment-p)
               (ascend-point-in-note-p)))
      (unexpand-abbrev)))


(defun ascend-version ()
  "Print the version number of ASCEND mode in the minibuffer"
  (interactive)
  (message "ASCEND mode version %s Ident: $Id$" ascend-mode-version))


(defun ascend-add-matching-end-block ()
  "Add the matching end"
  (let ((case-fold-search nil) block indentation)
    (save-excursion
      (beginning-of-line)
      (setq indentation (skip-chars-forward " \t"))
      (if (or (not (looking-at ascend-block-start-regexp))
              (looking-at "\\<ELSE\\>"))
          ;; nothing to do
          ()
        (if (looking-at "\\bUNIVERSAL\\b")
            (progn
              (skip-syntax-forward "w_")
              (skip-syntax-forward " ")))
        (if (looking-at "\\<\\(METHOD\\|ATOM\\|MODEL\\|DEFINITON\\)\\>")
            (progn
              (skip-syntax-forward "w_")
              (skip-syntax-forward " ")))
        (setq block (buffer-substring (point) (progn (skip-syntax-forward "w_")
                                                     (point))))
        (end-of-line)
        (insert "\nEND " block ";")
        (beginning-of-line)
        (indent-to indentation)))))
              

;;;
;;;  Menus  -------------------------------------------------------------------
;;;


(defun ascend-create-menu ()
  "Modify this buffer's menubar to include the ASCEND menu.
Won't create the menubar if one doesn't already exist."
  (interactive)
  (cond ((not ascend-menu)
         ;; do nothing
         )
	((and (string-match "XEmacs" emacs-version) current-menubar)
	 (set-buffer-menubar current-menubar)
	 (add-submenu nil (append '("ASCEND") ascend-menu)))
	((and (string-match "Lucid" emacs-version) current-menubar)
	 (set-buffer-menubar current-menubar)
	 (add-menu nil "ASCEND" (copy-tree ascend-menu)))
	((string-match "^[.0-9]+$" emacs-version)
         (load "easymenu")
         (easy-menu-define ascend ascend-mode-map "Ascend mode menu"
                           (cons "Ascend" ascend-menu)))))


;;;
;;;  Done  --------------------------------------------------------------------
;;;

(provide 'ascend-mode)

;; ascend-mode.el ends here
