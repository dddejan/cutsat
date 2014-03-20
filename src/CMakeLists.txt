foreach(DIR constraints parser solver propagators heuristics util main)
  add_subdirectory(${DIR})
endforeach(DIR)
