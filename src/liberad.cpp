#include "../include/liberad.h"
#include <string.h>
// #include "EradLogger.h"

libusb_context *context = nullptr;
libusb_device **devs = nullptr;
structlog LOGCFG = {};

/* Wrapper for callback passed to libusb_fill_bulk_transfer for INbound transfers */
void LIBUSB_CALL callback_wrapper_in(struct libusb_transfer* transfer){
    Oeradar *radar = reinterpret_cast<Oeradar*>(transfer->user_data);
    radar->cb_in(transfer);
}


/* Wrapper for callback passed to libusb_fill_bulk_transfer for OUTbound transfers */
void LIBUSB_CALL callback_wrapper_out(struct libusb_transfer* transfer){
  Oeradar* radar = reinterpret_cast<Oeradar*>(transfer->user_data);
  radar->cb_out(transfer);
}




/* Initializes Libusb context and sets the logging level.
* @param LogLevel sets the logging level of both liberad and libusb
* LIBERAD_NONE : sets libusb to LIBUSB_LOG_LEVEL_NONE
* LIBERAD_ERROR : sets libusb to LIBUSB_LOG_LEVEL_ERROR
* LIBERAD_WARN : sets libusb to LIBUSB_LOG_LEVEL_WARNING
* LIBERAD_INFO : sets libusb to LIBUSB_LOG_LEVEL_INFO
* LIBERAD_DEBUG : sets libusb to LIBUSB_LOG_LEVEL_INFO
* LIBERAD_DEBUG_2 : sets libusb to LIBUSB_LOG_LEVEL_DEBUG
* @return LIBERAD_SUCCESS if libusb successfully initialized
*         LIBERAD_ERR if libusb not initialized
*/
int liberad_init(LogLevel lvl){

  int r = libusb_init(&context);
  liberad_set_logger(lvl);

  Elog(LIBERAD_DEBUG) << "libusb_init() = " << r;
  if (r < 0){
    Elog(LIBERAD_ERROR) << "Error initializing libusb context";
    return LIBERAD_ERR;
  }
  Elog(LIBERAD_INFO) << "Liberad successfully init";
  return LIBERAD_SUCCESS;
}



/* Sets logging level of liberad. If liberad_init not called beforehand libusb
* will not be set. Subsequent calls to liberad_init will override this value.
* @param LogLevel level
* LIBERAD_NONE : sets libusb to LIBUSB_LOG_LEVEL_NONE
* LIBERAD_ERROR : sets libusb to LIBUSB_LOG_LEVEL_ERROR
* LIBERAD_WARN : sets libusb to LIBUSB_LOG_LEVEL_WARNING
* LIBERAD_INFO : sets libusb to LIBUSB_LOG_LEVEL_INFO
* LIBERAD_DEBUG : sets libusb to LIBUSB_LOG_LEVEL_INFO
* LIBERAD_DEBUG_2 : sets libusb to LIBUSB_LOG_LEVEL_DEBUG
*/
void liberad_set_logger(LogLevel lvl){
  LOGCFG.headers = true;
  LOGCFG.level = lvl;

  if (!liberad_check_init()) Elog(LIBERAD_WARN) << "Libusb not init";

  switch (LOGCFG.level) {
    case LIBERAD_NONE : libusb_set_debug(context, LIBUSB_LOG_LEVEL_NONE); break;
    case LIBERAD_ERROR : libusb_set_debug(context, LIBUSB_LOG_LEVEL_ERROR); break;
    case LIBERAD_WARN : libusb_set_debug(context, LIBUSB_LOG_LEVEL_WARNING); break;
    case LIBERAD_INFO : libusb_set_debug(context, LIBUSB_LOG_LEVEL_INFO); break;
    case LIBERAD_DEBUG : libusb_set_debug(context, LIBUSB_LOG_LEVEL_INFO); break;
    case LIBERAD_DEBUG_2 : libusb_set_debug(context, LIBUSB_LOG_LEVEL_DEBUG); break;
  }
  Elog(LIBERAD_DEBUG) << "Liberad Logging Level: " << lvl;
}

