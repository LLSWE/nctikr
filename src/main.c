#include <ncurses.h>

WINDOW *gen_border() {
  WINDOW *b = newwin(15, 50, 2, 5);
  return b;
}

void display_table(void *w) {
  box(w, 0, 0);
  wnoutrefresh(w);
}

int main(int argc, char *argv[]) {
  void *stdscr = initscr();
  keypad(stdscr, 0);
  start_color();



  refresh();
  getch();

  endwin();
  return 0;
}
