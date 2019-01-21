#ifndef LIBERAD_H
#define LIBERAD_H

#include <libusb-1.0/libusb.h>
#include <iostream>
#include <signal.h>
#include <vector>
#include <atomic>
#include "EradLogger.h"

using namespace std;

#define CONTROL_B 0b11111010
#define LIBERAD_ENDPOINT_OUT (1 | LIBUSB_ENDPOINT_OUT)
#define LIBERAD_ENDPOINT_IN (0x81 | LIBUSB_ENDPOINT_IN)
#define MIN_BUFFER_IN_SIZE 600
#define TRACE_LENGTH 585

/* Signals for changing the operational time window of Oerad hardware */
enum TimeWindow {SHORT = 0b00110001, LONG = 0b00110111};

/* Signals for changing gain levels of Oerad hardware  */
enum Gain {LEVEL1 = 0b00110010, LEVEL2 = 0b00110011, LEVEL3 = 0b00110100, LEVEL4 = 0b00110101, LEVEL5 = 0b00110110};

/* Liberad functions return values */
enum LiberadErrorCodes {LIBERAD_SUCCESS = 1, LIBERAD_ERR = -1, LIBERAD_NOT_INIT = -2, LIBERAD_OERADAR_FIELDS_EMPTY = -3};

/* Function prototype for user defined callback function called on receipt of data from Oerad hardware */
typedef void (*LiberadCallbackIn)(unsigned char* buffer, int length);

/* Function prototype for user defined callback function called on sending data to Oerad hardware */
typedef void (*LiberadCallbackOut)(unsigned char* buffer, int length);

/* Structure representing an Oerad hardware device. Can be handled via liberad functions or directly if further functionality is required. */
class Oeradar{
public:
  enum OeradarState{NO_DEV = -1, ON_BUS = 0, CONNECTED = 1, INIT = 2, TRANSMITTING = 3, RUNNING = 4};
  std::atomic<OeradarState> state{NO_DEV};

  TimeWindow window;
  Gain gain;

  unsigned char* buffer_in = nullptr;
  unsigned char* buffer_out = nullptr;
  int buffer_in_size = 0;
  int buffer_out_size = 0;

  libusb_device* device = nullptr;
  libusb_device_handle* dev_handle = nullptr;

  struct libusb_transfer* transfer_in = nullptr;
  struct libusb_transfer* transfer_out = nullptr;

  void init_transfer_in(LiberadCallbackIn cb, unsigned char* buffer, int buffer_size );
  int setup_single_transfer_in(LiberadCallbackIn user_callback_in, unsigned char* buffer, int buffer_size);
  void init_transfer_out(LiberadCallbackOut user_callback_out, unsigned char* buffer, int buffer_size );

  int register_transfer_out();
  int register_transfer_in();

  void cb_in(struct libusb_transfer* transfer);
  void cb_in_single(struct libusb_transfer* transfer);
  void cb_out(struct libusb_transfer* transfer);
  LiberadCallbackIn user_callback_in;
  LiberadCallbackOut user_callback_out;

  void run();
  void run_single();
  bool wireless;

};


/* Sets logging level of Liberad and libusb */
void liberad_set_logger(LogLevel level);



/* Initializes liberad. libusb context is created. */
int liberad_init(LogLevel lvl);

/* Checks all usb connected devices and populates a vector with hardware-backed Oeradar isntances */
int liberad_get_valid_devices(vector<Oeradar*>* gprs);

/* Prints information about product - id, vendor, interfaces, endpoints, descriptors and addresses */
int liberad_print_device_info(Oeradar* device);


/* Opens a device, removes attached kernel drivers if any and claims interface of Oerad device */
int liberad_connect_to_device(Oeradar* device);

/* Sets up connection parameters - baud rate, stop bits, parity */
int liberad_init_device(Oeradar* device);