/* Checks if libusb is initialized
* @return true if libusb is attached to a libusb_context
* false if libusb is not init
*/
bool liberad_check_init(){
  if (!context){
    Elog(LIBERAD_ERROR) << "Liberad not init";
    return false;
  }
  return true;
}



// -------------------------------------------------------------------------------------------------

/* Callback invoked when new data from GPR is available for processing.
* This function is called internally and is not designed to be exposed to
* the user of liberad. The function calls a user defined LiberadCallbackIn function.
* @param libusb_transfer* transfer
*/
void LIBUSB_CALL Oeradar::cb_in(libusb_transfer* transfer){

  int r = libusb_submit_transfer(this->transfer_in);

  Elog(LIBERAD_DEBUG_2) << "cb_in submit transfer: " << r;

  user_callback_in(transfer->buffer, transfer->actual_length);

}



/* Callback invoked when data is transfered from user to GPR.
* Function is called internally and is not designed to be exposed to
* users of liberad. Oeradar::cb_out calls a user defined LiberadCallbackOut function.
* @param libusb_transfer* transfer
*/
void LIBUSB_CALL Oeradar::cb_out(libusb_transfer* transfer){

  user_callback_out(transfer->buffer, transfer->actual_length);

}

/* Initializes the current inscance of Oeradar fields for IN transfers.
* Must be called before Oeradar::register_transfer_in()
* @param LiberadCallbackIn cb - user defined function called when new data is available from GPR
* @param unsigned char* buffer - user allocated buffer to store incoming data from GPR
* @param int buffer_size - size of user buffer
*/
void Oeradar::init_transfer_in(LiberadCallbackIn cb, unsigned char* buffer, int buffer_size){

  if (buffer_size < MIN_BUFFER_IN_SIZE) Elog(LIBERAD_WARN) << "In buffer too small, may cause errors";

  this->user_callback_in = cb;
  this->buffer_in_size = buffer_size;
  this->buffer_in = buffer;
}

/* Allocates a libusb_transfer, fills its field params and submits it
* Must call init_transfer_in(LiberadCallbackIn, unsigned char*, int) first to populate libusb_transfer struct
* @return LIBERAD_ERR if unsuccessful libusb_submit_transfer
* @return LIBERAD_SUCCESS if successfully libusb_submit_transfer
*/
int Oeradar::register_transfer_in(){

  this->transfer_in = libusb_alloc_transfer(0);

  libusb_fill_bulk_transfer(transfer_in, dev_handle, LIBERAD_ENDPOINT_IN, buffer_in, buffer_in_size, callback_wrapper_in, this, 0);

  int r = libusb_submit_transfer(transfer_in);

  Elog(LIBERAD_DEBUG) << "Libusb submit in transfer: " << r;

  if (r != 0){
    Elog(LIBERAD_ERROR) << "Could not submit in trasnfer.";
    return LIBERAD_ERR;
  }

  Elog(LIBERAD_INFO) << "Registered in transfer";
  return LIBERAD_SUCCESS;
}

/* Sets this instance's fields for OUT transfers. Must be called before register_transfer_out()
* @param LiberadCallbackOut cb - user defined function called when successfully sending data to GPR
* @param unsigned char* buffer - user defined buffer for storing OUT signals
* @param int buffer_size - size of user defined buffer
* @return
*/
void Oeradar::init_transfer_out(LiberadCallbackOut cb, unsigned char* buffer, int buffer_size){
  this->user_callback_out = cb;
  this->buffer_out = buffer;
  this->buffer_out_size = buffer_size;
}

/* Allocates a libusb_transfer for outgoing signals, sets its fields and submits the transfer.
* Must be called after a call to init_transfer_out(LiberadCallbackOut, unsigned char*, int)
* @return LIBERAD_ERR on unsuccessful libusb_transfer submission
* @return LIBERAD_SUCCESS on successful libusb_transfer submission
*/
int Oeradar::register_transfer_out(){

  this->transfer_out = libusb_alloc_transfer(0);
  libusb_fill_bulk_transfer(transfer_out, dev_handle, LIBERAD_ENDPOINT_OUT, buffer_out, 1, callback_wrapper_out, this, 0 );
  int r = libusb_submit_transfer(transfer_out);
  Elog(LIBERAD_DEBUG) << "Libusb submit out transfer: " << r;
  if (r != 0){
    Elog(LIBERAD_ERROR) << "Could not submit out trasnfer.";
    return LIBERAD_ERR;
  }

  Elog(LIBERAD_INFO) << "Registered out transfer";
  return LIBERAD_SUCCESS;
}

