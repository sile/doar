;; parse mecab matrix.def
(defun gen-matrix (file)
  (with-open-file (in file)
    (let* ((left-size (read in))
	   (right-size (read in))
	   (matrix (make-array (list left-size right-size)
			       :element-type 'fixnum)))
      (dotimes (lft left-size)
	(format t " # ~A~%" lft)
	(dotimes (rgt right-size)
	  (let ((before-id (read in))
		(after-id (read in))
		(cost (read in)))
	    (setf (aref matrix lft rgt) cost))))
      matrix)))

(require :asdf)
(require :cl-ppcre)
;;(load "pairing-heap")
(load "doar")
(load "gen-matrix")

(deftype octet  () '(unsigned-byte 8))
(deftype octets () '(vector octet))

(defvar *fastest* '(optimize (speed 3) (debug 0) (safety 0) 
			     (compilation-speed 0)))
(defvar *note* '(unmuffle-conditions compiler-note))

;;;;;;;;;;;;
(defstruct word
  (lid  0 :type fixnum)
  (rid  0 :type fixnum)
  (cost 0 :type fixnum)
  info)

(defun gen-word-map (csv-file idx)
  (with-open-file (in csv-file)
    (let ((map (make-array (doar::keyset-size idx) :initial-element nil))
	  (cnt 0))
      (a.while (read-line in nil nil)
        (when (zerop (mod (incf cnt) 1000))
	  (princ (s "# " cnt #\Newline)))
        (destructuring-bind (word lid rid cost info) (cl-ppcre:split #\, it :limit 5)
          (let ((id (doar:search word idx)))
	    (assert id () "~A not found" word)
	    (push (make-word :lid  (parse-integer lid)
			     :rid  (parse-integer rid)
			     :cost (parse-integer cost)
			     :info info)
		  (aref map id)))))
      map)))

(defparameter *idx* (doar:load "../key.idx"))
(defparameter *matrix* (gen-matrix "../matrix.def"))
(defparameter *word-map* (gen-word-map "../word.csv" *idx*))

(defun get-words (key &optional (idx *idx*) (wmap *word-map*))
  (declare #.*fastest* #.*note*)
  (etypecase key
    (string (a.when (doar:search key idx)
              (svref wmap it)))
    (number (svref wmap key))))

(defun lnk-cost (lftw rgtw &optional (matrix *matrix*))
  (declare #.*fastest* #.*note*
	   ((simple-array fixnum (* *)) matrix))
  (aref matrix
	(word-rid lftw)
	(word-lid rgtw)))

(defmethod print-object ((obj word) stream)
  (with-slots (lid cost info) obj  ; XXX: lid==ridというipadicに特化した出力
    (format stream "[~D ~D :~A]" lid cost 
	    (first (cl-ppcre:split #\, info :limit 2)))))

;; TODO: 未知語を埋める
;; 開始位置をindexにした配列にmapする
(defparameter *unknown* (make-word :rid 0 :lid 521 :cost 5000 :info "未知語,*,*,*,*"))
(defparameter *bos* (make-word :rid 0 :lid 0 :cost 0 :info "*,*,*,*,*,BOS"))
(defparameter *eos* (make-word :rid 0 :lid 0 :cost 0 :info "*,*,*,*,*,EOS"))

(defvar *max* most-positive-fixnum)

;; 形態素
(defstruct morp
  (beg -1 :type fixnum)
  (end -1 :type fixnum)
  (text "" :type string)
  (cost *max* :type fixnum)
  prev
  word)

(defmethod print-object ((obj morp) stream)
  (with-slots (beg end text cost prev word) obj 
    (format stream "{~D-~D ~S ~X ~A ~A}" 
	    beg end text cost prev word)))

(defun lattice (text &optional (*idx* *idx*))
  (declare #.*fastest* #.*note*
	   (simple-string text))
  (let (acc)
    (dotimes (beg (length text))
      (dolist (r (doar:common-prefix-search text *idx* :start beg))
	(destructuring-bind (end . id) r
	  (dolist (word (get-words id))
	    (push (make-morp :beg beg :end end 
			     :text (subseq text beg end) ; TODO: 後で計算可能
			     :word word)
		  acc)))))
    (nreverse acc)))

(defun lattice-vector (text &optional (*idx* *idx*))
  (declare #.*fastest* #.*note*
	   (simple-string text))
  (let* ((morps (lattice text *idx*))
	 (size  (length text))
	 (begs  (make-array (1+ size) :initial-element '()))
	 (ends  (make-array (1+ size) :initial-element '())))
    (dolist (m morps)
      (push m (svref begs (morp-beg m))))
    
    (dotimes (i size)
      (unless #1=(svref begs i)
        (push (make-morp :beg i :end (1+ i) :text "_" :word *unknown*)
	      #1#)))

    ;;
    (loop for ms across begs do
      (dolist (m ms)
	(push m (svref ends (morp-end m)))))
    
    ;; BOS/EOS
    (push (make-morp :beg 0 :end 0 :text "BOS" :cost 0 :word *bos*) (svref ends 0))
    (push (make-morp :beg #2=(1- (length begs)) :end #2# :text "EOS" :word *eos*) (svref begs #2#))

    (values begs ends)))

(defun viterbi (text &optional (*word-map* *word-map*) (*matrix* *matrix*) (*idx* *idx*))
  (declare #.*fastest* #.*note*
	   (simple-string text))
  (multiple-value-bind (begs ends) (lattice-vector text)
    (declare (simple-vector begs ends))
    (loop for beg-ms across begs do
      (dolist (beg-m beg-ms)
	(let (min) ; TODO: improve
	  (dolist (end-m (svref ends (morp-beg beg-m)))
	    (unless (= (morp-cost end-m) (the fixnum *max*))
	      (let ((cost (the fixnum (+ (morp-cost end-m)
					 (the fixnum (word-cost (morp-word beg-m)))
					 (the fixnum (lnk-cost (morp-word end-m) (morp-word beg-m)))))))
		(when (or (null min)
			  (< cost (the fixnum (car min))))
		  (setf min (cons cost end-m))))))
	  (when min
	    (setf (morp-cost beg-m) (car min)
		  (morp-prev beg-m) (cdr min))))))
    begs))

(defun min-path (begs)
  (car (svref begs (1- (length begs)))))

(defun extract-word (rlt &optional acc)
  (if (null rlt)
      acc
    (extract-word (morp-prev rlt) (cons (morp-text rlt) acc))))

(defun wakati (text &optional (*idx* *idx*) (*word-map* *word-map*) (*matrix* *matrix*))
  (extract-word (min-path (viterbi text))))

;; NOTE: 単語辞書の中で、単語名・品詞IDが同じで、コストだけ異なる単語がある場合、一番低いもの以外は削除してしまって問題ない?

(time
 (let ((cnt 0))
  (with-open-file (in "/home/ohta/kokoro")
    (a.while (read-line in nil nil)
      (when (zerop (mod (incf cnt) 100))
	(print (wakati it))
	(format t " # ~D~%" cnt))
      (wakati it)))))