(defpackage :doar
  (:use :common-lisp)
  (:shadow :load :search)
  (:export :doar
           :load
           :search
	   :size
           :common-prefix-search))
(in-package :doar)

(defstruct (searcher (:conc-name ""))
  (flag #*0 :type simple-bit-vector)     
  (tind #() :type (simple-array fixnum)) 
  (base #() :type (simple-array fixnum)) 
  (chck #() :type (simple-array (unsigned-byte 8)))
  (tail #() :type (simple-array (unsigned-byte 8))))

(defun size (idx) (length (tind idx)))
(defmethod print-object ((obj searcher) stream)
  (format stream "#<SEACHER>"))

;;;;;;;;;;;;
;;; load ;;;
(defun to-fixnum (num) (ldb (byte 29 0) num))
(defun highest-bit (num) (ldb (byte 1 31) num))

(defun load (path)
  (with-open-file (in8 path :element-type '(unsigned-byte 8))
    (with-open-file (in32 path :element-type '(unsigned-byte 32))
      (read-byte in32) (read-byte in32)     ;; XXX: マジックストリングのチェックが省略されている
      (let* ((node-size #1=(read-byte in32))
             (tind-size #1#)
             (tail-size #1#)
             (tind-array (make-array tind-size :element-type '(unsigned-byte 32)))
             (base-array (make-array node-size :element-type '(unsigned-byte 32)))
             (chck-array (make-array node-size :element-type '(unsigned-byte 8)))
             (tail-array (make-array tail-size :element-type '(unsigned-byte 8))))
        (read-sequence tind-array in32) (file-position in32 (+ 5 tind-size))
        (read-sequence base-array in32) (file-position in8 #2=(* 4 (+ 5 tind-size node-size)))
        (read-sequence chck-array in8)  (file-position in8 (+ #2# node-size))
        (read-sequence tail-array in8)

        (make-searcher
         :tind (make-array tind-size :element-type 'fixnum 
                                     :initial-contents tind-array)
         :base (make-array node-size :element-type 'fixnum 
                                     :initial-contents (map 'vector #'to-fixnum base-array))
         :flag (make-array node-size :element-type 'bit 
                                     :initial-contents (map 'vector #'highest-bit base-array))
         :chck chck-array
         :tail tail-array)))))

;;;;;;;;;;;;;;
;;; search ;;;
(defvar *fastest* '(optimize (speed 3) (debug 0) (compilation-speed 0) (safety 0)))
(declaim (inline next-index get-id tail-index not-leaf? 
                 key-exists? key-including?)
         #.*fastest*)

(defmacro a.when (expr &body body)
  `(let ((it ,expr))
     (when it
       ,@body)))

(defmacro with-doar-abbrev ((doar) &body body)
  `(let ((base (base ,doar)) (chck (chck ,doar))
         (tind (tind ,doar)) (tail (tail ,doar))
         (flag (flag ,doar)))
     ,@body))

(defun get-id (node) node)
(defun tail-index (node tind) (aref tind node))
(defun next-index (base-node code) (+ base-node code))
(defun not-leaf? (node bits) (zerop (sbit bits node)))

(defun key-exists? (key  beg1 end1 
                    tail beg2 end2 &aux (end2 (min end2 (+ beg2 (- end1 beg1) 1))))
  (and (zerop (aref tail (1- end2)))
       (common-lisp:search key tail :start1 beg1 :start2 beg2 :end2 end2)))

(defun key-including? (key beg1 end1 tail beg2)
  (do ((i beg1 (1+ i)) (j beg2 (1+ j)))
      ((zerop (aref tail j)) i)
    (declare (fixnum i j))
    (when (or (= i end1)
              (/= (aref key i) (aref tail j)))
      (return-from key-including? nil))))
       
(declaim (ftype (function ((simple-array (unsigned-byte 8)) doar) (or NULL fixnum)) search-impl))
(defun search-impl (key doar &aux (len (length key)))
  (with-doar-abbrev (doar)
    (let ((tail-len (length tail)))
      (do* ((i 0 (the fixnum (1+ i)))
            (node (aref base 0))
            (code (aref key 0) (if (= len i) 0 (aref key i)))
            (next #1=(next-index node code) #1#))
           ((zerop code) (and (zerop  (aref chck next))
                              (get-id (aref base next))))
        (setf node (aref base next))
        (unless (and (= code (aref chck next))
                     (cond ((not-leaf? next flag) t) ; next loop
                           ((key-exists? key (1+ i) len tail (tail-index node tind) tail-len)
                            (return-from search-impl (get-id node)))))
          (return-from search-impl nil))))))

(declaim (ftype (function ((simple-array (unsigned-byte 8)) doar) list)
                common-prefix-search-impl))
(defun common-prefix-search-impl (key doar &aux (len (length key)) acc)
  (with-doar-abbrev (doar)
    (do* ((i 0 (the fixnum (1+ i)))
          (node (aref base 0))
          (code (aref key 0) (if (= len i) 0 (aref key i)))
          (next #1=(next-index node code) #1#))
         ((zerop code) (and (zerop  (aref chck next))
                            (push `(,i . ,(get-id (aref base next))) acc))
                       (nreverse acc))
      (when (and (/= i 0)
                 (zerop (aref chck #2=(next-index node 0))))
        (push `(,i . ,(get-id (aref base #2#))) acc))

      (unless (= code (aref chck next))
        (return-from common-prefix-search-impl (nreverse acc)))
      (setf node (aref base next))  
      (unless (not-leaf? next flag)
        (a.when (key-including? key (1+ i) len tail (tail-index node tind))
          (push `(,it . ,(get-id node)) acc))
        (return-from common-prefix-search-impl (nreverse acc))))))

;; public interface
(declaim (inline search))

;; -> id
(defun search (key doar)
  (search-impl (sb-ext:string-to-octets key) doar))
 
;; ((end-pos . id))
(defun common-prefix-search (key doar &key (start 0))
  (declare (fixnum start)
           (simple-string key))
  (let ((octets (sb-ext:string-to-octets key :start start)))
    (loop FOR (end-pos . id) IN (common-prefix-search-impl octets doar) COLLECT
      (cons (the fixnum 
                 (+ start
                    (length (sb-ext:octets-to-string octets :end end-pos))))
            id))))