/* Handles libusb transfer events.
* Called by liberad_run_connection_async(Oeradar* radar) to be run on a separate
* execution thread.
*/
void Oeradar::run(){

  state = RUNNING;
  while(state == RUNNING){
    int r = libusb_handle_events_completed(context, NULL);
    if (r < 0){
      Elog(LIBERAD_ERROR) << "Libusb error handling events: " << r;
      state = TRANSMITTING;
      break;
    }
  }
}

/* Handles libusb transfer events and is called by liberad_get_current_trace_async().
*/
void Oeradar::run_single(){
  for (int i = 0; i<5; i++){
    Elog(LIBERAD_DEBUG) << "run single";
    int r = libusb_handle_events_completed(context, NULL);
    if (r < 0){
      Elog(LIBERAD_ERROR) << "Libusb error handling events: " << r;
    }
  }
}


// -------------------------------------------------------------------------------------------------



/* Gets a list of all USB devices, sorts valid Oeradar devices and populates a vector
* object with pointers to valid Oeradar devices with set libusb_device* fields.
* @param std::vector<Oeradar*> devices - the list to populate with valid Oeradar devices
* @return int count - number of valid Oeradar devices
*/
int liberad_get_valid_devices(vector<Oeradar*>* devices){

  if (!liberad_check_init()) return LIBERAD_NOT_INIT;

  libusb_device **devs;
  int countAll = libusb_get_device_list(context, &devs);
  int count = 0;
  count = liberad_filter_valid(devs, countAll, devices);

  if (count <= 0){
    Elog(LIBERAD_WARN) << "No valid Oerad devices found";
    return LIBERAD_ERR;
  }

  Elog(LIBERAD_INFO) << "Found " << count << "valid Oerad devices";
  return count;

}

/* If device is valid, opens and sets the libusb_device_handle* field of the Oeradar instance.
* If a kernel driver is attached to the device, it is detached. The Oeradar usb interface is claimed.
* @param Oeradar* device - pointer to an Oeradar instance
* @return LIBERAD_SUCCESS on successfully opening and claiming the interface of the Oeradar instance. Note
* that this will be returned evein if GPR has not been powered up.
* @return LIBERAD_ERR if interface can't be claimed.
*/
int liberad_connect_to_device(Oeradar* radar){

  if (!liberad_check_init()) return LIBERAD_NOT_INIT;

  if (radar->state < Oeradar::ON_BUS){
    Elog(LIBERAD_ERROR) << "Oeradar object not associated with address on USB bus.";
    return LIBERAD_ERR;
  }

  int r = 0;

  r = libusb_open(radar->device, &radar->dev_handle);
  if (r == 0) Elog(LIBERAD_INFO) << "Successfully opened device";
  Elog(LIBERAD_DEBUG) << "libusb_open: " << r;

  if(libusb_kernel_driver_active(radar->dev_handle, 0) == 1) {

     Elog(LIBERAD_DEBUG) << "Kernel Driver Active" ;
     r = libusb_detach_kernel_driver(radar->dev_handle, 0);
     Elog(LIBERAD_DEBUG) << "Libusb detach kernel driver: " << r;

     if (r != 0){
       Elog(LIBERAD_ERROR) << "Error detaching kernel driver: " << r;
     }

     Elog(LIBERAD_DEBUG) << "Kernel Driver Detached";

  }
  Elog(LIBERAD_DEBUG) << "Kernel driver not active";

  r = libusb_claim_interface(radar->dev_handle, 0);
  if (r < 0){
    Elog(LIBERAD_DEBUG) << "Can't claim interface " << r;
    return LIBERAD_ERR;
  }
  Elog(LIBERAD_INFO) << "Successfully claimed interface";

  radar->state = Oeradar::CONNECTED;
  return LIBERAD_SUCCESS;

}

