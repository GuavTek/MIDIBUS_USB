/**
 * \file
 *
 * \brief USB host driver for Communication Device Class interface.
 *
 * Copyright (c) 2012-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#ifndef _UHI_AUDIO_H_
#define _UHI_AUDIO_H_

#include "conf_usb_host.h"
#include "usb_protocol_audio.h"
#include "uhc.h"
#include "uhi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup uhi_audio_group
 * \defgroup uhi_audio_group_uhc Interface with USB Host Core (UHC)
 *
 * Define and functions required by UHC.
 * 
 * @{
 */

//! Global define which contains standard UHI API for UHC
//! It must be added in USB_HOST_UHI define from conf_usb_host.h file.
#define UHI_AUDIO { \
	.install = uhi_audio_install, \
	.enable = uhi_audio_enable, \
	.uninstall = uhi_audio_uninstall, \
	.sof_notify = uhi_audio_sof, \
}

/**
 * \name Functions required by UHC
 * See \ref uhi_api_t for the function descriptions
 * @{
 */
uhc_enum_status_t uhi_audio_install(uhc_device_t* dev);
void uhi_audio_enable(uhc_device_t* dev);
void uhi_audio_uninstall(uhc_device_t* dev);
void uhi_audio_sof(bool b_micro);
//@}

//@}

/**
 * \ingroup uhi_group
 * \defgroup uhi_audio_group UHI for Communication Device Class
 *
 * Common APIs used by high level application to use this USB host class.
 * These routines are used by memory to transfer its data
 * to/from USB CDC endpoint.
 *
 * See \ref uhi_audio_quickstart.
 * @{
 */

/**
 * \brief Open a port of UHI CDC interface
 *
 * \param port          Communication port number
 * \param configuration Pointer on port configuration
 *
 * \return \c true if the port is available
 */
bool uhi_audio_open(uint8_t port, usb_audio_line_coding_t *configuration);

/**
 * \brief Close a port
 *
 * \param port       Communication port number
 */
void uhi_audio_close(uint8_t port);

/**
 * \brief This function checks if a character has been received on the CDC line
 *
 * \param port       Communication port number
 *
 * \return \c true if a byte is ready to be read.
 */
bool uhi_audio_is_rx_ready(uint8_t port);

/**
 * \brief This function returns the number of character available on the CDC line
 *
 * \param port       Communication port number
 *
 * \return the number of data received
 */
iram_size_t uhi_audio_get_nb_received(uint8_t port);

/**
 * \brief Waits and gets a value on CDC line
 *
 * \param port       Communication port number
 *
 * \return value read on CDC line
 */
int uhi_audio_getc(uint8_t port);

/**
 * \brief Reads a RAM buffer on CDC line
 *
 * \param port      Communication port number
 * \param buf       Values read
 * \param size      Number of value read
 *
 * \return the number of data remaining
 */
iram_size_t uhi_audio_read_buf(uint8_t port, void* buf, iram_size_t size);

/**
 * \brief This function checks if a new character sent is possible
 * The type int is used to support scanf redirection from compiler LIB.
 *
 * \param port       Communication port number
 *
 * \return \c true if a new character can be sent
 */
bool uhi_audio_is_tx_ready(uint8_t port);

/**
 * \brief Puts a byte on CDC line
 * The type int is used to support printf redirection from compiler LIB.
 *
 * \param port       Communication port number
 * \param value      Value to put
 *
 * \return \c true if function was successfully done, otherwise \c false.
 */
int uhi_audio_putc(uint8_t port, int value);

/**
 * \brief Writes a RAM buffer on CDC line
 *
 * \param port       Communication port number
 * \param buf       Values to write
 * \param size      Number of value to write
 *
 * \return the number of data remaining
 */
iram_size_t uhi_audio_write_buf(uint8_t port, const void* buf, iram_size_t size);
//@}

