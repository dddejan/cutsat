(benchmark pigeonhole
:status unsat
:category { crafted }
:logic QF_LIA
:extrafuns ((p_0_0 Int) (p_0_1 Int) (p_1_0 Int) (p_1_1 Int) (p_2_0 Int) (p_2_1 Int) )
:assumption (>= p_0_0 0)
:assumption (<= p_0_0 1)
:assumption (>= p_0_1 0)
:assumption (<= p_0_1 1)
:assumption (>= p_1_0 0)
:assumption (<= p_1_0 1)
:assumption (>= p_1_1 0)
:assumption (<= p_1_1 1)
:assumption (>= p_2_0 0)
:assumption (<= p_2_0 1)
:assumption (>= p_2_1 0)
:assumption (<= p_2_1 1)
:assumption (>= (+ p_0_0 p_0_1 ) 1)
:assumption (>= (+ p_1_0 p_1_1 ) 1)
:assumption (>= (+ p_2_0 p_2_1 ) 1)
:assumption (<= (+ p_0_0 p_1_0 p_2_0 ) 1)
:assumption (<= (+ p_0_1 p_1_1 p_2_1 ) 1)
:formula true
)
