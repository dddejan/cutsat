(benchmark primecone
:status unsat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) (x_2 Int) (x_3 Int) (x_4 Int) (x_5 Int) (x_6 Int) (x_7 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (>= x_2 0)
:assumption (>= x_3 0)
:assumption (>= x_4 0)
:assumption (>= x_5 0)
:assumption (>= x_6 0)
:assumption (>= x_7 0)
:assumption (<= (+ (* (~ 76) x_0) (* 2 x_1) (* 2 x_2) (* 2 x_3) (* 2 x_4) (* 2 x_5) (* 2 x_6) (* 2 x_7) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 75) x_1) (* 3 x_2) (* 3 x_3) (* 3 x_4) (* 3 x_5) (* 3 x_6) (* 3 x_7) ) 0)
:assumption (<= (+ (* 5 x_0) (* 5 x_1) (* (~ 73) x_2) (* 5 x_3) (* 5 x_4) (* 5 x_5) (* 5 x_6) (* 5 x_7) ) 0)
:assumption (<= (+ (* 7 x_0) (* 7 x_1) (* 7 x_2) (* (~ 71) x_3) (* 7 x_4) (* 7 x_5) (* 7 x_6) (* 7 x_7) ) 0)
:assumption (<= (+ (* 11 x_0) (* 11 x_1) (* 11 x_2) (* 11 x_3) (* (~ 67) x_4) (* 11 x_5) (* 11 x_6) (* 11 x_7) ) 0)
:assumption (<= (+ (* 13 x_0) (* 13 x_1) (* 13 x_2) (* 13 x_3) (* 13 x_4) (* (~ 65) x_5) (* 13 x_6) (* 13 x_7) ) 0)
:assumption (<= (+ (* 17 x_0) (* 17 x_1) (* 17 x_2) (* 17 x_3) (* 17 x_4) (* 17 x_5) (* (~ 61) x_6) (* 17 x_7) ) 0)
:assumption (<= (+ (* 19 x_0) (* 19 x_1) (* 19 x_2) (* 19 x_3) (* 19 x_4) (* 19 x_5) (* 19 x_6) (* (~ 59) x_7) ) 0)
:assumption (>= (+ x_0 x_1 x_2 x_3 x_4 x_5 x_6 x_7 ) 1)
:assumption (<= (+ x_0 x_1 x_2 x_3 x_4 x_5 x_6 x_7 ) 76)
:formula true
)
