; demo.lsp - a short XLISP 1.1 tour for CP/M-86
;
; Run with:  xlisp demo
;
; Note: in XLISP 1.1 the branches of 'if' are lists of expressions,
; so 'cond' is used below for the usual one-expression branches.

(defun fib (n)
    (cond ((< n 2) n)
          (t (+ (fib (- n 1)) (fib (- n 2))))))

(defun fact (n)
    (cond ((< n 2) 1)
          (t (* n (fact (- n 1))))))

(princ "XLISP on CP/M-86\n\n")

(princ "(fib 15)    => " (fib 15) "\n")
(princ "(fact 7)    => " (fact 7) "\n")

(princ "fib 1..10   => ")
(foreach x '(1 2 3 4 5 6 7 8 9 10) (princ (fib x) " "))
(princ "\n")

(princ "(reverse '(hello from cp/m-86)) => ")
(print (reverse '(hello from cp/m-86)))
(princ "\n")

(princ "(strcat \"XLISP \" \"on \" \"CP/M-86\") => ")
(print (strcat "XLISP " "on " "CP/M-86"))
(princ "\n")

(princ "\nType (exit 0) to leave.\n")
