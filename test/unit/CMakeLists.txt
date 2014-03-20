# Find the Boost unit test library
find_package(Boost 1.36.0 COMPONENTS unit_test_framework REQUIRED)

add_executable(cutsat_test test_runner.cpp)
add_dependencies(check cutsat_test)

# Original cutsat libraries
foreach (DIR heuristics propagators constraints util solver parser)
  link_directories(${cutsat_BINARY_DIR}/src/${DIR})
  set(cutsat_test_LIBS ${DIR} ${cutsat_test_LIBS})
endforeach(DIR)

# The test libraries
foreach (DIR constraints parser solver)
  add_subdirectory(${DIR})
  set(cutsat_test_LIBS -Wl,--whole-archive ${DIR}_test -Wl,--no-whole-archive ${cutsat_test_LIBS})
endforeach(DIR)

target_link_libraries(cutsat_test ${cutsat_test_LIBS} ${Boost_LIBRARIES} ${COIN_LIBRARY} ${CLN_LIBRARY} ${GMP_LIBRARY} ${PTHREADS_LIBRARY} ${Z_LIBRARY} ${BZ2_LIBRARY})

add_test(unit_tests cutsat_test -i)
