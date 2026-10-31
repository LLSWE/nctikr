#include <ctype.h>
#include <curl/curl.h>
#include <data.h>
#include <locale.h>
#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define COLOR_DEF 1
#define COLOR_HIGHER 2
#define COLOR_LOWER 3
#define COLOR_HEADER 4
#define COLOR_INPUT 5

void draw_table(BrapiTrackerJsonResponse *data, int scroll_offset,
                int max_visible_rows) {
  clear();

  attron(COLOR_PAIR(COLOR_HEADER) | A_BOLD);
  mvprintw(0, (COLS - 32) / 2, "TERMONITOR - B3 MARKET DATA");
  attroff(COLOR_PAIR(COLOR_HEADER) | A_BOLD);

  int col_ticker = 0;
  int w_ticker = 8;

  int col_name = col_ticker + w_ticker + 2;
  int w_name = (int)(COLS * 0.28);
  if (w_name < 10)
    w_name = 10;

  int col_type = col_name + w_name + 2;
  int w_type = 8;

  int col_sector = col_type + w_type + 2;
  int w_sector = (int)(COLS * 0.18);
  if (w_sector < 10)
    w_sector = 10;

  int col_price = col_sector + w_sector + 2;
  int w_price = 12;

  int col_var = col_price + w_price + 2;
  int w_var = 10;

  int col_mcap = col_var + w_var + 2;
  int w_mcap = 16;

  attron(A_REVERSE | A_BOLD);
  mvprintw(2, col_ticker, "%-*s", w_ticker, "TICKER");
  mvprintw(2, col_name, "%-*s", w_name, "COMPANY");
  mvprintw(2, col_type, "%-*s", w_type, "TYPE");
  mvprintw(2, col_sector, "%-*s", w_sector, "SECTOR");
  mvprintw(2, col_price, "%*s", w_price, "PRICE (R$)");
  mvprintw(2, col_var, "%*s", w_var, "VAR (%)");
  mvprintw(2, col_mcap, "%*s", w_mcap, "MARKET CAP");
  attroff(A_REVERSE | A_BOLD);

  int current_screen_line = 3;
  for (int i = 0; i < max_visible_rows; i++) {
    int asset_idx = scroll_offset + i;
    if (asset_idx >= (int)data->results_count)
      break;

    BrapiResults *asset = &data->results[asset_idx];

    mvprintw(current_screen_line, col_ticker, "%-*s", w_ticker,
             asset->symbol ? asset->symbol : "N/A");

    char truncated_name[256];
    snprintf(truncated_name, w_name + 1, "%s",
             asset->name ? asset->name : "N/A");
    mvprintw(current_screen_line, col_name, "%-*s", w_name, truncated_name);

    mvprintw(current_screen_line, col_type, "%-*s", w_type,
             asset->assetType ? asset->assetType : "N/A");

    char truncated_sector[256];
    snprintf(truncated_sector, w_sector + 1, "%s",
             asset->sector ? asset->sector : "N/A");
    mvprintw(current_screen_line, col_sector, "%-*s", w_sector,
             truncated_sector);

    mvprintw(current_screen_line, col_price, "%12.2f", asset->quote.lastPrice);

    if (asset->quote.changePercent > 0) {
      attron(COLOR_PAIR(COLOR_HIGHER) | A_BOLD);
      mvprintw(current_screen_line, col_var, "%+10.2f%%",
               asset->quote.changePercent);
      attroff(COLOR_PAIR(COLOR_HIGHER) | A_BOLD);
    } else if (asset->quote.changePercent < 0) {
      attron(COLOR_PAIR(COLOR_LOWER) | A_BOLD);
      mvprintw(current_screen_line, col_var, "%10.2f%%",
               asset->quote.changePercent);
      attroff(COLOR_PAIR(COLOR_LOWER) | A_BOLD);
    } else {
      mvprintw(current_screen_line, col_var, "%10.2f%%", 0.00);
    }

    if (asset->quote.marketCap > 0) {
      mvprintw(current_screen_line, col_mcap, "%16lld", asset->quote.marketCap);
    } else {
      mvprintw(current_screen_line, col_mcap, "%16s", "N/A");
    }

    current_screen_line++;
  }

  attron(COLOR_PAIR(COLOR_HEADER));
  mvprintw(LINES - 1, 0,
           " [ARROWS] Navigate | [/] Search | [Q] Quit | Assets: %d/%zu",
           scroll_offset + (data->results_count > 0 ? 1 : 0),
           data->results_count);
  clrtoeol();
  attroff(COLOR_PAIR(COLOR_HEADER));

  refresh();
}

