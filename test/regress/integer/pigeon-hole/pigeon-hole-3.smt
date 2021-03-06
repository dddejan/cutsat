(benchmark pigeonhole
:status unsat
:category { crafted }
:logic QF_LIA
:extrafuns ((p_0_0 Int) (p_0_1 Int) (p_0_2 Int) (p_1_0 Int) (p_1_1 Int) (p_1_2 Int) (p_2_0 Int) (p_2_1 Int) (p_2_2 Int) (p_3_0 Int) (p_3_1 Int) (p_3_2 Int) )
:assumption (>= p_0_0 0)
:assumption (<= p_0_0 1)
:assumption (>= p_0_1 0)
:assumption (<= p_0_1 1)
:assumption (>= p_0_2 0)
:assumption (<= p_0_2 1)
:assumption (>= p_1_0 0)
:assumption (<= p_1_0 1)
:assumption (>= p_1_1 0)
:assumption (<= p_1_1 1)
:assumption (>= p_1_2 0)
:assumption (<= p_1_2 1)
:assumption (>= p_2_0 0)
:assumption (<= p_2_0 1)
:assumption (>= p_2_1 0)
:assumption (<= p_2_1 1)
:assumption (>= p_2_2 0)
:assumption (<= p_2_2 1)
:assumption (>= p_3_0 0)
:assumption (<= p_3_0 1)
:assumption (>= p_3_1 0)
:assumption (<= p_3_1 1)
:assumption (>= p_3_2 0)
:assumption (<= p_3_2 1)
:assumption (>= (+ p_0_0 p_0_1 p_0_2 ) 1)
:assumption (>= (+ p_1_0 p_1_1 p_1_2 ) 1)
:assumption (>= (+ p_2_0 p_2_1 p_2_2 ) 1)
:assumption (>= (+ p_3_0 p_3_1 p_3_2 ) 1)
:assumption (<= (+ p_0_0 p_1_0 p_2_0 p_3_0 ) 1)
:assumption (<= (+ p_0_1 p_1_1 p_2_1 p_3_1 ) 1)
:assumption (<= (+ p_0_2 p_1_2 p_2_2 p_3_2 ) 1)
:formula true
)
