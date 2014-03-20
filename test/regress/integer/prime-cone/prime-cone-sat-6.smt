(benchmark primecone
:status sat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) (x_2 Int) (x_3 Int) (x_4 Int) (x_5 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (>= x_2 0)
:assumption (>= x_3 0)
:assumption (>= x_4 0)
:assumption (>= x_5 0)
:assumption (<= (+ (* (~ 40) x_0) (* 2 x_1) (* 2 x_2) (* 2 x_3) (* 2 x_4) (* 2 x_5) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 39) x_1) (* 3 x_2) (* 3 x_3) (* 3 x_4) (* 3 x_5) ) 0)
:assumption (<= (+ (* 5 x_0) (* 5 x_1) (* (~ 37) x_2) (* 5 x_3) (* 5 x_4) (* 5 x_5) ) 0)
:assumption (<= (+ (* 7 x_0) (* 7 x_1) (* 7 x_2) (* (~ 35) x_3) (* 7 x_4) (* 7 x_5) ) 0)
:assumption (<= (+ (* 11 x_0) (* 11 x_1) (* 11 x_2) (* 11 x_3) (* (~ 31) x_4) (* 11 x_5) ) 0)
:assumption (<= (+ (* 13 x_0) (* 13 x_1) (* 13 x_2) (* 13 x_3) (* 13 x_4) (* (~ 29) x_5) ) 0)
:assumption (>= (+ x_0 x_1 x_2 x_3 x_4 x_5 ) 1)
:formula true
)
