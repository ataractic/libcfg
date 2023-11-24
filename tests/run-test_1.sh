#!/bin/bash

clang -std=c2x -Wall -Wextra test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -g -O0 -fsanitize=address test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -g -O0 -fsanitize=memory test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -fsanitize=thread test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -fsanitize=undefined test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -fsanitize=cfi -flto -fvisibility=hidden test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -fsanitize=kcfi test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
clang -std=c2x -Wall -Wextra -fsanitize=safe-stack test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}
# clang -std=c2x -Wall -Wextra -fsanitize=dataflow test_1.c ../src/cfg.c -o test_1.out && ./test_1.out ${1}