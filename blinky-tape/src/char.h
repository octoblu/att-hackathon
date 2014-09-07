#ifndef _CHAR
#define _CHAR

#include <stdio.h>

class Char {
private:
  bool last;
  char character;
  Char *next;

public:
  Char() {
    last = true;
    next = NULL;
    character = '\0';
  }

  Char(char pcharacter) {
    last = true;
    character = pcharacter;
    next = 0;
  }

  char get_character() {
    return character;
  }

  Char *get_next() {
    return next;
  }

  void set_next(Char *pnext) {
    last = false;
    next = pnext;
  }

  bool is_last() {
    return last;
  }
};

#endif /* _CHAR */