/**
 * \page uhi_audio_quickstart Quick start guide for USB host Communication Device Class module (UHI CDC)
 *
 * This is the quick start guide for the \ref uhi_audio_group 
 * "USB host Communication Device Class module (UHI CDC)" with step-by-step instructions on 
 * how to configure and use the modules in a selection of use cases.
 *
 * The use cases contain several code fragments. The code fragments in the
 * steps for setup can be copied into a custom initialization function, while
 * the steps for usage can be copied into, e.g., the main application function.
 * 
 * \section uhi_audio_basic_use_case Basic use case
 * In this basic use case, the "USB Host CDC (Single Class support)" module is used.
 *
 * The "USB Host CDC (Multiple Classes support)" module usage is described
 * in \ref uhi_audio_use_cases "Advanced use cases".
 *
 * \section uhi_audio_basic_use_case_setup Setup steps
 * \subsection uhi_audio_basic_use_case_setup_prereq Prerequisites
 * \copydetails uhc_basic_use_case_setup_prereq
 * \subsection uhi_audio_basic_use_case_setup_code Example code
 * \copydetails uhc_basic_use_case_setup_code
 * \subsection uhi_audio_basic_use_case_setup_flow Workflow
 * \copydetails uhc_basic_use_case_setup_flow
 *
 * \section uhi_audio_basic_use_case_usage Usage steps
 *
 * \subsection uhi_audio_basic_use_case_usage_code Example code
 * Content of conf_usb_host.h:
 * \code
	#define USB_HOST_UHI        UHI_AUDIO
	#define UHI_AUDIO_CHANGE(dev, b_plug) my_callback_audio_change(dev, b_plug)
	extern bool my_callback_audio_change(uhc_device_t* dev, bool b_plug);
	#define UHI_AUDIO_RX_NOTIFY() my_callback_audio_rx_notify()
	extern void my_callback_audio_rx_notify(void);
	#include "uhi_audio.h" // At the end of conf_usb_host.h file
\endcode
 *
 * Add to application C-file:
 * \code
	 static bool my_flag_audio_available = false;
	 bool my_callback_audio_change(uhc_device_t* dev, bool b_plug)
	 {
	    if (b_plug) {

	       // USB Device CDC connected
	       my_flag_audio_available = true;
	       // Open and configure USB CDC ports
	       usb_audio_line_coding_t cfg = {
	          .dwDTERate   = CPU_TO_LE32(115200),
	          .bCharFormat = CDC_STOP_BITS_1,
	          .bParityType = CDC_PAR_NONE,
	          .bDataBits   = 8,
	       };
	       uhi_audio_open(0, &cfg);

	    } else {

	       my_flag_audio_available = false;

	    }
	 }

	 void my_callback_audio_rx_notify(void)
	 {
	    // Wakeup my_task_rx() task
	 }

	 #define MESSAGE "Hello"
	 void my_task(void)
	 {
	    static bool startup = true;

	    if (!my_flag_audio_available) {
	       startup = true;
	       return;
	    }

	    if (startup) {
	       startup = false;
	       // Send data on CDC communication port
	       uhi_audio_write_buf(0, MESSAGE, sizeof(MESSAGE)-1);
	       uhi_audio_putc(0,'\n');
	       return;
	    }
	 }

	 void my_task_rx(void)
	 {
	    while (uhi_audio_is_rx_ready(0)) {
	       int value = uhi_audio_getc(0);
	    }
	 }
\endcode
 *
 * \subsection uhi_audio_basic_use_case_setup_flow Workflow
 * -# Ensure that conf_usb_host.h is available and contains the following configuration
 * which is the USB host CDC configuration:
 *   - \code #define USB_HOST_UHI   UHI_AUDIO \endcode
 *     \note It defines the list of UHI supported by USB host.
 *   - \code #define UHI_AUDIO_CHANGE(dev, b_plug) my_callback_audio_change(dev, b_plug)
	 extern bool my_callback_audio_change(uhc_device_t* dev, bool b_plug); \endcode
 *     \note This callback is called when a USB device CDC is plugged or unplugged.
 *     The communication port can be opened and configured here.
 *   - \code #define UHI_AUDIO_RX_NOTIFY() my_callback_audio_rx_notify()
	 extern void my_callback_audio_rx_notify(void); \endcode
 *     \note This callback is called when a new data are received.
 *     This can be used to manage data reception through interrupt and avoid pooling.
 * -# The CDC data access functions are described in \ref uhi_audio_group.
 *
 * \section uhi_audio_use_cases Advanced use cases
 * For more advanced use of the UHI CDC module, see the following use cases:
 * - \subpage uhc_use_case_1
 * - \subpage uhc_use_case_2
 * - \subpage uhc_use_case_3
 */


#ifdef __cplusplus
}
#endif
#endif // _UHI_AUDIO_H_
