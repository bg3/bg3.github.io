/*  Pipi
 *  Static site generator for Park Imminent.
 *  B. Grayland
 *  2020/05/10
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

/* defining this creates pages that point to ../style.css, as opposed to /style.css */
//#define LOCAL_BUILD 1;

#define PI_WHITE "#fffff9"
#define PI_NEWWH "#f7ffff"
#define PI_BLACK "#111111"
#define PI_GREY  "#d1d1d1"
#define PI_RED   "#ef5541"

#define PI_RUNES      " \"+=->#*'|/"
#define PI_PARAGRAPH  ' '
#define PI_QUOTE      '\"'
#define PI_IMAGE      '+'
#define PI_HEADING    '='
#define PI_SUBHEADING '-'
#define PI_CODE       '>'
#define PI_ORDERED    '#'
#define PI_UNORDERED  '*'
#define PI_TABLE      '|'
#define PI_COMMENT    '\''
#define PI_DATE       '/'

typedef enum { OFF, TABLE, CODE, ORDERED, UNORDERED } block_mode;

/* structs */

typedef struct page page;
typedef struct line line;
typedef struct date date;

struct date {
  int year;
  int month;
  int day;
};

struct page {
  char*   title;
  char*   filename;
  char*   section;
  date*   created;
  date*   updated;
  page*   parent;
  line*   lines;
  int     numlines;
};

struct line {
  char  type;
  char* text;
};

int string_to_filename(char* str, char* fn) {
  int i = 0;
  int last_char_was_dash = 0;

  for (int j = 0; j < (int) strlen(str); j++) {
    char char_to_write = '\0';
    if (isalnum(str[j])) {
      char_to_write = str[j];
      last_char_was_dash = 0;
    } else if (str[j] == ' ') {
      if (!last_char_was_dash) {
        char_to_write = '-';
        last_char_was_dash = 1;
      }
    }

    if (char_to_write) {
      fn[i] = tolower(char_to_write);
      i++;
    }
  }
  fn[i] = '\0';
  return i;
}

/* html output */

#ifdef LOCAL_BUILD

  void html_header(FILE* f, char line[]) {
    fprintf(f, "<!DOCTYPE html>\n <html lang=\"en\">\n <head>\n <title>%s</title>\n <link rel=\"stylesheet\" type=\"text/css\" href=\"../style.css\"> </head>\n <body>\n", line);
  }
#else
  void html_header(FILE* f, char line[]) {
    fprintf(f, "<!DOCTYPE html>\n <html lang=\"en\">\n <head>\n <title>%s</title>\n <link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\"> </head>\n <body>\n", line);
  }

#endif

void html_banner(FILE* f) {
  fprintf(f, "<header><h2><a href=\"/index.html\">Park</br>Imminent</a></h2></header>");
}

void html_title(FILE* f, char line[]) {
  fprintf(f, "<h1>%s</h1>\n", line);
}

void html_footer(FILE* f) {
  fprintf(f, "</body>\n</html>");
}

void html_para(FILE* f, char text[]) {
  fprintf(f, "<p>%s</p>\n", text);
}

void html_quote(FILE* f, char text[]) {
  fprintf(f, "<blockquote>%s</blockquote>\n", text);
}

void html_heading(FILE* f, char text[]) {
  fprintf(f, "<h2>%s</h2>\n", text);
}

void html_subheading(FILE* f, char text[]) {
  fprintf(f, "<h3>%s</h3>\n", text);
}

/* HTML table */

int count_table_columns(char header_row[]) {
  int cols = 1;
  size_t len = strlen(header_row);

  for (size_t i = 0; i < len; i++) {
    if (header_row[i] == '|' && header_row[i + 1] != '\0') {
      cols++;
    }
  }

  return cols;
}

void html_table_start(FILE *f) {
  fprintf(f, "<table>\n");
}

