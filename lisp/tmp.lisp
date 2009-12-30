;;;;;;;;;;;;;;;;;
;;; 使用モジュール
(load "doar")

;;;;;;;;;;;;;                
;;; 構造体
;; 単語(形態素)用の構造体
;; スロットは、MeCabの単語辞書の行の各項目に対応する
(defstruct word
  (surface  "" :type string) ; 表層形(単語名)
  (left-id   0 :type fixnum) ; 左連接状態番号  
  (right-id  0 :type fixnum) ; 右連接状態番号  
  (cost      0 :type fixnum) ; 単語コスト　　  
  info)                      ; 単語の付加情報: 普通は、品詞や読みなど。ただし任意の情報が可。キー(単語)に対応する値

(defmethod print-object ((obj word) stream)
  (with-slots (surface info) obj
    (format stream "[~S:~A]" surface info)))

;; wordに位置情報や累積コストなどを付与した構造体
(defstruct node 
  word                                     ; word構造体
  (beg -1 :type fixnum)                    ; 開始位置
  (end -1 :type fixnum)                    ; 終了位置
  (cost most-positive-fixnum :type number) ; ノードのコスト
  prev)                                    ; 連接するノード

(defmethod print-object ((obj node) stream)
  (with-slots (beg end word prev) obj 
    (format stream "{~D-~D ~S ~S}" beg end word prev)))


;;;;;;;;;;;;;;;;
;;; スペシャル変数
;; 辞書関連
(defvar *da* (doar:load "../key.idx"))
(defvar *wdic*)   ; 単語辞書用: (aref *wdic* 単語ID) => (list word構造体)
        
;; EOS,BOS,未知語の定義
(defvar *bos*     (make-word :surface "BOS" :info "*,*,*,*,*,BOS"))
(defvar *eos*     (make-word :surface "EOS" :info "*,*,*,*,*,EOS"))
(defvar *unknown* (make-word :surface "_"
                             :left-id  1285  ; "名詞,一般"の品詞IDを割り当てておく
                             :right-id 1285
                             :cost     20000 ; 適当に大きいコスト
                             :info "未知語,*,*,*,*"))

;;;;;;;;;;;;;;;
;;; 単語辞書操作
;; 読込み
(defun load-word-dic (csv-file &optional (*da* *da*))
  (let ((dic (make-array (doar:size *da*) :initial-element nil))
        (cnt 0))
    (with-open-file (in csv-file)
      (a.while (read-line in nil nil)
        ;; 進捗表示            
        (when (zerop (mod (incf cnt) 1000))
          (format t "# ~D~%" cnt))
        
        ;; 単語定義行を','で分割
        (destructuring-bind (surface lid rid cost info) (split-by-chars "," it 4)
          (let ((id (doar:search surface *da*)))
            ;; wordを追加
            (push (make-word :surface   surface
                             :left-id   (parse-integer lid)
                             :right-id  (parse-integer rid)
                             :cost      (parse-integer cost)
                             :info      info)
                  (aref dic id))))))
    dic))
;; 実際に読み込む
(defparameter *wdic* (load-word-dic "../word.csv"))


