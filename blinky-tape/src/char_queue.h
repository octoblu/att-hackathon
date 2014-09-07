#ifndef _CHAR_QUEUE
#define _CHAR_QUEUE

#include "char.h"

class CharQueue {
private:
  int mSize;
  Char *mHead;
  Char *mTail;

public:
  CharQueue() {
    mSize = 0;
    mHead = new Char();
    mTail = mHead;
  }

  void clear() {
    while(this->pop()) {}
    mTail = mHead;
  }

  char pop() {
    Char *realHead = mHead->get_next();
    char character = '\0';

    if(realHead) {
      character = realHead->get_character();
      mHead->set_next(realHead->get_next());
      delete realHead;
      mSize--;
    }

    return character;
  }

  void push(char character) {
    Char *next = new Char(character);
    mTail->set_next(next);
    mTail = next;

    mSize++;
  }

  int size() {
    return mSize;
  }
};

#endif /* _CHAR_QUEUE */
