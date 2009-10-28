(defpackage :doar
  (:use :common-lisp :common-utils)
  (:shadow :load :search)
  (:export :doar
           :load
           :string-to-octets
           :octets-to-string
           :search
           :string-search
           :common-prefix-search))
(in-package :doar)