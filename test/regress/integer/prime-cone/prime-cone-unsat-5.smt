(benchmark primecone
:status unsat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) (x_2 Int) (x_3 Int) (x_4 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (>= x_2 0)
:assumption (>= x_3 0)
:assumption (>= x_4 0)
:assumption (<= (+ (* (~ 27) x_0) (* 2 x_1) (* 2 x_2) (* 2 x_3) (* 2 x_4) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 26) x_1) (* 3 x_2) (* 3 x_3) (* 3 x_4) ) 0)
:assumption (<= (+ (* 5 x_0) (* 5 x_1) (* (~ 24) x_2) (* 5 x_3) (* 5 x_4) ) 0)
:assumption (<= (+ (* 7 x_0) (* 7 x_1) (* 7 x_2) (* (~ 22) x_3) (* 7 x_4) ) 0)
:assumption (<= (+ (* 11 x_0) (* 11 x_1) (* 11 x_2) (* 11 x_3) (* (~ 18) x_4) ) 0)
:assumption (>= (+ x_0 x_1 x_2 x_3 x_4 ) 1)
:assumption (<= (+ x_0 x_1 x_2 x_3 x_4 ) 27)
:formula true
)
