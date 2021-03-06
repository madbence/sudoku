# parallel sudoku solver

> parallel sudoku solver with `pthread`, uses backtracing as a last resort method,
  otherwise it tries to solve the puzzle like a human would do.

![example](example.gif)

![example](workers.gif)

## compile

```sh
$ g++ -pthread -std=c++11 -O0 -DT=4 -DW=1 -DSLOW=100 -o sudoku main.cpp
# T=4: use 4 worker threads
# W=1: workers will take 1 job per turn
# SLOW=100: show progress, sleep 100 ms between steps (SLOW=0 to disable progress display)

# optional:
# -DGENERATE: generate sudoku puzzle
# -DN=4: sudoku size (3 by default)
```

## run

```sh
$ curl 'http://staffhome.ecm.uwa.edu.au/~00013890/sudoku17' > puzzles.txt

# solve the first 5000 puzzles
$ ./sudoku

# solve specific puzzle
$ ./sudoku 000000010400000000020000000000050407008000300001090000300400200050100000000806000
```

## features

- it finds *every* solution (0, 1 or more)
- can solve *9x9*, *16x16*, *25x25*, etc sizes
- humanistic approach to improve speed
  - naked single (there is only one candidate for a cell)
  - hidden single (there are multiple candidates for a cell, but one of them appears only once in a row/col/block)
- generate new puzzles with unique solution

## license

MIT