;; 単語名 or 単語ID から、対応するwordのリストを取得する
(defun get-words (word &optional (*da* *da*) (*wdic* *wdic*))
  (etypecase word
    (string (when #1=(doar:search word *da*)
              (svref *wdic* #1#)))
    (fixnum (svref *wdic* word))))


;;;;;;;;;;;;;;;;;;;
;;; 入力テキスト解析
;; テキスト(の部分文字列)に一致する全ての辞書内の単語を抽出する
;; 'word'とあるが、実際にはwordに位置情報(等)を追加したnodeのリストが返される
(defun matched-word-list (text &optional (*da* *da*))
  (let ((list '()))
    (dotimes (beg-pos (length text))
      (dolist (pair (doar:common-prefix-search text *da* :start beg-pos))
        (destructuring-bind (end-pos . word-id) pair
          (dolist (word (get-words word-id))
            (push (make-node :word word
                             :beg beg-pos
                             :end end-pos)
                  list)))))
    (nreverse list)))

;; matched-word-listの結果に、BOS/EOS/未知語、を追加
;; ※ 関数名は不適切
(defun lattice (text &optional (*da* *da*) &aux (size (length text)))
  `(,(make-node :word *bos* :beg -1 :end 0 :cost 0)
    ,@(sort 
       (nconc
        (loop FOR i FROM 0 BELOW size
              COLLECT (make-node :word *unknown* :beg i :end (1+ i)))
        (matched-word-list text))
       (lambda (a b)
         (or (< (node-beg a) (node-beg b))
             (< (node-end a) (node-end b)))))
    ,(make-node :word *eos* :beg size :end (1+ size))))
    
;;;;;;;;;;;
;; 形態素解析
(defun load-matrix.def (file)
  (with-open-file (in file)
    (let* ((left-size (read in))
           (right-size (read in))
           (matrix (make-array (list left-size right-size)
                               :element-type 'fixnum)))
      (dotimes (lft left-size)
        (format t " # ~A~%" lft)
        (dotimes (rgt right-size)
          (read in) (read in)     ; 左連接ID(?)と右連接IDは、lftとrgtと同じなので読み飛ばす
          (let ((cost (read in)))
            (setf (aref matrix lft rgt) cost))))
      matrix)))
(defparameter *matrix* (load-matrix.def "../matrix.def"))

(defun link-cost (left-word right-word &optional (matrix *matrix*))
  (aref matrix
        (word-right-id left-word)
        (word-left-id  right-word)))

(defun select-min (list key)
  (if (null list)
      (values nil nil)
    (let ((min-elem  (car list))
          (min-value (funcall key (car list))))
      (dolist (elem (cdr list))
        (let ((value (funcall key elem)))
          (when (< value min-value)
            (setf min-elem  elem
                  min-value value))))
      (values min-elem min-value))))

;; 簡単だが非効率: 呼び出しごとにO(n)の処理が必要: n=(length node-list)
;; node-endをインデックスにした配列を利用すれば、O(1)で同様の処理が可能
(defun nodes-end-at (node-list end-pos)
  (remove end-pos node-list :key #'node-end :test #'/=))

(defun viterbi (text &optional (*wdic* *wdic*) (*matrix* *matrix*) (*da* *da*))
  (let ((nodes (lattice text)))
    (dolist (cur (cdr nodes)) ; BOSは飛ばす
      (multiple-value-bind (prev-node min-cost)
          (select-min (nodes-end-at nodes (node-beg cur))
                      (lambda (prev)
                        (+ (node-cost prev)
                           (word-cost (node-word cur))
                           (link-cost (node-word prev) (node-word cur)))))
        (setf (node-cost cur) min-cost
              (node-prev cur) prev-node)))
    ;; 最小コストのパスを返す (EOSノードから辿れるパスが最小コストとなっている)
    (car (last nodes))))

(defun extract-surface (node &optional acc)
  (if (null node)
      acc
    (extract-surface (node-prev node) 
                     (cons (word-surface (node-word node)) acc))))

(defun wakati (text &optional (*da* *da*) (*wdic* *wdic*) (*matrix* *matrix*))
  (extract-surface (viterbi text)))

(defun extract-info (node &optional acc)
  (if (null node)
      acc
    (with-slots (prev word) node
      (extract-info prev
                    (cons `(,(word-surface word) ,(word-info word)) acc)))))

(defun parse (text &optional (*da* *da*) (*wdic* *wdic*) (*matrix* *matrix*))
  (extract-info (viterbi text)))


;;;;;;;;
;; 出力
(defun node-label (node &key (id t))
  (with-slots (beg end word) node
    (format nil "~A~@[#~36R~][~D-~D]" 
            (word-surface word) 
            (and id (mod (sb-kernel:get-lisp-obj-address word) #36R10))
            beg end)))

(defun print-dot (text &key (*da* *da*) (out *standard-output*) remove-unknown)
  (format out "~&digraph lattice {~%")
  (format out "graph [rankdir=LR]~%")
  
  (let ((nodes (lattice text)))
    (when remove-unknown
      (setf nodes (remove *unknown* nodes :key #'node-word)))
    (dolist (node nodes)
      (format out "~S [label=~S]~%" (node-label node) (node-label node :id nil)))
    (dolist (next (cdr nodes))
      (dolist (prev (nodes-end-at nodes (node-beg next)))
        (format out "~S -> ~S~%" (node-label prev) (node-label next)))))
  (format out "~&}~%"))
    

(defun dot (text dot-file gif-file &key (remove-unknown t))
  (with-open-file (out dot-file :direction :output :if-exists :supersede)
    (print-dot text :out out :remove-unknown remove-unknown))
  (run-program "dot" (list "-s10" "-Tgif" "-o" gif-file dot-file) :search t))


(defun node-label-with-cost (node)
  (s (node-label node :id nil) "\\n" (word-cost (node-word node))))

(defun print-dot-with-cost (text &key (*da* *da*) (out *standard-output*) remove-unknown)
  (format out "~&digraph lattice {~%")
  (format out "graph [rankdir=LR]~%")
  
  (let ((nodes (lattice text)))
    (when remove-unknown
      (setf nodes (remove *unknown* nodes :key #'node-word)))
    (dolist (node nodes)
      (format out "~S [label=\"~A\"]~%" (node-label node) (node-label-with-cost node)))
    (dolist (next (cdr nodes))
      (dolist (prev (nodes-end-at nodes (node-beg next)))
        (format out "~S -> ~S [label=~S]~%" 
		(node-label prev) (node-label next)
		(link-cost (node-word prev) (node-word next))))))
  (format out "~&}~%"))

(defun dot-with-cost (text dot-file gif-file &key (remove-unknown t))
  (with-open-file (out dot-file :direction :output :if-exists :supersede)
    (print-dot-with-cost text :out out :remove-unknown remove-unknown))
  (run-program "dot" (list "-s10" "-Tgif" "-o" gif-file dot-file) :search t))


(defun print-dot-viterbi (text &key (*da* *da*) (out *standard-output*) remove-unknown only-min)
  (format out "~&digraph lattice {~%")
  (format out "graph [rankdir=LR]~%")
  
  (let ((nodes (lattice text)))
    (when remove-unknown
      (setf nodes (remove *unknown* nodes :key #'node-word)))
    (dolist (node nodes)
      (format out "~S [label=\"~A\"]~%" (node-label node) (node-label-with-cost node)))
    
    (unless only-min
      (dolist (next (cdr nodes))
	(dolist (prev (nodes-end-at nodes (node-beg next)))
	  (format out "~S -> ~S [label=~S]~%" 
		  (node-label prev) (node-label next)
		  (link-cost (node-word prev) (node-word next))))))

    ;; viterbi
    (dolist (cur (cdr nodes)) ; BOSは飛ばす
      (multiple-value-bind (prev-node min-cost)
        (select-min (nodes-end-at nodes (node-beg cur))
		    (lambda (prev)
		      (+ (node-cost prev)
			 (word-cost (node-word cur))
			 (link-cost (node-word prev) (node-word cur)))))
	(when prev-node
	  (format out "~S -> ~S [color=red,style=bold,fontcolor=blue,label=~S]~%" 
		  (node-label prev-node) (node-label cur)
		  min-cost)
	  (setf (node-cost cur) min-cost
		(node-prev cur) prev-node))))
    )
  (format out "~&}~%"))

(defun dot-viterbi (text dot-file gif-file &key (remove-unknown t) only-min)
  (with-open-file (out dot-file :direction :output :if-exists :supersede)
    (print-dot-viterbi text :out out :remove-unknown remove-unknown :only-min only-min))
  (run-program "dot" (list "-s10" "-Tgif" "-o" gif-file dot-file) :search t))

(defun viterbi (text &optional (*wdic* *wdic*) (*matrix* *matrix*) (*da* *da*))
  (let ((nodes (lattice text)))
    (dolist (cur (cdr nodes)) ; BOSは飛ばす
      (multiple-value-bind (prev-node min-cost)
          (select-min (nodes-end-at nodes (node-beg cur))
                      (lambda (prev)
                        (+ (node-cost prev)
                           (word-cost (node-word cur))
                           (link-cost (node-word prev) (node-word cur)))))
        (setf (node-cost cur) min-cost
              (node-prev cur) prev-node)))
    ;; 最小コストのパスを返す (EOSノードから辿れるパスが最小コストとなっている)
    (car (last nodes))))