void html_table_row(FILE* f, char text[], int cols) {

  /*
   * cols =
   *           1         2
   * 0123456789012345678901
   * cell1 | cell2 | cell3\0
   * cell1 | cell2 |\0
   * cell1 |\0
   *
   * col_start = 0
   * col_end   = 6
   * print 0-6
   *
   * at last column? write the remaining text into that column.
   * else
   *  check length of remaining string - nonzero?
   *    calc col_end
   *      col_end = null
   *        write remaining string to cell
   *      else
   *        write from col_start to col_end into td
   *        col_start = col_end
   *  else
   *    write an empty cell
   */

  fprintf(f, "<tr>");

  int current_col = 0;
  char* col_start = text;
  char* col_end = strchr(text, '|');

  while (current_col < cols) {
    fprintf(f, "<td>");
    for (; col_start < col_end; col_start++) {
      fprintf(f, "%c", col_start[0]);
    }
    col_start = col_end + 1;
    col_end = strchr(col_start, '|');
    fprintf(f, "</td>");
    current_col++;
  }

  fprintf(f, "</tr>");
}

void html_table_end(FILE* f) {
  fprintf(f, "</table>\n");
}


void html_ordered_list_start(FILE* f) {
  fprintf(f, "<ol>\n");
}

void html_unordered_list_start(FILE* f) {
  fprintf(f, "<ul>\n");
}

void html_list_item(FILE* f, char* text) {
  fprintf(f, "<li>%s</li>\n", text);
}

void html_ordered_list_end(FILE* f) {
  fprintf(f, "</ol>\n");
}

void html_unordered_list_end(FILE* f) {
  fprintf(f, "</ul>\n");
}

void html_code_start(FILE* f) {
  fprintf(f, "<pre><code>");
}

void html_code_end(FILE* f) {
  fprintf(f, "</code></pre>\n");
}

char* convert_links(char* raw) {
  /* :: Link text: http://wwww.xyz.html */
  /* ::: http:///www.xyz.txt for no text. */
  char* current = raw;
  for ( ; *current != '\0'; current++) {
    if (current[0] == ':' && current[1] == ':') {
      if (current[2] == ':') {
        current += 4;
        printf("OO");
        // link only, no link text
      } else {
        char* link_start = current;
        char* text_start = current + 3;
        char* text_end = text_start;
        while (*text_end != '\0' && *text_end != ':')
          text_end++;

        char* url_start = text_end + 2;
        char* url_end = url_start;
        while (*url_end != '\0' && *url_end != ' ')
          url_end++;

        char text[text_end - text_start + 1];
        char url[url_end - url_start + 1];

        strncpy(text, text_start, text_end - text_start + 1);
        text[text_end - text_start] = '\0';

        strncpy(url, url_start, url_end - url_start + 1);
        url[url_end - url_start] = '\0';

        size_t link_len = strlen(text) + strlen(url) + 15;
        char link[link_len];
        sprintf(link, "<a href=\"%s\">%s</a>", url, text);

        size_t pre = link_start - raw;
        size_t end = strlen(url_end + 1);
        char* replacement_string = malloc(pre + link_len + end + 1);
        strncpy(replacement_string, raw, pre);
        strcat(replacement_string, link);
        strcat(replacement_string, url_end + 1);
        raw = replacement_string;
        current = raw;
      }
    }
  }
  return raw;
}


void html_img(FILE* f, char text[]) {
  int image_number;
  int n;
  //  need to check if sscanf failed - n would be uniitialised otherwise.
  //  also, figcaption may not be well supported....
  sscanf(text, "%i %n", &image_number, &n);
  char* caption = text+n;
  fprintf(f, "<figure>\n<img src=\"..\\img\\%i.png\" />\n<figcaption>%s</figcaption>\n</figure>\n", image_number, caption);
}

void unhandled_line(FILE* f, char text[]) {
  fprintf(f, "<p style=\"background-color: #ef5541; color: #111111\">%s</p>\n", text);
}

char* remove_section_from_title(char* section, char* title) {
  char* split_point;
  split_point = strchr(title, '\\');
  if (split_point) {
    section = realloc(section, sizeof(char) * (split_point - title));
    memcpy(section, title, split_point - title);
    section[split_point - title] = '\0';
    title = split_point + 1;
  }
  return title;
}
     

int remove_char(char* str, char c) {
   int removed = 0;
   char* tmp;

   while (*str) {
     tmp = strchr(str, c);
     if (NULL == tmp) {
       break;
     } else {
       size_t len = strlen(tmp + 1);
       memmove(tmp, tmp + 1, len);
       tmp[len] = 0;
       ++removed;
       str = tmp;
     }
   }
   return removed;
}

