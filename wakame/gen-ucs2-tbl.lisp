(let ((tbl (make-array #x10000 :element-type 'fixnum :initial-element 0)))
  (handler-case 
   (with-open-file (in "unk_ch.def")
     (loop
      (setf (aref tbl (char-code #1=(read-char in)))
	    (parse-integer (s (progn #1# #1#))))
      (while (char/= (peek-char nil in) #\Newline)
	#1#)
      #1#))
   (error (c)
     (princ c)))
  ;;(setf (aref tbl 0) #xFF) ;; XXX: '\0'は特別扱いしてみる
  (defparameter *tbl* tbl))


(with-open-file (out "dat/char-category.map" 
		     :direction :output
		     :if-exists :supersede
		     :element-type '(unsigned-byte 8))
  (loop FOR c ACROSS *tbl* DO
    (write-byte c out)))
		     