char *open_search_dialog() {
  static char buffer[32];
  memset(buffer, 0, sizeof(buffer));

  echo();
  curs_set(1);

  attron(COLOR_PAIR(COLOR_INPUT) | A_BOLD);
  mvprintw(LINES - 1, 0, " Enter ticker (e.g. PETR4): ");
  clrtoeol();
  attroff(COLOR_PAIR(COLOR_INPUT) | A_BOLD);

  int pos = 0;
  int ch;

  while (1) {
    ch = getch();
    if (ch == '\n' || ch == KEY_ENTER) {
      break;
    } else if (ch == 27) {
      buffer[0] = '\0';
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
      if (pos > 0) {
        pos--;
        buffer[pos] = '\0';
        mvprintw(LINES - 1, 28 + pos, " ");
        move(LINES - 1, 28 + pos);
      }
    } else if (isalnum(ch) && pos < (int)sizeof(buffer) - 1) {
      buffer[pos++] = (char)toupper(ch);
      buffer[pos] = '\0';
    }
  }

  noecho();
  curs_set(0);

  if (strlen(buffer) == 0)
    return NULL;
  return buffer;
}

int main() {
  setlocale(LC_ALL, "");

  curl_global_init(CURL_GLOBAL_ALL);

  BrapiTrackerJsonResponse *data = fetch_brapi_dataset();
  if (!data || data->results_count == 0) {
    printf("Failed to initialize or retrieve dataset from API.\n");
    curl_global_cleanup();
    return 1;
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(COLOR_DEF, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_HIGHER, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_LOWER, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_HEADER, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_INPUT, COLOR_YELLOW, COLOR_BLACK);
  }

  int scroll_offset = 0;
  int ch;

  while (1) {
    int max_visible_rows = LINES - 4;
    if (max_visible_rows < 1)
      max_visible_rows = 1;

    draw_table(data, scroll_offset, max_visible_rows);

    ch = getch();

    if (ch == 'q' || ch == 'Q') {
      break;
    } else if (ch == KEY_DOWN) {
      if (scroll_offset + max_visible_rows < (int)data->results_count) {
        scroll_offset++;
      }
    } else if (ch == KEY_UP) {
      if (scroll_offset > 0) {
        scroll_offset--;
      }
    } else if (ch == '/' || ch == 'f' || ch == 'F') {
      char *query = open_search_dialog();
      if (query && strlen(query) > 0) {
        BrapiTrackerJsonResponse *new_result = fetch_brapi_specific(query);
        if (new_result && new_result->results_count > 0) {
          free_brapi_response(data);
          data = new_result;
          scroll_offset = 0;
        } else {
          if (new_result)
            free_brapi_response(new_result);
          attron(COLOR_PAIR(COLOR_LOWER) | A_BOLD);
          mvprintw(LINES - 1, 0,
                   " Ticker '%s' not found! Press any key to continue...",
                   query);
          clrtoeol();
          attroff(COLOR_PAIR(COLOR_LOWER) | A_BOLD);
          getch();
        }
      }
    } else if (ch == KEY_RESIZE) {

      resizeterm(0, 0);
      clear();
    }
  }

  endwin();
  free_brapi_response(data);
  curl_global_cleanup();

  return 0;
}