char* p_getline(FILE* in) {
  char buffer[10];
  char* input = 0;
  size_t cur_len = 0;

  while (fgets(buffer, sizeof(buffer), in)) {
    size_t buf_len = strlen(buffer);
    char* extra = realloc(input, buf_len + cur_len + 1);
    if (extra == 0)
      break;
    input = extra;
    strcpy(input + cur_len, buffer);
    cur_len += buf_len;
    if (input[cur_len - 1] == '\n')
      break;
  }

  return input;
}

int compare_dates(date* d1, date* d2) {
  /* 1 means d1 is greater */
  if (d1->year > d2->year) {
    return 1;
  } else if (d1->year < d2->year) {
    return -1;
  } else {
    if (d1->month > d2->month) {
      return 1;
    } else if (d1->month < d2->month) {
      return -1;
    } else {
      if (d1->day > d2->day) {
        return 1;
      } else if (d1->day < d2->day) {
        return -1;
      } else {
        return 0;
      }
    }
  }
}

int compare_page_dates(const void* a, const void* b) {
  const page* p1 = a;
  const page* p2 = b;
  date p1_date = *(p1->created);
  date p2_date = *(p2->created);
  if (p1->updated != NULL)
    p1_date = *(p1->updated);
  if (p2->updated != NULL)
    p2_date = *(p2->updated);

  return compare_dates(&p2_date, &p1_date);
}

void parse_date(char* text, page* p) {
  /* / yyyy-mm-dd, yyyy-mm-dd */
  size_t len = strlen(text);
  date* created = malloc(sizeof(date)); 
  date* updated = malloc(sizeof(date));
  *created = (date) { 0, 0, 0 };
  *updated = (date) { 0, 0, 0 };
  if (len == 10) {
    /* single date */
    sscanf(text, "%d-%d-%d", &(created->year), &(created->month), &(created->day));
    p->created = created;
  } else if (len == 22) {
    sscanf(text, "%d-%d-%d, %d-%d-%d", &(created->year), &(created->month), &(created->day), &(updated->year), &(updated->month), &(updated->day));
    p->updated = updated;
    p->created = created;
  } else {
    printf("Invalid date (/) found in %s: \n\t%s\n", p->title, text);
  }
}

void add_line_to_page(line l, page* p) {
  p->numlines++;
  p->lines = realloc(p->lines, sizeof(line) * p->numlines);
  p->lines[p->numlines - 1] = l;
}

int generate_page(FILE* out, page* p) {
  html_header(out, p->title);
  html_banner(out);
  html_title(out, p->title);

  block_mode mode = OFF;

  int table_cols = 0;
  int n = 0;
  for (int j = 0; j < p->numlines; j++) {
    line l = p->lines[j];
    n++;

    if (mode == TABLE && l.type != PI_TABLE) {
      html_table_end(out);
      mode = OFF;
      table_cols = 0;
    }

    if (mode == ORDERED && l.type != PI_ORDERED) {
      html_ordered_list_end(out);
      mode = OFF;
    }

    if (mode == UNORDERED && l.type != PI_UNORDERED) {
      html_unordered_list_end(out);
      mode = OFF;
    }

    if (mode == CODE && l.type != PI_CODE) {
      html_code_end(out);
      mode = OFF;
    }

    l.text = convert_links(l.text);

    switch(l.type) {
      case PI_PARAGRAPH:
        html_para(out, l.text);
        break;
      case PI_QUOTE:
        html_quote(out, l.text);
        break;
      case PI_HEADING:
        html_heading(out, l.text);
        break;
      case PI_SUBHEADING:
        html_subheading(out, l.text);
        break;
      case PI_COMMENT:
        continue;
      case PI_IMAGE:
        html_img(out, l.text);
        break;
      case PI_TABLE:
        if (mode != TABLE) {
          mode = TABLE;
          table_cols = count_table_columns(l.text);
          html_table_start(out);
        }
        html_table_row(out, l.text, table_cols);
        break;
      case PI_ORDERED:
        if (mode != ORDERED) {
          mode = ORDERED;
          html_ordered_list_start(out);
        }
        html_list_item(out, l.text);
        break;
      case PI_UNORDERED:
        if (mode != UNORDERED) {
          mode = UNORDERED;
          html_unordered_list_start(out);
        }
        html_list_item(out, l.text);
        break;
      case PI_CODE:
        if (mode != CODE) {
          mode = CODE;
          html_code_start(out);
        }
        fprintf(out, "%s\n", l.text);
        break;     
      default:
        unhandled_line(out, l.text);
        break;
    }
  }
  html_footer(out);
  return n;
}

