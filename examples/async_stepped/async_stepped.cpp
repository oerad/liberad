#include "liberad/liberad.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <string>


#define IN_BUFFER_SIZE 1024*8
#define OUT_BUFFER_SIZE 8*8

using namespace std;

static uint8_t in_buffer[IN_BUFFER_SIZE];
static uint8_t out_buffer[OUT_BUFFER_SIZE];

Oeradar* active_gpr;
bool running;

void callback_in(unsigned char* buffer, int received);
void callback_out(unsigned char* buffer, int sent);
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

/*
*This function may be broken down in three other functions depending on your use case flow:
* liberad_set_async_in_params(active_gpr, callback_in, in_buffer, IN_BUFFER_SIZE); + liberad_register_in_handling(active_gpr);
* liberad_set_async_out_params(active_gpr, callback_out, out_buffer, OUT_BUFFER_SIZE);
* liberad_start_transmission_async(active_gpr, SHORT, LEVEL1);
*/
  liberad_start_io_async(active_gpr, SHORT, LEVEL1, callback_in, callback_out, in_buffer, IN_BUFFER_SIZE, out_buffer, OUT_BUFFER_SIZE);

  thread user_input_thread(user_input);
  thread gpr_connection_thread(liberad_handle_io_async, active_gpr);

  running = true;
  user_input_thread.join();
  gpr_connection_thread.join();

  return 0;

}

/* This is typedefed as LiberadCallbackIn - must return void and take two params.
* This is called when new data is available from GPR
* @param unsigned char* buffer - pointer to user allocated buffer
* @param int received - num of bytes received
*/
void callback_in(unsigned char* buffer, int received){
  cout << "received: " << received << " bytes" << endl; // this may block your console for input. Best is to feed this to file, some drawing funct or some other stream.
}

/* This is typedefed as LiberadCallbackOut - must return void and take two params
* This is called when a signal is sent to GPR
* @param unsigned char* buffer - pointer to user allocated buffer
* @param int received - num of bytes received
*/
void callback_out(unsigned char* buffer, int sent){
  cout << "sent: " << sent << " bytes" << endl;
}

// Simple user input mechanism for controlling device state & lifecycle.
void user_input(){
  while (running){
    string temp;
    while (getline(cin, temp)){
      if (temp == "s"){
        liberad_set_time_window_async(active_gpr, SHORT);
      }
      if (temp == "l"){
        liberad_set_time_window_async(active_gpr, LONG);
      }
      if (temp == "1"){
        liberad_set_gain_async(active_gpr, LEVEL1);
      }
      if (temp == "2"){
        liberad_set_gain_async(active_gpr, LEVEL2);
      }
      if (temp == "3"){
        liberad_set_gain_async(active_gpr, LEVEL3);
      }
      if (temp == "4"){
        liberad_set_gain_async(active_gpr, LEVEL4);
      }
      if (temp == "5"){
        liberad_set_gain_async(active_gpr, LEVEL5);
      }
      if (temp == "q"){
        running = false;
        liberad_stop_io(active_gpr);
        liberad_disconnect_device(active_gpr);
        liberad_exit();
        break;
      }
    }
  }
}
