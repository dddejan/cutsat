(benchmark primecone
:status sat
:category { crafted }
:logic QF_LIA
:extrafuns ((x_0 Int) (x_1 Int) )
:assumption (>= x_0 0)
:assumption (>= x_1 0)
:assumption (<= (+ (* (~ 4) x_0) (* 2 x_1) ) 0)
:assumption (<= (+ (* 3 x_0) (* (~ 3) x_1) ) 0)
:assumption (>= (+ x_0 x_1 ) 1)
:formula true
)