//done
int liberad_print_device_info(Oeradar* radar){

  if (radar->state < Oeradar::ON_BUS){
    Elog(LIBERAD_ERROR) << "Oeradar object not associated with address on USB bus";
    return LIBERAD_ERR;
  }

  libusb_device_descriptor desc;
  int r = libusb_get_device_descriptor(radar->device, &desc);
  if (r < 0){
    cout << "error getting device descriptor" <<endl;
    return 1;
  }
  cout<<"Number of possible configurations: "<<(int)desc.bNumConfigurations<<"  "<<endl;
  cout<<"Device Class: "<<(int)desc.bDeviceClass<<"  "<<endl;
  cout<<"VendorID: "<<desc.idVendor<<"  " <<endl;
  cout<<"ProductID: "<<desc.idProduct<<endl;

  libusb_config_descriptor *config;
  libusb_get_config_descriptor(radar->device, 0, &config);

  cout << "num of interfaces: " << (int)config->bNumInterfaces <<endl;
  const libusb_interface *inter;
  const libusb_interface_descriptor *interdesc;
  const libusb_endpoint_descriptor *epdesc;
  for (int i = 0; i < (int)config->bNumInterfaces; i++){

    inter = &config->interface[i];
    cout<<"Number of alternate settings: "<<inter->num_altsetting<< endl;

    for (int j = 0; j<inter->num_altsetting; j++ ){

      interdesc = &inter->altsetting[j];
      cout<<"Interface Number: "<<(int)interdesc->bInterfaceNumber<< endl;
      cout<<"Number of endpoints: "<<(int)interdesc->bNumEndpoints<< endl;

      cout << endl ;

      for(int k=0; k<(int)interdesc->bNumEndpoints; k++) {
        epdesc = &interdesc->endpoint[k];
        cout<<"Descriptor Type: "<<(int)epdesc->bDescriptorType<<endl;
        cout<<"EP Address: "<<(int)epdesc->bEndpointAddress<< endl;
        cout << endl ;
      }

      cout << endl;

    }

    cout << endl;
  }

  cout << endl << endl << endl;
  libusb_free_config_descriptor(config);
  return 0;
  }

/* Sends control signals to Oeradar device for successful data transfer. Control signals
* enable UART, sets the modem handshaking, sets baud rate divisor, sets the baud rate and line control.
* @param Oeradar* device - pointer to device for initalization
* @return LIBERAD_SUCCESS on successful device initialization. Note that this will be returned evein if GPR has
* not been powered up.
*/
int liberad_init_device(Oeradar* device){

    if (device->state < Oeradar::CONNECTED){
      Elog(LIBERAD_ERROR) << "Oeradar device not connected. Need to call to liberad_connect_to_device";
      return LIBERAD_ERR;
    }

    int32_t baudRate = 115200;
    unsigned char *baud = new unsigned char[4];
    baud[0] = baudRate & 0xff;
    baud[1] = (baudRate >> 8) & 0xff;
    baud[2] = (baudRate >> 16) & 0xff;
    baud[3] = (baudRate >> 24) & 0xff;
    int r;
    r = libusb_control_transfer(device->dev_handle, 0x41, 0x00, 0x0001, 0, NULL, 0, 5000);
    Elog(LIBERAD_DEBUG) << "Enable UART: " << r;
    r = libusb_control_transfer(device->dev_handle, 0x41, 0x07, 0x303, 0, NULL, 0, 5000);
    Elog(LIBERAD_DEBUG) << "Set modem handshaking: " << r;
    r = libusb_control_transfer(device->dev_handle, 0x41, 0x01, 0x20, 0, NULL, 0, 5000);
    Elog(LIBERAD_DEBUG) << "Set baud rate divisor: " << r;
    r = libusb_control_transfer(device->dev_handle, 0x41, 0x1E, 0, 0, baud, 4, 5000);
    Elog(LIBERAD_DEBUG) << "Set baud rate: " << r;
    r = libusb_control_transfer(device->dev_handle, 0x41, 0x03, 0x0800, 0, NULL, 0, 5000);
    Elog(LIBERAD_DEBUG) << "Set line control: " << r;

    device->state = Oeradar::INIT;
    // TODO check each step for errors
    return LIBERAD_SUCCESS;
}

