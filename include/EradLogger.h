#ifndef ERADLOG_H
#define ERADLOG_H

#include <iostream>

enum LogLevel{LIBERAD_DEBUG_2, LIBERAD_DEBUG, LIBERAD_INFO, LIBERAD_WARN, LIBERAD_ERROR, LIBERAD_NONE};

using namespace std;

struct structlog {
    bool headers = false;
    LogLevel level = LIBERAD_WARN;
};

extern structlog LOGCFG;

class Elog {

public:

  Elog(){}
  Elog(LogLevel lvl){
    msglvl = lvl;
    if (LOGCFG.headers){
      operator << ("["+getLabel(lvl)+"]");
    }
  }
  ~Elog(){
    if (opened){
      cout << endl;
    }
    opened = false;
  }
  template<class T>
  Elog &operator<<(const T &msg){
    if (msglvl >= LOGCFG.level){
      cout << msg;
      opened = true;
    }
    return *this;
  }

private:

  bool opened = false;
  LogLevel msglvl = LIBERAD_DEBUG;
  inline string getLabel(LogLevel lvl){
    string label;
    switch (lvl) {
      case LIBERAD_NONE : label = " "; break;
      case LIBERAD_ERROR : label = "ERROR "; break;
      case LIBERAD_DEBUG : label = "DEBUG "; break;
      case LIBERAD_INFO : label = "INFO "; break;
      case LIBERAD_WARN : label = "WARN "; break;
      case LIBERAD_DEBUG_2 : label = "DEBUG2"; break;
    }
    return label;
  }
};


#endif
