(benchmark primecone
:status sat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) (x_2 Int) (x_3 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (>= x_2 0)
:assumption (>= x_3 0)
:assumption (<= (+ (* (~ 16) x_0) (* 2 x_1) (* 2 x_2) (* 2 x_3) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 15) x_1) (* 3 x_2) (* 3 x_3) ) 0)
:assumption (<= (+ (* 5 x_0) (* 5 x_1) (* (~ 13) x_2) (* 5 x_3) ) 0)
:assumption (<= (+ (* 7 x_0) (* 7 x_1) (* 7 x_2) (* (~ 11) x_3) ) 0)
:assumption (>= (+ x_0 x_1 x_2 x_3 ) 1)
:formula true
)