/* Sets this Oeradar instance to transmit a signal with a Gain level and at a TimeWindow lenght.
* No callback is registered here and no data is obtained from the GPR.
* @param Oeradar* device - pointer to device to power up and set
* @param TimeWindow lenght - GPR operational time window (SHORT or LONG)
* @param Gain level - hardware gain level {LEVEL1, LEVEL2, LEVEL3, LEVEL4 or LEVEL5}
* @return LIBERAD_ERR if device has not been initialized or failure to send parameter signals
* @return LIBERAD_SUCCESS on successfully starting up the GPR. Note that this will be returned evein if GPR has
* not been powered up.
*/
int liberad_start_transmission(Oeradar* device, TimeWindow length, Gain level){

  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  liberad_send_signal_sync(device, length);
  liberad_send_signal_sync(device, level);
  if (liberad_send_signal_sync(device, length) == LIBERAD_SUCCESS &&
      liberad_send_signal_sync(device, level) == LIBERAD_SUCCESS){
        Elog(LIBERAD_INFO) << "Transmission started";
        device->state = Oeradar::TRANSMITTING;
        return LIBERAD_SUCCESS;
  }
  return LIBERAD_ERR;

}

/* Sets this Oeradar instance to transmit via an asynchronous mechanism. Requires that all fields
* relating to asynchronous communication are set - user_callback_in, user_callback_out, buffer_in, buffer_out,
* buffer_in_size and buffer_out_size. This can be done via a call to liberad_set_async_in_params() and
* liberad_set_async_out_params().
* @param Oeradar* device - pointer to active device instance. Needs to be INIT
* @param TimeWindow length - operational time window of GPR (SHORT or LONG)
* @param Gain level - hardware gain level {LEVEL1, LEVEL2, LEVEL3, LEVEL4 or LEVEL5}
* @return LIBERAD_ERR if device not init
* @return LIBERAD_OERADAR_FIELDS_EMPTY if Oeradar instance fields for asynchronous communication not set
* @return LIBERAD_SUCCESS on successful start of transmission. Note that this will be returned evein if GPR has
* not been powered up.
*/
int liberad_start_transmission_async(Oeradar* device, TimeWindow length, Gain level){

  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  if (device->buffer_out_size == 0 || device->buffer_out == nullptr){
    Elog(LIBERAD_ERROR) << "Buffers for outgoing data not set";
    return LIBERAD_OERADAR_FIELDS_EMPTY;
  }

  if (liberad_set_time_window_async(device, length) != LIBERAD_SUCCESS &&
      liberad_set_gain_async(device, level) != LIBERAD_SUCCESS){
        return LIBERAD_ERR;
  }

  device->state = Oeradar::TRANSMITTING;
  return LIBERAD_SUCCESS;

}


//if getting data only once and storing it in different location. Technically the in
/*
* Technically cb_in, buffer and buffer_size params will be stored int he Oeradar instance so
* Subsequent calls may omit these params. This is in case each call to get current trace requires a
* different parameter.
*/
/* Gets a single trace from Oeradar instance via an asynchronous mechanism.
* @param Oeradar* device - pointer to device
* @param LiberadCallbackIn cb - user defined function to be called when data from GPR is received
* @param unsigned char* buffer_in - user allocated buffer to store incoming data
* @param int buffer_size - size of user allocated buffer
* @return LIBERAD_SUCCESS on successful asynchronous transfer setup.
*/
int liberad_get_current_trace_async(Oeradar* device, LiberadCallbackIn cb_in, unsigned char* buffer, int buffer_size){

  if (device->state < Oeradar::TRANSMITTING){
    Elog(LIBERAD_ERROR) << "Device not started transmission. Need to call liberad_start_transmission";
    return LIBERAD_ERR;
  }

  device->init_transfer_in(cb_in, buffer, buffer_size);
  if( device->register_transfer_in() != LIBERAD_SUCCESS) return LIBERAD_ERR;

  device->run_single();
  return LIBERAD_SUCCESS;

}

