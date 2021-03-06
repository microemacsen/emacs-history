;; Incremental search]
;; Copyright (C) 1985 Richard M. Stallman. 
 
;; This file is part of GNU Emacs. 
 
;; GNU Emacs is distributed in the hope that it will be useful, 
;; but without any warranty.  No author or distributor 
;; accepts responsibility to anyone for the consequences of using it 
;; or for whether it serves any particular purpose or works at all, 
;; unless he says so in writing. 
 
;; Everyone is granted permission to copy, modify and redistribute 
;; GNU Emacs, but only under the conditions described in the 
;; document "GNU Emacs copying permission notice".   An exact copy 
;; of the document is supposed to have been given to you along with 
;; GNU Emacs so that you can know how you may redistribute it all. 
;; It should be in a file named COPYING.  Among other things, the 
;; copyright notice and this notice must be preserved on all copies. 
 
 
;; This function does all the work of incremental search. 
;; The functions attached to ^R and ^S are trivial, 
;; merely calling this one, but they are always loaded by default 
;; whereas this file can optionally be autoloadable. 
;; This is the only entry point in this file. 
 
(defun isearch (forward &optional regexp) 
  (let ((search-string "") 
	(search-message "") 
	(cmds nil) 
	(success t) 
	(other-end nil)    ;Start of last match if fwd, end if backwd. 
	(odot (dot)) 
	(inhibit-quit t))  ;Prevent ^G from quitting immediately. 
    (isearch-push-state) 
    (catch 'search-done 
      (while t 
	(isearch-message) 
	(let ((char (if quit-flag 
			(discard-input) 
		      (read-char)))) 
	  (setq quit-flag nil) 
	  ;; Meta character means exit search. 
	  (if (and (>= char 128) 
		   search-exit-option) 
	      (progn 
		(setq unread-command-char char) 
		(throw 'search-done t))) 
	  (if (eq char search-exit-char) 
	      ;; Esc means exit search normally. 
	      ;; Except, if first thing typed, it means do nonincremental. 
	      (progn (if (= 0 (length search-string)) 
			 (nonincremental-search forward regexp)) 
		     (push-mark odot) 
		     (throw 'search-done t))) 
	  (if (= char ?\^G) 
	      ;; ^G means the user tried to quit. 
	      (if success 
		  ;; If search is successful, move back to starting dot 
		  ;; and really do quit. 
		  (progn (ding) 
			 (discard-input) 
			 (goto-char odot) 
			 (setq inhibit-quit nil quit-flag t) 
			 ;; Cause the quit, just requested, to happen. 
			 (eval '(progn nil))) 
		;; If search is failing, rub out until it is once more successful. 
		(ding) 
		(discard-input) 
		(while (not success) (isearch-pop))) 
	    (if (eq char search-repeat-char) 
		;; ^S means search again, forward, for the same string. 
		(progn (setq forward t) 
		       (if (null (cdr cmds)) 
			   ;; If the first char typed, 
			   ;; it means search for the string of the previous search. 
			   (setq search-string search-last-string 
				 search-message  
				 (mapconcat 'text-char-description 
					    search-string ""))) 
		       (isearch-search) 
		       (isearch-push-state)) 
	      (if (eq char search-reverse-char) 
		  ;; ^R is similar but it searches backward. 
		  (progn (setq forward nil) 
			 (if (null (cdr cmds)) 
			     (setq search-string search-last-string 
				   search-message 
				   (mapconcat 'text-char-description 
					      search-string ""))) 
			 (isearch-search) 
			 (isearch-push-state)) 
		(if (= char 127) 
		    ;; Rubout means discard last input item and move dot back. 
		    ;; If buffer is empty, just beep. 
		    (if (null (cdr cmds)) 
			(ding) 
		      (isearch-pop)) 
		  (if (or (eq char search-yank-word-char) 
			  (eq char search-yank-line-char)) 
		      ;; ^W means gobble next word from buffer. 
		      ;; ^Y means gobble rest of line from buffer. 
		      (let ((word (buffer-substring (dot) 
				    (save-excursion (if (eq char search-yank-line-char) 
							(end-of-line) 
						      (forward-word 1)) 
						    (dot))))) 
			(setq search-string (concat search-string word) 
			      search-message 
				(concat search-string 
					(mapconcat 'text-char-description 
						   word "")))) 
		    (if (= char ?\^Q) 
			(progn 
			  (isearch-message t) 
			  (setq char (read-char))) 
		      ;; Any other control char => unread it and exit the search normally. 
		      (if (and search-exit-option 
			       (< char ? ) (/= char ?\t) (/= char ?\^m)) 
			  (progn (setq unread-command-char char) 
				 (push-mark odot) 
				 (throw 'search-done t)))) 
		    (if (= char ?\^m) (setq char ?\n)) 
		    ;; Any other character => add it to the search string and search. 
		    (setq search-string (concat search-string (char-to-string char)) 
			  search-message (concat search-message (text-char-description char)))) 
		  (if (or (not success) 
			  (and regexp (= char ?\\))) 
		      nil 
		    (if other-end 
			(goto-char (+ (if forward 0 1) other-end))) 
		    (isearch-search)) 
		  (isearch-push-state)))))))) 
    (setq search-last-string search-string))) 
 
(defun isearch-message (&optional ctl-q) 
  (message "%s" 
	   (concat (if success "" "Failing ") 
		   (if regexp (if success "Regexp " "regexp ") "") 
		   (if forward "I-search: " "I-search backward: ") 
		   search-message 
		   (if ctl-q "^Q" "")))) 
 
(defun isearch-pop () 
  (setq cmds (cdr cmds)) 
  (let ((cmd (car cmds))) 
    (setq search-string (car cmd) 
	  search-message (car (cdr cmd)) 
	  success (car (cdr (cdr (cdr cmd)))) 
	  forward (car (cdr (cdr (cdr (cdr cmd))))) 
	  other-end (car (cdr (cdr (cdr (cdr (cdr cmd))))))) 
    (goto-char (car (cdr (cdr cmd)))))) 
 
(defun isearch-push-state () 
  (setq cmds (cons (list search-string search-message (dot) 
			 success forward other-end) 
		   cmds))) 
 
(defun isearch-search () 
  (if (setq success 
	    (condition-case () 
		(let ((inhibit-quit nil)) 
		  (funcall 
		   (if regexp 
		       (if forward 're-search-forward 're-search-backward) 
		     (if forward 'search-forward 'search-backward)) 
		   search-string nil t)) 
	      (search-failed (setq unread-command-char ?\^g) 
			     nil))) 
      (setq other-end 
	    (if forward (match-beginning 0) (match-end 0))) 
    (ding) 
    (goto-char (car (cdr (cdr (car cmds))))))) 
 
;; This is called from incremental-search 
;; if the first input character is the exit character. 
;; We store the search string in  search-string 
;; which has been bound already by incremental-search 
;; so that, when we exit, it is copied into search-last-string. 
(defun nonincremental-search (forward regexp) 
  (let (message char (inhibit-quit nil)) 
    ;; Prompt assuming not word search, 
    (setq message (if regexp  
		      (if forward "Regexp search: " 
			"Regexp search backward: ") 
		    (if forward "Search: " "Search backward: "))) 
    (message message) 
    ;; Read 1 char and switch to word search if it is ^W. 
    (setq char (read-char)) 
    (if (eq char search-yank-word-char) 
	(setq message (if forward "Word search: " "Word search backward: ")) 
      ;; Otherwise let that 1 char be part of the search string. 
      (setq unread-command-char char)) 
    ;; Read the search string with corrected prompt. 
    (setq search-string (read-string message)) 
    ;; Empty means use default. 
    (if (= 0 (length search-string)) 
	(setq search-string search-last-string) 
      ;; Set last search string now so it is set even if we fail. 
      (setq search-last-string search-string)) 
    ;; Go ahead and search. 
    (funcall (if (eq char search-yank-word-char) 
		 (if forward 'word-search-forward 'word-search-backward) 
	       (if regexp 
		   (if forward 're-search-forward 're-search-backward) 
		 (if forward 'search-forward 'search-backward))) 
	     search-string))) 
