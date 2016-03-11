# parallel sudoku solver

## compile

```sh
$ g++ -pthread -std=c++11 -o sudoku main.cpp
```

## run

```sh
$ curl http://staffhome.ecm.uwa.edu.au/~00013890/sudoku17 > puzzles.txt
$ ./sudoku
```

## features

- it finds *every* solution
- can solve *9x9*, *16x16*, *25x25*, etc sizes

## future

- humanistic approach to improve speed
  - [ ] naked single
  - [ ] hidden single

## license

MIT