//if data destination has already been set
int liberad_get_current_trace_async(Oeradar* device){

  if (device->state < Oeradar::TRANSMITTING){
    Elog(LIBERAD_ERROR) << "Device not started transmission. Need to call liberad_start_transmission";
    return LIBERAD_ERR;
  }

  if (device->buffer_in_size == 0 || device->buffer_in == nullptr){
    Elog(LIBERAD_ERROR) << "Buffers for incoming data not set";
    return LIBERAD_OERADAR_FIELDS_EMPTY;
  }

  if (device->register_transfer_in() != LIBERAD_SUCCESS) return LIBERAD_ERR;
  device->run_single();
  return LIBERAD_SUCCESS;
}





/* Sets the passed Oeradar instance's fields for outgoing asynchronous communication
* @param Oeradar* device - pointer to device
* @param LiberadCallbackOut cb_out - user defined function to be called on successful OUT signal passing
* @param unsigned char* buffer_out - pointer to user allocated buffer to store outgoing signal
* @param int out_buffer_size - size of user allocated buffer
* @return LIBERAD_ERR if device has not been initialized
* @return LIBERAD_SUCCESS else
*/
int liberad_set_async_out_params(Oeradar* device,
                                  LiberadCallbackOut cb_out,
                                  unsigned char* buffer_out,
                                  int out_buffer_size){

  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  device->init_transfer_out(cb_out, buffer_out, out_buffer_size);
  return LIBERAD_SUCCESS;
}


/* Sets the passed Oeradar instance's fields for outgoing asynchronous communication
* @param Oeradar* device - pointer to device
* @param LiberadCallbackOut cb_in - user defined function to be called when new data is available from GPR
* @param unsigned char* buffer_in - pointer to user allocated buffer to store incoming data
* @param int in_buffer_size - size of user allocated buffer
* @return LIBERAD_ERR if device has not been initialized
* @return LIBERAD_SUCCESS else
*/
int liberad_set_async_in_params(Oeradar* device,
                                LiberadCallbackIn cb_in,
                                unsigned char* buffer_in,
                                int in_buffer_size){

  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  device->init_transfer_in(cb_in, buffer_in, in_buffer_size);
  return LIBERAD_SUCCESS;

}

/*
*/
int liberad_register_in_handling(Oeradar* device){

  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  if (device->buffer_in_size == 0 || device->buffer_in == nullptr){
    Elog(LIBERAD_ERROR) << "Buffers for incoming data not set";
    return LIBERAD_OERADAR_FIELDS_EMPTY;
  }

  return device->register_transfer_in();

}



/* Sets this Oeradar instance to transmit with the given parameters and initializes an
* asynchronous mechanism for handling incoming and outgoing data.
* @param Oeradar* device - device instance to start. Must be on USB bus and initialized.
* @param TimeWindow lenght - operational time window of GPR {SHORT or LONG}
* @param Gain level - hardware gain level {LEVEL1, LEVEL2, LEVEL3, LEVEL4 or LEVEL5}
* @param LiberadCallbackIn callback_in - user defined function to be called on incoming data from GPR
* @param LiberadCallbackOut callback_out - user defined function to be called on outgoing signals to GPR
* @param unsigned char* buffer_in - pointer to buffer to store incoming data. Must be at least MIN_BUFFER_IN_SIZE
* @param int inLenght - size of buffer_in
* @param unsigned char* buffer_out - pointer to buffer to store outgoing signals.
* @param int outLength - size of buffer_out
* @return LIBERAD_ERR if Oeradar instance has not been initialized
* @return LIBERAD_SUCCESS if successfully started an asynchronous transmission. Note that this will be returned evein if GPR has
* not been powered up.
*/
int liberad_start_io_async(Oeradar* device,
                                TimeWindow length,
                                Gain level,
                                LiberadCallbackIn callback_in,
                                LiberadCallbackOut callback_out,
                                unsigned char* buffer_in,
                                int inLength,
                                unsigned char* buffer_out,
                                int outLength){


  if (device->state < Oeradar::INIT){
    Elog(LIBERAD_ERROR) << "Device not initialized. Needs a call to liberad_init_device(Oeradar*)";
    return LIBERAD_ERR;
  }

  device->init_transfer_out(callback_out, buffer_out, outLength);
  if (liberad_set_time_window_async(device, length) != LIBERAD_SUCCESS &&
      liberad_set_gain_async(device, level) != LIBERAD_SUCCESS){ return LIBERAD_ERR; }

  device->init_transfer_in(callback_in, buffer_in, inLength);
  if( device->register_transfer_in() != LIBERAD_SUCCESS) return LIBERAD_ERR;

  device->state = Oeradar::TRANSMITTING;

  return LIBERAD_SUCCESS;

}








