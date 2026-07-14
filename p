#!/usr/bin/bash
# to run this file: ./p
#
# nvim tips:
#   file manager: ":Neotree" or ", b f"
#     to get help on Neotree, just press ?
#     toggle between Neotree: Shift-f
#   splits
#     sp: horizontal split
#     vs: vertical split
#     only: keep only this split(close others)
#   switch between source/header: F2
#   search files: ctrl-f, alt-f, C/Tab
#   search strings: ctrl-g, alt-g, /
#   find string/text in whole project: ft <text> | rg <text>
#   folding/unfolding:
#     zM: close all folds.
#     zR: open all folds.
#     za: toggle one fold under cursor.
#     zA: toggle All folds under cursor.
#     zc: close one fold under cursor.
#     zC: close All folds under cursor.
#   documentation:
#     , c c: generate doxygen doc for a class
#     , n f: generate doxygen doc for a function
#     run: doxygen in root of project from a terminal
#   lsp: ,l
#   find/replace:
#     project: ambr <src> <dst> <path>
#     current buffer: :%s/foo/bar/g
#   diff between two files/dirs: delta <DIR1> <DIR2>
#   debuggers: dap in neovim, gf2, gdb
#   find docs of c standard librariy: 
#     - install man-pages-devel and man <method>
#     - zeal
#     - www.devdocs.io 

