#include "liberad/liberad.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <string>


#define IN_BUFFER_SIZE 1024*8

using namespace std;

static uint8_t in_buffer[IN_BUFFER_SIZE];

Oeradar* active_gpr;
bool running;

void user_input();

int main(){

  liberad_init(LIBERAD_DEBUG);
  std::vector<Oeradar*> valid_devices;
  int count = liberad_get_valid_devices(&valid_devices);

  if (count <= 0){
    cout << "No valid devices." << endl;
    liberad_exit();
    return 1;
  }

  active_gpr = valid_devices.at(0);
  liberad_connect_to_device(active_gpr);
  liberad_init_device(active_gpr);

  liberad_start_transmission(active_gpr, LONG, LEVEL2);


  thread user_input_thread(user_input);

  running = true;
  user_input_thread.join();

  return 0;

}


// Simple user input mechanism for controlling device state & lifecycle.
void user_input(){
  while (running){
    string temp;
    while (getline(cin, temp)){
      if (temp == "s"){
        liberad_set_time_window(active_gpr, SHORT);
      }
      if (temp == "l"){
        liberad_set_time_window(active_gpr, LONG);
      }
      if (temp == "1"){
        liberad_set_gain(active_gpr, LEVEL1);
      }
      if (temp == "2"){
        liberad_set_gain(active_gpr, LEVEL2);
      }
      if (temp == "3"){
        liberad_set_gain(active_gpr, LEVEL3);
      }
      if (temp == "4"){
        liberad_set_gain(active_gpr, LEVEL4);
      }
      if (temp == "5"){
        liberad_set_gain(active_gpr, LEVEL5);
      }
      if (temp == "t"){
        liberad_get_current_trace(active_gpr, in_buffer, IN_BUFFER_SIZE);
      }
      if (temp == "q"){
        liberad_disconnect_device(active_gpr);
        liberad_exit();
        break;
      }
    }
  }
}
