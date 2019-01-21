# liberad

1.  [Introduction](#introduction)
2.  [Installation](#installation)
3.  [Structure](#structure)
4.  [Workflow](#workflow)
5.  [Examples](#examples)
	 
### Introduction
	 
Liberad is an open source C++ 11 library for interfacing with Oerad GPR/WPR devices. It is intended for developers, students and researchers who wish to get raw radar data and/or develop data analysis tools themselves. Liberad is built on top of libusb and offers synchronous and asynchronous communication mechanisms, full control over the radar and basic logging functionality. It is licensed under GPLv3 and later.

### Installation
	
Current installation instructions are for Unix systems. Future releases will include support for Windows. We're using cmake for easy portability.

1. Download or clone the project to your system.
2. [Get CMake](https://cmake.org/install/).
3. open a terminal in the download location or cd to it

		cd 'download/location/of/liberad'
		
4. Generate a folder to store your build files, for example:

		mkdir _build
	
5. Generate your build files. 

		cmake -H -B._build -G "Unix Makefiles"

6. Install liberad

		cd _build
		sudo make install
			
7. To use in your project add this to your header file: 

		#include <liberad/liberad.h>
		
Make sure to link the library in your project's CMakeLists file. More detailed examples can be found in the examples folder.  

### Structure

##### Gain
There are five levels of gain that can be applied to the transmission of the GPR - LEVEL1, LEVEL2, LEVEL3, LEVEL4 and LEVEL5. This enum encodes the signals that are sent to the Oerad device to switch between the gain levels.

##### TimeWindow
As of January 2019 each Oerad device has only two operational time windows - hence the notation SHORT and LONG. For each device these translate to different time windows:
- Dipolo      - SHORT = 75ns, LONG = 150ns
- Scudo       - SHORT = 50ns, LONG = 100ns
- Concretto   - SHORT = 7.5ns, LONG = 15ns

The TimeWindow enum encodes the signals sent to Oerad devices to set between either operational time window.

For further information on what are gain and time windows, you can refer to https://www.oerad.eu/tech.

##### LiberadErrorCodes
There are three exit codes from most liberad functions:
- `LIBERAD_ERR = -1`,
- `LIBERAD_SUCCESS = 1`,
- `LIBERAD_NOT_INIT = -2`.

##### LiberadCallbackIn
A function prototype called when incoming data from an Oerad device is available. The user defines it and then passes it to a liberad method that sets it for an instance of an Oerad device.
```c++
typedef void (*LiberadCallbackIn)(unsigned char* buffer, int length);
```
For example
```c++
void callbackIn(unsigned char* buffer, int transfered){
	cout << "received: " << transfered << endl;
}
```
is a valid user-defined callback function.

##### LiberadCallbackOut
A function prototype, called when outgoing data to an Oerad device has been transmitted. The user defines it and then passes it to a liberad method that sets it for an instance of an Oerad device.
```c++
typedef void (*LiberadCallbackOut)(unsigned char* buffer, int length);
```
For example
```c++
void callbackOut(unsigned char* buffer, int sent){
	cout << "sent: " << sent << endl;
}
```
is a valid user-defined callback function.

These can be passed to an Oeradar instance with
```c++
int liberad_start_io_async(Oeradar* device, TimeWindow length, Gain level, LiberadCallbackIn user_callback_in, LiberadCallbackOut user_callback_out, unsigned char* buffer_in, int inLength, unsigned char* buffer_out, int outLength);
```
```c++
int liberad_set_async_out_params(Oeradar* device, LiberadCallbackOut user_callback_out, unsigned char* buffer_out, int out_buffer_size);
```
or
```c++
int liberad_set_async_in_params(Oeradar* device, LiberadCallbackIn user_callback_in, unsigned char* buffer_in, int in_buffer_size);
```
##### Oeradar
An Oeradar is a virtual abstraction of a physical Oerad device. It:
- Holds the current state of a connected physical device
- Provides access to libusb fields for developers who need additional functionality
- Holds pointers to buffers for incoming and outgoing data
- Holds a user defined callback functions for incoming and outgoing data

To get an instance of Oeradar backed by a physical device, the user needs to create a `std::vector<Oeradar*>` object and pass it to `liberad_get_valid_devices(vector<Oeradar*>* gprs)`. All created Oeradar instances are initialized with `libusb_device* device` pointing to the libusb field and `Oeradar::state = ON_BUS`. In future updates a more list-agnostic approach is planned.


##### Oeradar state
OeradarState holds the current state of the Oeradar instance from its creation, whether backed by a physical device or not.
- `NO_DEV` - an instance of Oeradar is created but it is not associated to a physical device connected to the host.
- `ON_BUS` - an instance of Oeradar is created and it is associated with a physical device connected to the host. `libusb_device* device` pointer is set.
- `CONNECTED` - the physical device is opened, kernel drivers (if any) have been detached from it and its interface has been claimed successfully. The device is now ready for control signal transmission.
- `INIT` - the connection control parameters have been successfully sent to the physical device(baud rate, parity, stop bits) and it is ready for data transmission.
- `TRANSMITTING` - the Oerad physical device is transmitting with the set `TimeWindow` and `Gain` parameters.
- `RUNNING` - an asynchronous communication mechanism is running, preferably on a user created thread as not to stall other operations.

##### libusb fields
The user needn't worry about libusb fields and details but if they wish they could access `libusb` functions via `libusb_device* device` and `libusb_device_handle* dev_handle`.
- `libusb_device* device` - a pointer to a `libusb` structure representing a USB device detected on the system. For more information visit http://libusb.sourceforge.net/api-1.0/index.html
- `libusb_device_handle* dev_handle` - a pointer to a `libusb` structure representing a handle on a USB device. For more information visit http://libusb.sourceforge.net/api-1.0/index.html
- `struct libusb_transfer* transfer_in`  
- `struct libusb_transfer* transfer_out`  

##### Buffers
Two buffers need to be allocated by the user - one for incoming data - `unsigned char* buffer_in` and one for outgoing data `unsigned char* buffer_out`. Usually for a wired connection incoming trace data is in packets of 585 bytes. This 585 byte packet represents a single quantized trace and is available every 55ms. Sometimes, however, the hardware may produce a trace twice as long so this needs to be accounted for when allocating space for the buffer. Outgoing signals are usually one byte long. For wireless connections (via the Oerad USB dongle) the trace data is divided up in packets of different sizes. This will be reflected in future updates of Liberad.

##### liberad_ functions
Most liberad functions take as a parameter an instance of Oeradar and handle `libusb` commands internally so the user doesn't need to be bothered with particularities of USB connectivity. Users are free to access Oeradar libusb-related fields and methods directly.

### Workflow
The basic workflow is: 
1. Initialize the library 
```c++
int liberad_init(LogLevel lvl);
```
2. Get a list of pointers to all valid devices 
```c++
int liberad_get_valid_devices(vector<Oeradar*>* devs);`
```
3. Connect to a device
```c++
int liberad_connect_to_device(Oeradar* device);
```
4. Setup the device for data transfers
```c++
int liberad_init_device(Oeradar* device);
```
5. Transmit data between your system and the Oerad device. This can be done synchronously and asynchronously.

	 - Synchronous:
	 	 - Startup the device with work parameters (TimeWindow and Gain):
		 ```c++
		 int liberad_start_transmission(Oerdar* device, TimeWindow length, Gain level);
		 ```
		 - Handle data
		 ```c++
		 int liberad_get_current_trace(Oeradar* device, unsigned char* buffer_in, int buffer_size);
		 int liberad_set_time_window(Oeradar* device, TimeWindow length);
		 int liberad_set_gain(Oeradar* device, Gain level);
		 ```
	 - Asynchronous
	   - Set fields needed for asynchronous communication
		 ```c++
		 int liberad_set_async_out_params(Oeradar* device, LiberadCallbackOut user_callback_out, unsigned char* buffer_out, int out_buffer_size);
		 int liberad_set_async_in_params(Oeradar* device, LiberadCallbackIn user_callback_in, unsigned char* buffer_in, int in_buffer_size);									
		 ```
		 - Register the asynchrnous data transfers
		 - Handle data transfers (usually on a new worker thread) `std::thread worker(liberad_handle_io_async, active_gpr)` 
		 ```c++
		 int liberad_handle_io_async(Oeradar* device);
		 ```
		 - Transfer data 
		 ```c++
		 int liberad_get_current_trace_async(Oeradar* device);
		 int liberad_set_time_window_async(Oeradar* device, TimeWindow length);
		 int liberad_set_gain_async(Oeradar* device, Gain level);
		 ```
		 - Stop asynchronous data handling
		 ```c++
		 void liberad_stop_io(Oeradar* device);
		 ```
6. Disconnect from the device
```c++
int liberad_disconnect_device(Oeradar* device);
```
7. Quit libusb and free resources
```c++
void liberad_exit();
```

### Examples
There are three examples: 
- A synchronous stepped data transfer mechanism
- An asynchronous continous data transfer mechanism
- An asynchronous stepped data transfer mechanism 

These can be found in the Examples folder 