/* Sets fields needed for carrying out asynchronous OUT-bound data transfers to Oerad hardware */
int liberad_set_async_out_params(Oeradar* device,
                                  LiberadCallbackOut user_callback_out,
                                  unsigned char* buffer_out,
                                  int out_buffer_size);

/* Sets fields needed for carrying out asynchronous IN-bound data transfers from Oerad hardware. On sending data user_callback_out will be called. */
int liberad_set_async_in_params(Oeradar* device,
                                LiberadCallbackIn user_callback_in,
                                unsigned char* buffer_in,
                                int in_buffer_size);

/* Registers an IN-bound asynchronous data transfer from an Oerad device. On receipt of data user_callback_in will be called. */
int liberad_register_in_handling(Oeradar* device);



/* Sends signals to Oerad hardware to start transmission with the passed TimeWindow and Gain parameters */
int liberad_start_transmission(Oeradar* device,
                          TimeWindow length,
                          Gain level);

/* Sets asynchronous IN and OUT fields needed for asynchronous transmission, starts the device with given params. */
int liberad_start_io_async(Oeradar* device,
                                TimeWindow length,
                                Gain level,
                                LiberadCallbackIn user_callback_in,
                                LiberadCallbackOut user_callback_out,
                                unsigned char* buffer_in,
                                int inLength,
                                unsigned char* buffer_out,
                                int outLength);

/* Sends signals to Oerad hardware to start signal transmission via an asynchronous mechanism. Requires OUT-bound data transfer fields to be set. */
int liberad_start_transmission_async(Oeradar* device,
                                      TimeWindow length,
                                      Gain level);





/* Loop for handling asynchronous IN and OUT transfers. Designed to be submitted to a worker thread. */
int liberad_handle_io_async(Oeradar* device);

/* Stop loop handling asynchronous IN and OUT transfers. */
void liberad_stop_io(Oeradar* device);



/* Gets a single trace sent from the Oerad hardware via a synchronous mechanism. Stores it in buffer_in. */
int liberad_get_current_trace(Oeradar* device, unsigned char* buffer_in, int buffer_size);

/* Gets a single trace sent from the Oerad hardware via an asynchronous mechanism. Stores it in buffer_in and calls user_callback_in on receipt of data. */
int liberad_get_current_trace_async(Oeradar* device, LiberadCallbackIn user_callback_in, unsigned char* buffer, int buffer_size);

/* Gets a single trace sent from the Oerad hardware via an asynchronous mechanism. Requires IN-bound data transfer fields to be set.
* Stores data in buffer_in and calls user_callback_in on receipt of data.
*/
int liberad_get_current_trace_async(Oeradar* device);

/* Sets Oerad hardware to operate in the passed time window length. Sends signal synchronously. */
int liberad_set_time_window(Oeradar* device,
                              TimeWindow length);

/* Sets Oerad hardware to transmit data with passed gain level. Sends signal synchronously.  */
int liberad_set_gain(Oeradar* device,
                          Gain level);

/* Sets Oerad hardware to operate in the passed time window length. Sends signal asynchronously. */
int liberad_set_time_window_async(Oeradar* device, TimeWindow length);

/* Sets Oerad hardware to transmit data with passed gain level. Sends signal asynchronously.  */
int liberad_set_gain_async(Oeradar* device, Gain level);




/* Cancels any pending IN and OUT transfers, releases hardware interface and closes the device. */
int liberad_disconnect_device(Oeradar* device);


/* Releases libusb resources. */
void liberad_exit();



/* Checks if libusb context is created and USB connection can be established. */
bool liberad_check_init();

/* Checks if device is a valid Oerad hardware. */
bool liberad_is_device_valid(libusb_device* device);

/* Iterates over all USB devices attached to system and filters hardware produced by Oerad Tech. */
ssize_t liberad_filter_valid(libusb_device ** connected, ssize_t countAll, vector<Oeradar*> *valid);

/* Sends a signal to Oerad hardware via a synchronous mechanism. */
ssize_t liberad_send_signal_sync(Oeradar* device, unsigned char signal);



#endif