int liberad_handle_io_async(Oeradar* device){

  if (device->state < Oeradar::TRANSMITTING){
    Elog(LIBERAD_ERROR) << "Device not started transmission. Need to call liberad_start_transmission_async";
    return LIBERAD_ERR;
  }

  device->run();
  return LIBERAD_SUCCESS;

}

void liberad_stop_io(Oeradar* device){
  Elog(LIBERAD_INFO)<< "Stopped connection";
  device->state = Oeradar::TRANSMITTING;
}

/* Gets a single trace synchronously by the pointed Oeradar instance.
* @param Oeradar* device - pointer to device
* @param unsigned char* buffer_in - pointer to user allocated buffer to store incoming data
* @param int bufferLenght - size of user allocated buffer.
* @return LIBERAD_ERR if device not started TRANSMITTING
* @return number of transfered bytes. Should be 585 as of January 2019.
*/
int liberad_get_current_trace(Oeradar* device, unsigned char* buffer_in, int bufferLength){

  if (device->state < Oeradar::TRANSMITTING){
    Elog(LIBERAD_ERROR) << "Device not started transmission. Need to call liberad_start_transmission";
    return LIBERAD_ERR;
  }

  int actual;
  bool ok_signal = false;
  for (int i = 0; i < 5; i++){
    libusb_bulk_transfer(device->dev_handle, (0x81 | LIBUSB_ENDPOINT_IN), buffer_in, bufferLength, &actual, 600);
    if (actual == TRACE_LENGTH) ok_signal = true;
  }
  if (ok_signal) return TRACE_LENGTH;

  return actual;
}





/* Sets the passed Oeradar instance's operational time window via an asynchronous mechanism
* @param Oeradar* device - pointer to device
* @param TimeWindow length - operational time window signal of GPR. Can be SHORT or LONG.
* @return LIBERAD_SUCCESS on successful transfer registration.
*/
int liberad_set_time_window_async(Oeradar* device, TimeWindow length){

  memcpy(device->buffer_out, &length, 1 );
  device->window = length;
  return device->register_transfer_out();

}


/*Sets the passed Oeradar instance's hardware gain level via an asynchronous mechanism. Fills the buffer
* pointed to by Oeradar::buffer_out with gain command to be sent to device. The pointer has to be set beforehand
* with liberad_start_transmission_async() or liberad_set_async_out_params().
* @param Oeradar* device - pointer to device instance
* @param Gain level - gain signal
* @return LIBERAD_ERR on unsuccessful libusb_transfer submission
* @return LIBERAD_SUCCESS on successful libusb_transfer submission
*/
int liberad_set_gain_async(Oeradar* device, Gain level){
  memcpy(device->buffer_out, &level, 1 );
  device->gain = level;
  return device->register_transfer_out();

}


/* The time window signal is sent to the passed instance of Oeradar via a synchronous mechanism.
* @param Oeradar* device - pointer to device instance
* @param TimeWindow lenght - signal for time window length - SHORT or LONG
* @return LIBERAD_ERR if no data was passed to GPR
* @return LIBERAD_SUCCESS on successful OUT signal transmission
*/
int liberad_set_time_window(Oeradar* device, TimeWindow length){
  device->window = length;
  return liberad_send_signal_sync(device, length);

}


/* The gain level signal is sent to the passed instance of Oeradar via a synchronous mechanism.
* @param Oerdar* device - pointer to device instance
* @param Gain level - signal for gain level - LEVEL1, LEVEL2, LEVEL3, LEVEL4 or LEVEL5
* @return LIBERAD_ERR if no data was passed to GPR
* @return LIBERAD_SUCCESS on successful OUT signal transmission
*/
int liberad_set_gain(Oeradar* device, Gain level){
  device->gain = level;
  return liberad_send_signal_sync(device, level);

}