int generate_pages(page* pages, int page_count) {
  int n = 0;
  for (int i = 0; i < page_count; i++) {
   char* fn;
    asprintf(&fn, "../site/%s.html", pages[i].filename);
    FILE* out = fopen(fn, "w");
    n += generate_page(out, &pages[i]);
   }
  return n;
}

void generate_index(page* pages, int page_count) {
  FILE* out = fopen("../index.html", "w");
  html_header(out, "Park Imminent");
  fprintf(out, "<header><h1>Park</br>Imminent</br></h1></header>");
  fprintf(out, "<ul class=\"index_by_date\">");
  for (int i = 0; i < page_count; i++) {
    if (pages[i].updated == NULL) {
      fprintf(out, "<li><h2><a href=\"./site/%s.html\">%s</a></h2><h3>%s</h3><time>%d-%02d-%02d</time></li>\n", pages[i].filename, pages[i].title, pages[i].section, pages[i].created->year, pages[i].created->month, pages[i].created->day);
    } else {
      fprintf(out, "<li><h2><a href=\"./site/%s.html\">%s <em>(updated)</em></a></h2><h3>%s</h3><time>%d-%02d-%02d</time></li>\n", pages[i].filename, pages[i].title, pages[i].section, pages[i].updated->year, pages[i].updated->month, pages[i].updated->day);
    }
  }
  fprintf(out, "</ul>");
  html_footer(out);
}
    

void parse_file(FILE* file, page** p_pages, int* page_count) {
  char* rawline = 0;

  while( (rawline = p_getline(file)) ) {
    char rune = 0;
    int  n = 0;
    sscanf(rawline, "%c %n", &rune, &n);
    remove_char(rawline, '\n');
    char* text = rawline + n;

    if (rune == '\n') {
      continue;
    } else if (!strchr(PI_RUNES, rune)) {
      /* must be the title of a new page */
      *page_count = *page_count + 1;
      *p_pages = realloc(*p_pages, sizeof(page) * *page_count);

      int len = (int) strlen(rawline) + 1;
      char* section = malloc(1);
      rawline = remove_section_from_title(section, rawline); 
      char* fn = (char*) malloc(len * sizeof(char));
      string_to_filename(rawline, fn);
      page* p = *p_pages;
      int address = *page_count - 1;
      p[address] = (page) { .title = rawline, .section = section, .filename = fn, .parent = NULL, .lines = NULL, .numlines = 0 };
    } else if (rune == PI_DATE) {
      parse_date(text, *p_pages + *page_count - 1); 
    } else {
      line l = { .type = rune, .text = text };
      add_line_to_page(l, *p_pages + *page_count - 1);
    }
  }
}

/* init */

int main() {

  char* source_directory = "../data/";
  DIR* dir = opendir(source_directory);

  struct dirent* entry;
  int files = 0;

  page* raw_pages = NULL;

  int num_pages = 0;

  while ((entry = readdir(dir))) {

    if (entry->d_name[entry->d_namlen - 3] == '.' &&
        entry->d_name[entry->d_namlen - 2] == 'p' &&
        entry->d_name[entry->d_namlen - 1] == 'i') {
      files++;
      size_t filename_length = strlen(entry->d_name) + strlen(source_directory);
      char filename[filename_length + 1];
      strcpy(filename, source_directory);
      strcat(filename, entry->d_name);
      FILE* f = fopen(filename, "r");
      parse_file(f, &raw_pages, &num_pages);
    }
  }
  /* close the FILE */

  qsort(raw_pages, num_pages, sizeof(page), compare_page_dates); 

  generate_pages(raw_pages, num_pages);
  generate_index(raw_pages, num_pages);

  return EXIT_SUCCESS;
}
