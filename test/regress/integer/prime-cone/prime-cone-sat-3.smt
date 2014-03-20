(benchmark primecone
:status sat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) (x_2 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (>= x_2 0)
:assumption (<= (+ (* (~ 9) x_0) (* 2 x_1) (* 2 x_2) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 8) x_1) (* 3 x_2) ) 0)
:assumption (<= (+ (* 5 x_0) (* 5 x_1) (* (~ 6) x_2) ) 0)
:assumption (>= (+ x_0 x_1 x_2 ) 1)
:formula true
)
