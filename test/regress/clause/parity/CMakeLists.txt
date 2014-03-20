file(GLOB files "${CMAKE_CURRENT_SOURCE_DIR}/*.cnf")
set(files README ${files})
install(FILES ${files} DESTINATION share/doc/cutsat/examples/parity)