/* Cancels any pending in or out asynchronous transfers, releases the device interface & closes
* the connection with it. Oeradar instance is still listed on the USB BUS i.e. device->libusb_device
* pointer still points to the device.
* @param Oeradar* device - pointer to device instance
* @return LIBERAD_ERR if error in releasing the interface
* @return LIBERAD_SUCCESS on successful releasing.
*/
int liberad_disconnect_device(Oeradar* device){
  if (device->transfer_in){
    int r = libusb_cancel_transfer(device->transfer_in);
    Elog(LIBERAD_DEBUG) << "Cancel transfer in: " << r;
  }

  if (device->transfer_out){
    int r = libusb_cancel_transfer(device->transfer_out);
    Elog(LIBERAD_DEBUG) << "Cancel transfer out: " << r;

  }

  int r = libusb_release_interface(device->dev_handle, 0);
  Elog(LIBERAD_DEBUG) << "Release interface: " << r;
  if (r != 0)  return LIBERAD_ERR;

  Elog(LIBERAD_INFO) << "Released Interface";
  libusb_close(device->dev_handle);

  device->state = Oeradar::ON_BUS;
  return LIBERAD_SUCCESS;
}

/* Frees the libusb device lists and exits with this context.
*/
void liberad_exit(){

  libusb_free_device_list(devs, 1);
  libusb_exit(context);

  Elog(LIBERAD_INFO) << "Liberad exit & freed resources";
}


// -------------------------------------------------------------------------------------------------

/* Iterates through libusb_device** list and if a device is an Oerad GPR an Oeradar object is created.
* A user created vector object is filled with all valid devices. This function is designed for
* internal use.
* @param libusb_device** connected - all USB devices obtained by libusb
* @param ssize_t countAll - number of all USB devices
* @param vector<Oeradar*>* valid - pointer to a vector of Oeradar pointers.
* @return the number of valid devices
*/
ssize_t liberad_filter_valid(libusb_device ** connected, ssize_t countAll, vector<Oeradar*> *valid){

  valid->reserve(countAll);
  for (int i = 0; i< countAll; i++){
    if (liberad_is_device_valid(connected[i])){
      Oeradar* radar = new Oeradar();
      radar->device = connected[i];
      radar->state = Oeradar::ON_BUS;
      valid->push_back(radar);
    }
  }
  valid->shrink_to_fit();
  return valid->size();
}

/* Checks if device is an Oerad GPR
* @param libusb_device* device - USB device
* @return true if valid else false
*/
bool liberad_is_device_valid(libusb_device* device){

  libusb_device_descriptor desc;
  int r = libusb_get_device_descriptor(device, &desc);
  Elog(LIBERAD_DEBUG) << "Libusb get device descriptor: " << r;

  if (desc.idVendor == 4292 && (desc.idProduct == 60000 ||
                                desc.idProduct == 0x8A9F ||
                                desc.idProduct == 0x8AA0 ||
                                desc.idProduct == 0x8AA1 ||
                                desc.idProduct == 0x8AA2)){
    return true;
  }
  return false;
}


/* A synchronous OUT mechanism for sending signals to Oerad GPR
* @param Oeradar* device - pointer to Oerad device
* @param unsigned char* signal - signal to send - for Oerad GPR each command or signal is a single byte long
* @return the amount of transferred bytes
*/
ssize_t liberad_send_signal_sync(Oeradar* device, unsigned char signal){

  if (device->state < Oeradar::INIT) {
    Elog(LIBERAD_ERROR) << "Device not initialized";
    return LIBERAD_ERR;
  }

  int actual = 0;
  int r = libusb_bulk_transfer(device->dev_handle, LIBERAD_ENDPOINT_OUT, &signal, 1, &actual, 400 );

  Elog(LIBERAD_DEBUG) << "Libusb bulk transfer signal: " << signal << " to device result: " << r;
  if (actual == 0){
    Elog(LIBERAD_ERROR) << "Error sending signal to device";
    return LIBERAD_ERR;
  }

  return LIBERAD_SUCCESS;
}