menu () {
  commands=(
    # debug
    "build(debug)" "run(debug)" "gf2" "rr record" "gf2 --rr-replay" "clean(debug)"

    # release
    "build(release)" "run(release)" "clean(release)"
  
    # tests
    "build(tests)" "run(tests)" "gdb(tests)" "clean(tests)"
 
    # documentation
    "doxygen(generate)" 
    "doxygen(show)" 
 
    # static analyzer
    "cppcheck(generate)"
    "cppcheck(show)"
 
    "frama-c" "frama-c (eva)" "ivette (eva)" "frama-c-gui (eva)"
  
    # code coverage
    "lcov"
    "gcovr"
    "genhtml"
    "show lcov report"
    "kcov(generate)"
    "kcov(show)"
 
    # we can't use lcov, gcov, gcovr(requires: gcc-multilib) with zagros, because these tools asks us to re-compile zagros with:
    #   --coverage -fprofile-arcs -ftest-coverage 
    #   linker flags: -L/usr/lib/gcc/x86_64-unknown-linux-gnu/14.2/32/ -lgcov
    # which is not possible. because there's no libc compiled with zagros. it's statically linked.
    # but we can use them for zagros_test
  
    # trace/profile
    "valgrind(memcheck)" "callgrind" "kcachegrind" "cachegrind" "helgrind" "massif" "ms_print" "drd" "dhat" "dhat(cat)" "bbv" "bbv(cat)"
    "perf"
    "uftrace replay"
    "uftrace record"
    "uftrace dump"
    "uftrace report"
    "uftrace info"
    "uftrace graph"
    "uftrace tui"
  
    # we can't use libasan as compiler flag. (-fsanitize=address) and as linker flag (-L/usr/lib32 -lasan),
    # since zagros is statically linked and this flag needs libc compiled with zagros.
    
    # we can't use uftrace, because of many reasons:
    #   1. it just works with dynamically linked binaries. zagros is statically linked, so uftrace can't work with it.
    #   2. it needs to pass -pg to compiler option, and this requires mcount from clib. but we're building our kernel without libc!
    #   3. even with -finstrument-functions, our binary is still static. so uftrace can't work!
  
    # alternative: manual tracing using this macro for each function:
    #   #define TRACE_ENTER() do { asm volatile("syscall" : : "a"(1), "D"(1), "S"("Entering " __func__ "\n"), "d"(strlen("Entering " __func__ "\n")) : "rcx", "r11", "memory"); } while(0)  // Raw write syscall// Add TRACE_ENTER() at function starts; similar for exit.
  
    # binary analyse
    "ldd" "objdump" "strings" "nm" "readelf" "addr2line" "size" "file" "xxd"
  
    # util
    "scc" "search" "search/replace")
  selected=$(printf '%s\n' "${commands[@]}" | fzf --header="project:")
  
  case $selected in
    "build(debug)")
      clear
      echo ">>> creating build/debug directory"
      mkdir -p build/debug
 
      echo ">>> compiling (debug mode)"
      bear -- g++ -std=c++23 -g -pg -O0 -DDEBUG --coverage \
          -Wformat=2 -Wall -Werror -Wextra -Wunused-function -Wpedantic -Wno-unused-parameter \
          -Wshadow -Wwrite-strings -Wunused-result  \
          -Wredundant-decls -Wmissing-include-dirs -Wlogical-op \
          -ldl -pthread -lm \
          src/*.cpp -o ./build/debug/ush 

      if [ $? -eq 1 ]; then
        # error
        mpg123 -f 3000 /home/$USER/VoidConf/assets/error2.mp3 > /dev/null 2>&1 
      else
        # success
        mpg123 -f 3000 /home/$USER/VoidConf/assets/success.mp3 > /dev/null 2>&1 
      fi

      ;;
    "run(debug)")
      clear
      cd build/debug
      ./ush 
      cd ../..
      ;;
    "gf2")
      gf2 -nx -ex "source breakpoints.gdb" build/debug/ush
      ;;
    "rr record")
      /usr/bin/rr record build/debug/ush
      ;;
    "gf2 --rr-replay")
      gf2 --rr-replay -x breakpoints.gdb
      ;;
    "clean(debug)")
      echo ">>> cleaning build/debug directory"
      rm -r build/debug/*
      ;;
    "build(release)")
      clear
      echo ">>> creating build/release directory"
      mkdir -p build/release
 
      echo ">>> compiling (release mode)"
      #cc -O3 -Wall -Wextra -pedantic -std=c11 --coverage -lSDL2 -lSDL2_ttf -lm -o build/release/ush lib/util/*.c lib/*.c example/*.c
      cc -O3 -Wall -Wextra -pedantic -std=c11 --coverage -o ./build/release/ush $(pkg-config --cflags notcurses) ./src/*.c $(pkg-config --libs notcurses)

      #echo ">>> transfering assets to build/release"
      #cp assets/*.ttf build/release
      ;;
    "run(release)")
      echo ">>> running ush (release)"
      cd build/release
      ./ush
      cd ../..
      ;;
    "clean(release)")
      echo ">>> cleaning build/release directory"
      rm -r build/release/*
      ;;
    "build(tests)")
      clear
      echo ">>> creating build/tests directory"
      mkdir -p build/tests
 
      # cmock library is here: /usr/local/lib64/
      echo ">>> compiling (tests)"
      cc -g -pg -O0 -DDEBUG -std=c11 --coverage \
        -Wformat=2 -Wall -Wextra -Wpedantic -Wexceptions -fno-exceptions -Wno-unused-parameter \
        -Wshadow -Wwrite-strings -Wstrict-prototypes \
        -Wold-style-definition -Wredundant-decls -Wnested-externs -Wmissing-include-dirs -Wjump-misses-init -Wlogical-op \
        -lc -lcmocka -lmagic -o ./build/tests/usht \
        $(pkg-config --cflags notcurses) ./tests/*.c ./src/linarcx*.c  ./src/linux*.c $(pkg-config --libs notcurses) $(pkg-config --cflags --libs vips)
      ;;
    "run(tests)")
      clear
      cd build/tests
      ./usht
      cd ../..
      ;;
    "gdb(tests)")
      cd build/tests
      gdb --tui usht
      cd ../..
      ;;
    "clean(tests)")
      echo ">>> cleaning build/tests directory"
      rm -r build/tests/*
      ;;
    "doxygen(generate)")
      doxygen
      ;;
    "doxygen(show)")
      ~/software/brave/brave-browser-1.86.142-linux-amd64/brave ./docs/doxygen/html/index.html
      ;;
    "cppcheck")
      rm report/*
      # --suppressions-list=suppress.txt 
      # Suppress a specific warning. The format of <spec> is: [error id]:[filename]:[line].
      # - The [filename] and [line] are optional.
      # - [error id] may be * to suppress all warnings (for a specified file or files).
      # - [filename] may contain the wildcard characters * or ?.
      cppcheck --addon=cppcheck/misra.json --addon=cppcheck/findcasts.json --addon=cppcheck/misc.json --addon=cppcheck/y2038.json --addon=cppcheck/threadsafety.json --inline-suppr --std=c11 --enable=all --error-exitcode=1 --platform=unix64 --report-type=misra-c-2012 -q --xml --xml-version=2 lib/util/*.c lib/*.c example/*.c -I lib/util/ > report/cppcheck.xml 2>&1
      cppcheck-htmlreport --file=report/cppcheck.xml --title="ush" --report-dir=report --source-dir=.
      ;;
    "cppcheck(show)")
      ~/software/brave/brave-browser-1.86.142-linux-amd64/brave ./report/index.html
      #xdg-open report/index.html
      ;;
    "frama-c")
      frama-c -json-compilation-database compile_commands.json example/* lib/* lib/util/*
      ;;
    "frama-c (eva)")
      frama-c -json-compilation-database compile_commands.json example/* lib/* lib/util/* -eva -eva-precision 11
      ;;
    "ivette (eva)")
      ivette -json-compilation-database compile_commands.json example/* lib/* lib/util/* -eva -eva-precision 11
      ;;
    "frama-c-gui (eva)")
      frama-c-gui -json-compilation-database compile_commands.json example/* lib/* lib/util/* -eva -eva-precision 11
      ;;
    "lcov")
      # https://wiki.cs.jmu.edu/student/gcov/start
      ./build/debug/ush
      lcov --capture --directory build/debug --output-file build/debug/coverage.info
      ;;
    "gcovr")
      gcovr build/debug
      ;;
    "genhtml")
      genhtml build/debug/coverage.info --output-directory build/debug/coverage_report
      ;;
    "show lcov report")
      ~/software/brave/brave-browser-1.86.142-linux-amd64/brave ./build/debug/coverage_report/index.html
      # xdg-open unit_test/build/coverage_report/index.html
      ;;
    "kcov(generate)")
      rm -r coverage/*
      kcov coverage/ build/debug/ush
      ;;
    "kcov(show)")
      ~/software/brave/brave-browser-1.86.142-linux-amd64/brave ./coverage/index.html
      # xdg-open coverage/index.html
      ;;
    "llvm-cov")
      ;;
    "valgrind(memcheck)")
      valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --xtree-leak=yes -s -v build/debug/ush
      ;;
    "callgrind")
      valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes -s -v build/debug/ush
      ;;
    "kcachegrind")
      ls callgrind.out.* cachegrind.out.* | fzf --header="kcachgrind: " | xargs kcachegrind
      ;;
    "cachegrind")
      valgrind --tool=cachegrind --cache-sim=yes --branch-sim=yes -s -v build/debug/ush
      ;;
    "helgrind")
      valgrind --tool=helgrind -s -v build/debug/ush
      ;;
    "massif")
      valgrind --tool=massif -s -v build/debug/ush
      ;;
    "ms_print")
      ls massif.out.* | fzf --header="ms_print: " | xargs ms_print
      ;;
    "drd")
      valgrind --tool=drd --trace-fork-join=yes --trace-mutex=yes --trace-semaphore=yes -s -v build/debug/ush
      ;;
    "dhat")
      valgrind --tool=dhat -s -v build/debug/ush
      ;;
    "dhat(cat)")
      ls dhat.out.* | fzf --header="dhat: " | xargs cat | less
      ;;
    "bbv")
      valgrind --tool=exp-bbv -s -v build/debug/ush
      ;;
    "bbv(cat)")
      ls bb.out.* | fzf --header="bbv: " | xargs cat | less
      ;;
    "perf")
      ls build/debug | fzf --header="perf: " | xargs perf stat -d
      ;;
    "uftrace record(test)")
      uftrace --srcline -a --symbols --time --no-libcall --symbols -F __clove_symint___UtilSuite* record build/debug/ush
      ;;
    "uftrace replay(tests)")
      uftrace replay
      ;;
    "uftrace dump(test)")
      uftrace dump --chrome > dump.json
      echo "load dump.json in chrome://tracing/"
      ;;
    "uftrace report(test)")
      uftrace report
      ;;
    "uftrace info(test)")
      uftrace info
      ;;
    "uftrace graph(test)")
      uftrace graph
      ;;
    "uftrace tui(test)")
      uftrace tui
      ;;
    "lttng")
      ;;
    "gprof")
      ;;
    "ldd")
      ls build/*.bin build/obj/* | fzf --header="objdump: " | xargs ldd | less -N
      ;;
    "objdump")
      ls build/*.bin build/obj/* | fzf --header="objdump: " | xargs objdump -x -d -p -f -a -t -r | less -N
      ;;
    "strings")
      ls build/debug/* | fzf --header="strings: " | xargs strings | less -N
      ;;
    "nm")
      ls build/debug/* | fzf --header="nm: " | xargs nm -l -n --synthetic | less -N
      ;;
    "readelf")
      ls build/*.bin build/obj/* | fzf --header="readelf: " | xargs readelf -a -h -g -t -s -d | less -N
      ;;
    "addr2line")
      file=$(ls build/*.bin build/obj/* | fzf --header="Select ELF file for addr2line")
      echo "Enter the memory address:"; read -r addr;
      addr2line -f -a -e $file $addr
      ;;
    "size")
      ls build/*.bin build/obj/* | fzf --header="size: " | xargs size --common -t -d -A | less -N
      ;;
    "file")
      ls build/*.* build/obj/* | fzf --header="file type: " | xargs file
      ;;
    "xxd")
      ls build/*.bin build/*.iso build/obj/* unit_test/build/*.o | fzf --header="xxd: " | xargs xxd | less
      ;;
    "scc")
      scc -p -a -u -i c,h,md,cpp,c++,hpp,txt,json,s,ld
      ;;
    "search")
      read -p "keyword: " p_keyword; rg "$p_keyword" ;;
    "search/replace")
      read -p "to_search: " to_search
      read -p "to_replace: " to_replace
      ambr "$to_search" "$to_replace" ;;
    "elfedit")
      ;;
    "strip")
      ;;
    "objcopy")
      ;;
    "ranlib")
      ;;
    "gcc-ar")
      ;;
    "gcc-nm")
      ;;
    "gcc-ranlib")
      ;;
    "gcov")
      ;;
    "gcov-dump")
      ;;
    "gcov-tool")
      ;;
    "gprof")
      ;;
    "gprofng")
      ;;
    "gprofng-archive")
      ;;
    "gprofng-collect-app")
      ;;
    "gprofng-display-html")
      ;;
    "gprofng-display-src")
      ;;
    "gprofng-display-text")
     ;;
    *) ;;
  esac
}

if [ $# -eq 0 ]; then
  menu
fi

"$@"
