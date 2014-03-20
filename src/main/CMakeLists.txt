add_executable(cutsat main.cpp)
add_dependencies(check cutsat)

foreach (DIR heuristics propagators constraints util solver parser)
  link_directories(${cutsat_BINARY_DIR}/src/${DIR})
  set(cutsat_LIBS ${DIR} ${cutsat_LIBS})
endforeach(DIR)

target_link_libraries(cutsat ${cutsat_LIBS} ${Boost_LIBRARIES} ${COIN_LIBRARY} ${CLN_LIBRARY} ${GMP_LIBRARY} ${ADDITIONAL_LIBRARIES} ${PTHREADS_LIBRARY} ${Z_LIBRARY} ${BZ2_LIBRARY})

# Add all the regression tests
foreach (TEST ${REGRESSION_TESTS})
  separate_arguments(ARGS UNIX_COMMAND ${CMAKE_SOURCE_DIR}/${TEST})
  list(GET ARGS 0 TEST_NAME)
  add_test(${TEST_NAME} cutsat ${ARGS})
endforeach(TEST)

# Installation files
install(TARGETS cutsat
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
