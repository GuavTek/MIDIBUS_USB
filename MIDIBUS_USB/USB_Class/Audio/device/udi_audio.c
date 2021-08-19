/**
 * \file
 *
 * \brief USB Device Communication Device Class (CDC) interface.
 *
 * Copyright (c) 2009-2018 Microchip Technology Inc. and its subsidiaries.
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

#include "conf_usb.h"
#include "usb_protocol.h"
#include "usb_protocol_audio.h"
#include "udd.h"
#include "udc.h"
#include "udi_audio.h"
#include <string.h>

#ifdef UDI_AUDIO_LOW_RATE
#  ifdef USB_DEVICE_HS_SUPPORT
#    define UDI_AUDIO_TX_BUFFERS     (UDI_AUDIO_DATA_EPS_HS_SIZE)
#    define UDI_AUDIO_RX_BUFFERS     (UDI_AUDIO_DATA_EPS_HS_SIZE)
#  else
#    define UDI_AUDIO_TX_BUFFERS     (UDI_AUDIO_DATA_EPS_FS_SIZE)
#    define UDI_AUDIO_RX_BUFFERS     (UDI_AUDIO_DATA_EPS_FS_SIZE)
#  endif
#else
#  ifdef USB_DEVICE_HS_SUPPORT
#    define UDI_AUDIO_TX_BUFFERS     (UDI_AUDIO_DATA_EPS_HS_SIZE)
#    define UDI_AUDIO_RX_BUFFERS     (UDI_AUDIO_DATA_EPS_HS_SIZE)
#  else
#    define UDI_AUDIO_TX_BUFFERS     (5*UDI_AUDIO_DATA_EPS_FS_SIZE)
#    define UDI_AUDIO_RX_BUFFERS     (5*UDI_AUDIO_DATA_EPS_FS_SIZE)
#  endif
#endif

#ifndef UDI_AUDIO_TX_EMPTY_NOTIFY
#  define UDI_AUDIO_TX_EMPTY_NOTIFY(port)
#endif

/**
 * \ingroup udi_audio_group
 * \defgroup udi_audio_group_udc Interface with USB Device Core (UDC)
 *
 * Structures and functions required by UDC.
 *
 * @{
 */
bool udi_audio_comm_enable(void);
void udi_audio_comm_disable(void);
bool udi_audio_comm_setup(void);
bool udi_audio_data_enable(void);
void udi_audio_data_disable(void);
bool udi_audio_data_setup(void);
uint8_t udi_audio_getsetting(void);
void udi_audio_data_sof_notify(void);
UDC_DESC_STORAGE udi_api_t udi_api_audio_comm = {
	.enable = udi_audio_comm_enable,
	.disable = udi_audio_comm_disable,
	.setup = udi_audio_comm_setup,
	.getsetting = udi_audio_getsetting,
};
UDC_DESC_STORAGE udi_api_t udi_api_audio_data = {
	.enable = udi_audio_data_enable,
	.disable = udi_audio_data_disable,
	.setup = udi_audio_data_setup,
	.getsetting = udi_audio_getsetting,
	.sof_notify = udi_audio_data_sof_notify,
};
//@}

/**
 * \ingroup udi_audio_group
 * \defgroup udi_audio_group_internal Implementation of UDI CDC
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Internal routines
 */
//@{

/**
 * \name Routines to control serial line
 */
//@{

/**
 * \brief Returns the port number corresponding at current setup request
 *
 * \return port number
 */
static uint8_t udi_audio_setup_to_port(void);

/**
 * \brief Sends line coding to application
 *
 * Called after SETUP request when line coding data is received.
 */
static void udi_audio_line_coding_received(void);

/**
 * \brief Records new state
 *
 * \param port       Communication port number to manage
 * \param b_set      State is enabled if true, else disabled
 * \param bit_mask   Field to process (see CDC_SERIAL_STATE_ defines)
 */
static void udi_audio_ctrl_state_change(uint8_t port, bool b_set, le16_t bit_mask);

/**
 * \brief Check and eventually notify the USB host of new state
 *
 * \param port       Communication port number to manage
 * \param ep         Port communication endpoint
 */
static void udi_audio_ctrl_state_notify(uint8_t port, udd_ep_id_t ep);

/**
 * \brief Ack sent of serial state message
 * Callback called after serial state message sent
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finished
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param n          number of data transfered
 */
static void udi_audio_serial_state_msg_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep);

//@}

/**
 * \name Routines to process data transfer
 */
//@{

/**
 * \brief Enable the reception of data from the USB host
 *
 * The value udi_audio_rx_trans_sel indicate the RX buffer to fill.
 *
 * \param port       Communication port number to manage
 *
 * \return \c 1 if function was successfully done, otherwise \c 0.
 */
static bool udi_audio_rx_start(uint8_t port);

/**
 * \brief Update rx buffer management with a new data
 * Callback called after data reception on USB line
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finish
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param n          number of data received
 */
static void udi_audio_data_received(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep);

/**
 * \brief Ack sent of tx buffer
 * Callback called after data transfer on USB line
 *
 * \param status     UDD_EP_TRANSFER_OK, if transfer finished
 * \param status     UDD_EP_TRANSFER_ABORT, if transfer aborted
 * \param n          number of data transfered
 */
static void udi_audio_data_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep);

/**
 * \brief Send buffer on line or wait a SOF event
 *
 * \param port       Communication port number to manage
 */
static void udi_audio_tx_send(uint8_t port);

//@}

//@}

/**
 * \name Information about configuration of communication line
 */
//@{
COMPILER_WORD_ALIGNED
		static usb_audio_line_coding_t udi_audio_line_coding[UDI_AUDIO_PORT_NB];
static bool udi_audio_serial_state_msg_ongoing[UDI_AUDIO_PORT_NB];
static volatile le16_t udi_audio_state[UDI_AUDIO_PORT_NB];
COMPILER_WORD_ALIGNED static usb_audio_notify_serial_state_t uid_audio_state_msg[UDI_AUDIO_PORT_NB];

//! Status of CDC COMM interfaces
static volatile uint8_t udi_audio_nb_comm_enabled = 0;
//@}

/**
 * \name Variables to manage RX/TX transfer requests
 * Two buffers for each sense are used to optimize the speed.
 */
//@{

//! Status of CDC DATA interfaces
static volatile uint8_t udi_audio_nb_data_enabled = 0;
static volatile bool udi_audio_data_running = false;
//! Buffer to receive data
COMPILER_WORD_ALIGNED static uint8_t udi_audio_rx_buf[UDI_AUDIO_PORT_NB][2][UDI_AUDIO_RX_BUFFERS];
//! Data available in RX buffers
static volatile uint16_t udi_audio_rx_buf_nb[UDI_AUDIO_PORT_NB][2];
//! Give the current RX buffer used (rx0 if 0, rx1 if 1)
static volatile uint8_t udi_audio_rx_buf_sel[UDI_AUDIO_PORT_NB];
//! Read position in current RX buffer
static volatile uint16_t udi_audio_rx_pos[UDI_AUDIO_PORT_NB];
//! Signal a transfer on-going
static volatile bool udi_audio_rx_trans_ongoing[UDI_AUDIO_PORT_NB];

//! Define a transfer halted
#define  UDI_AUDIO_TRANS_HALTED    2

//! Buffer to send data
COMPILER_WORD_ALIGNED static uint8_t udi_audio_tx_buf[UDI_AUDIO_PORT_NB][2][UDI_AUDIO_TX_BUFFERS];
//! Data available in TX buffers
static uint16_t udi_audio_tx_buf_nb[UDI_AUDIO_PORT_NB][2];
//! Give current TX buffer used (tx0 if 0, tx1 if 1)
static volatile uint8_t udi_audio_tx_buf_sel[UDI_AUDIO_PORT_NB];
//! Value of SOF during last TX transfer
static uint16_t udi_audio_tx_sof_num[UDI_AUDIO_PORT_NB];
//! Signal a transfer on-going
static volatile bool udi_audio_tx_trans_ongoing[UDI_AUDIO_PORT_NB];
//! Signal that both buffer content data to send
static volatile bool udi_audio_tx_both_buf_to_send[UDI_AUDIO_PORT_NB];

//@}

bool udi_audio_comm_enable(void)
{
	uint8_t port;
	uint8_t iface_comm_num;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
	udi_audio_nb_comm_enabled = 0;
#else
	if (udi_audio_nb_comm_enabled > UDI_AUDIO_PORT_NB) {
		udi_audio_nb_comm_enabled = 0;
	}
	port = udi_audio_nb_comm_enabled;
#endif

	// Initialize control signal management
	udi_audio_state[port] = CPU_TO_LE16(0);

	uid_audio_state_msg[port].header.bmRequestType =
			USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS |
			USB_REQ_RECIP_INTERFACE;
	uid_audio_state_msg[port].header.bNotification = USB_REQ_AUDIO_NOTIFY_SERIAL_STATE;
	uid_audio_state_msg[port].header.wValue = LE16(0);

	switch (port) {
#define UDI_AUDIO_PORT_TO_IFACE_COMM(index, unused) \
	case index: \
		iface_comm_num = UDI_AUDIO_COMM_IFACE_NUMBER_##index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_PORT_TO_IFACE_COMM, ~)
#undef UDI_AUDIO_PORT_TO_IFACE_COMM
	default:
		iface_comm_num = UDI_AUDIO_COMM_IFACE_NUMBER_0;
		break;
	}

	uid_audio_state_msg[port].header.wIndex = LE16(iface_comm_num);
	uid_audio_state_msg[port].header.wLength = LE16(2);
	uid_audio_state_msg[port].value = CPU_TO_LE16(0);

	udi_audio_line_coding[port].dwDTERate = CPU_TO_LE32(UDI_AUDIO_DEFAULT_RATE);
	udi_audio_line_coding[port].bCharFormat = UDI_AUDIO_DEFAULT_STOPBITS;
	udi_audio_line_coding[port].bParityType = UDI_AUDIO_DEFAULT_PARITY;
	udi_audio_line_coding[port].bDataBits = UDI_AUDIO_DEFAULT_DATABITS;
	// Call application callback
	// to initialize memories or indicate that interface is enabled
	UDI_AUDIO_SET_CODING_EXT(port,(&udi_audio_line_coding[port]));
	if (!UDI_AUDIO_ENABLE_EXT(port)) {
		return false;
	}
	udi_audio_nb_comm_enabled++;
	return true;
}

bool udi_audio_data_enable(void)
{
	uint8_t port;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
	udi_audio_nb_data_enabled = 0;
#else
	if (udi_audio_nb_data_enabled > UDI_AUDIO_PORT_NB) {
		udi_audio_nb_data_enabled = 0;
	}
	port = udi_audio_nb_data_enabled;
#endif

	// Initialize TX management
	udi_audio_tx_trans_ongoing[port] = false;
	udi_audio_tx_both_buf_to_send[port] = false;
	udi_audio_tx_buf_sel[port] = 0;
	udi_audio_tx_buf_nb[port][0] = 0;
	udi_audio_tx_buf_nb[port][1] = 0;
	udi_audio_tx_sof_num[port] = 0;
	udi_audio_tx_send(port);

	// Initialize RX management
	udi_audio_rx_trans_ongoing[port] = false;
	udi_audio_rx_buf_sel[port] = 0;
	udi_audio_rx_buf_nb[port][0] = 0;
	udi_audio_rx_buf_nb[port][1] = 0;
	udi_audio_rx_pos[port] = 0;
	if (!udi_audio_rx_start(port)) {
		return false;
	}
	udi_audio_nb_data_enabled++;
	if (udi_audio_nb_data_enabled == UDI_AUDIO_PORT_NB) {
		udi_audio_data_running = true;
	}
	return true;
}

void udi_audio_comm_disable(void)
{
	Assert(udi_audio_nb_comm_enabled != 0);
	udi_audio_nb_comm_enabled--;
}

void udi_audio_data_disable(void)
{
	uint8_t port;

	Assert(udi_audio_nb_data_enabled != 0);
	udi_audio_nb_data_enabled--;
	port = udi_audio_nb_data_enabled;
	UDI_AUDIO_DISABLE_EXT(port);
	udi_audio_data_running = false;
}

bool udi_audio_comm_setup(void)
{
	uint8_t port = udi_audio_setup_to_port();

	if (Udd_setup_is_in()) {
		// GET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Get
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_AUDIO_GET_LINE_CODING:
				// Get configuration of CDC line
				if (sizeof(usb_audio_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_audio_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_audio_line_coding_t);
				return true;
			}
		}
	}
	if (Udd_setup_is_out()) {
		// SET Interface Requests
		if (Udd_setup_type() == USB_REQ_TYPE_CLASS) {
			// Requests Class Interface Set
			switch (udd_g_ctrlreq.req.bRequest) {
			case USB_REQ_AUDIO_SET_LINE_CODING:
				// Change configuration of CDC line
				if (sizeof(usb_audio_line_coding_t) !=
						udd_g_ctrlreq.req.wLength)
					return false; // Error for USB host
				udd_g_ctrlreq.callback =
						udi_audio_line_coding_received;
				udd_g_ctrlreq.payload =
						(uint8_t *) &
						udi_audio_line_coding[port];
				udd_g_ctrlreq.payload_size =
						sizeof(usb_audio_line_coding_t);
				return true;
			case USB_REQ_AUDIO_SET_CONTROL_LINE_STATE:
				// According cdc spec 1.1 chapter 6.2.14
				UDI_AUDIO_SET_DTR_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_DTE_PRESENT)));
				UDI_AUDIO_SET_RTS_EXT(port, (0 !=
						(udd_g_ctrlreq.req.wValue
						 & CDC_CTRL_SIGNAL_ACTIVATE_CARRIER)));
				return true;
			}
		}
	}
	return false;  // request Not supported
}

bool udi_audio_data_setup(void)
{
	return false;  // request Not supported
}

uint8_t udi_audio_getsetting(void)
{
	return 0;      // CDC don't have multiple alternate setting
}

void udi_audio_data_sof_notify(void)
{
	static uint8_t port_notify = 0;

	// A call of udi_audio_data_sof_notify() is done for each port
	udi_audio_tx_send(port_notify);
#if UDI_AUDIO_PORT_NB != 1 // To optimize code
	port_notify++;
	if (port_notify >= UDI_AUDIO_PORT_NB) {
		port_notify = 0;
	}
#endif
}


//-------------------------------------------------
//------- Internal routines to control serial line

static uint8_t udi_audio_setup_to_port(void)
{
	uint8_t port;

	switch (udd_g_ctrlreq.req.wIndex & 0xFF) {
#define UDI_AUDIO_IFACE_COMM_TO_PORT(iface, unused) \
	case UDI_AUDIO_COMM_IFACE_NUMBER_##iface: \
		port = iface; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_IFACE_COMM_TO_PORT, ~)
#undef UDI_AUDIO_IFACE_COMM_TO_PORT
	default:
		port = 0;
		break;
	}
	return port;
}

static void udi_audio_line_coding_received(void)
{
	uint8_t port = udi_audio_setup_to_port();
	UNUSED(port);

	UDI_AUDIO_SET_CODING_EXT(port, (&udi_audio_line_coding[port]));
}

static void udi_audio_ctrl_state_change(uint8_t port, bool b_set, le16_t bit_mask)
{
	irqflags_t flags;
	udd_ep_id_t ep_comm;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	// Update state
	flags = cpu_irq_save(); // Protect udi_audio_state
	if (b_set) {
		udi_audio_state[port] |= bit_mask;
	} else {
		udi_audio_state[port] &= ~(unsigned)bit_mask;
	}
	cpu_irq_restore(flags);

	// Send it if possible and state changed
	switch (port) {
#define UDI_AUDIO_PORT_TO_COMM_EP(index, unused) \
	case index: \
		ep_comm = UDI_AUDIO_COMM_EP_##index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_PORT_TO_COMM_EP, ~)
#undef UDI_AUDIO_PORT_TO_COMM_EP
	default:
		ep_comm = UDI_AUDIO_COMM_EP_0;
		break;
	}
	udi_audio_ctrl_state_notify(port, ep_comm);
}


static void udi_audio_ctrl_state_notify(uint8_t port, udd_ep_id_t ep)
{
#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	// Send it if possible and state changed
	if ((!udi_audio_serial_state_msg_ongoing[port])
			&& (udi_audio_state[port] != uid_audio_state_msg[port].value)) {
		// Fill notification message
		uid_audio_state_msg[port].value = udi_audio_state[port];
		// Send notification message
		udi_audio_serial_state_msg_ongoing[port] =
				udd_ep_run(ep,
				false,
				(uint8_t *) & uid_audio_state_msg[port],
				sizeof(uid_audio_state_msg[0]),
				udi_audio_serial_state_msg_sent);
	}
}


static void udi_audio_serial_state_msg_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep)
{
	uint8_t port;
	UNUSED(n);
	UNUSED(status);

	switch (ep) {
#define UDI_AUDIO_GET_PORT_FROM_COMM_EP(iface, unused) \
	case UDI_AUDIO_COMM_EP_##iface: \
		port = iface; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_GET_PORT_FROM_COMM_EP, ~)
#undef UDI_AUDIO_GET_PORT_FROM_COMM_EP
	default:
		port = 0;
		break;
	}

	udi_audio_serial_state_msg_ongoing[port] = false;

	// For the irregular signals like break, the incoming ring signal,
	// or the overrun error state, this will reset their values to zero
	// and again will not send another notification until their state changes.
	udi_audio_state[port] &= ~(CDC_SERIAL_STATE_BREAK |
			CDC_SERIAL_STATE_RING |
			CDC_SERIAL_STATE_FRAMING |
			CDC_SERIAL_STATE_PARITY | CDC_SERIAL_STATE_OVERRUN);
	uid_audio_state_msg[port].value &= ~(CDC_SERIAL_STATE_BREAK |
			CDC_SERIAL_STATE_RING |
			CDC_SERIAL_STATE_FRAMING |
			CDC_SERIAL_STATE_PARITY | CDC_SERIAL_STATE_OVERRUN);
	// Send it if possible and state changed
	udi_audio_ctrl_state_notify(port, ep);
}


//-------------------------------------------------
//------- Internal routines to process data transfer


static bool udi_audio_rx_start(uint8_t port)
{
	irqflags_t flags;
	uint8_t buf_sel_trans;
	udd_ep_id_t ep;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	flags = cpu_irq_save();
	buf_sel_trans = udi_audio_rx_buf_sel[port];
	if (udi_audio_rx_trans_ongoing[port] ||
		(udi_audio_rx_pos[port] < udi_audio_rx_buf_nb[port][buf_sel_trans])) {
		// Transfer already on-going or current buffer no empty
		cpu_irq_restore(flags);
		return false;
	}

	// Change current buffer
	udi_audio_rx_pos[port] = 0;
	udi_audio_rx_buf_sel[port] = (buf_sel_trans==0)?1:0;

	// Start transfer on RX
	udi_audio_rx_trans_ongoing[port] = true;
	cpu_irq_restore(flags);

	if (udi_audio_multi_is_rx_ready(port)) {
		UDI_AUDIO_RX_NOTIFY(port);
	}
	// Send the buffer with enable of short packet
	switch (port) {
#define UDI_AUDIO_PORT_TO_DATA_EP_OUT(index, unused) \
	case index: \
		ep = UDI_AUDIO_DATA_EP_OUT_##index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_PORT_TO_DATA_EP_OUT, ~)
#undef UDI_AUDIO_PORT_TO_DATA_EP_OUT
	default:
		ep = UDI_AUDIO_DATA_EP_OUT_0;
		break;
	}
	return udd_ep_run(ep,
			true,
			udi_audio_rx_buf[port][buf_sel_trans],
			UDI_AUDIO_RX_BUFFERS,
			udi_audio_data_received);
}


static void udi_audio_data_received(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep)
{
	uint8_t buf_sel_trans;
	uint8_t port;

	switch (ep) {
#define UDI_AUDIO_DATA_EP_OUT_TO_PORT(index, unused) \
	case UDI_AUDIO_DATA_EP_OUT_##index: \
		port = index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_DATA_EP_OUT_TO_PORT, ~)
#undef UDI_AUDIO_DATA_EP_OUT_TO_PORT
	default:
		port = 0;
		break;
	}

	if (UDD_EP_TRANSFER_OK != status) {
		// Abort reception
		return;
	}
	buf_sel_trans = (udi_audio_rx_buf_sel[port]==0)?1:0;
	if (!n) {
		udd_ep_run( ep,
				true,
				udi_audio_rx_buf[port][buf_sel_trans],
				UDI_AUDIO_RX_BUFFERS,
				udi_audio_data_received);
		return;
	}
	udi_audio_rx_buf_nb[port][buf_sel_trans] = n;
	udi_audio_rx_trans_ongoing[port] = false;
	udi_audio_rx_start(port);
}


static void udi_audio_data_sent(udd_ep_status_t status, iram_size_t n, udd_ep_id_t ep)
{
	uint8_t port;
	UNUSED(n);

	switch (ep) {
#define UDI_AUDIO_DATA_EP_IN_TO_PORT(index, unused) \
	case UDI_AUDIO_DATA_EP_IN_##index: \
		port = index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_DATA_EP_IN_TO_PORT, ~)
#undef UDI_AUDIO_DATA_EP_IN_TO_PORT
	default:
		port = 0;
		break;
	}

	if (UDD_EP_TRANSFER_OK != status) {
		// Abort transfer
		return;
	}
	udi_audio_tx_buf_nb[port][(udi_audio_tx_buf_sel[port]==0)?1:0] = 0;
	udi_audio_tx_both_buf_to_send[port] = false;
	udi_audio_tx_trans_ongoing[port] = false;

	if (n != 0) {
		UDI_AUDIO_TX_EMPTY_NOTIFY(port);
	}
	udi_audio_tx_send(port);
}


static void udi_audio_tx_send(uint8_t port)
{
	irqflags_t flags;
	uint8_t buf_sel_trans;
	bool b_short_packet;
	udd_ep_id_t ep;
	static uint16_t sof_zlp_counter = 0;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	if (udi_audio_tx_trans_ongoing[port]) {
		return; // Already on going or wait next SOF to send next data
	}
	if (udd_is_high_speed()) {
		if (udi_audio_tx_sof_num[port] == udd_get_micro_frame_number()) {
			return; // Wait next SOF to send next data
		}
	}else{
		if (udi_audio_tx_sof_num[port] == udd_get_frame_number()) {
			return; // Wait next SOF to send next data
		}
	}

	flags = cpu_irq_save(); // to protect udi_audio_tx_buf_sel
	buf_sel_trans = udi_audio_tx_buf_sel[port];
	if (udi_audio_tx_buf_nb[port][buf_sel_trans] == 0) {
		sof_zlp_counter++;
		if (((!udd_is_high_speed()) && (sof_zlp_counter < 100))
				|| (udd_is_high_speed() && (sof_zlp_counter < 800))) {
			cpu_irq_restore(flags);
			return;
		}
	}
	sof_zlp_counter = 0;

	if (!udi_audio_tx_both_buf_to_send[port]) {
		// Send current Buffer
		// and switch the current buffer
		udi_audio_tx_buf_sel[port] = (buf_sel_trans==0)?1:0;
	}else{
		// Send the other Buffer
		// and no switch the current buffer
		buf_sel_trans = (buf_sel_trans==0)?1:0;
	}
	udi_audio_tx_trans_ongoing[port] = true;
	cpu_irq_restore(flags);

	b_short_packet = (udi_audio_tx_buf_nb[port][buf_sel_trans] != UDI_AUDIO_TX_BUFFERS);
	if (b_short_packet) {
		if (udd_is_high_speed()) {
			udi_audio_tx_sof_num[port] = udd_get_micro_frame_number();
		}else{
			udi_audio_tx_sof_num[port] = udd_get_frame_number();
		}
	}else{
		udi_audio_tx_sof_num[port] = 0; // Force next transfer without wait SOF
	}

	// Send the buffer with enable of short packet
	switch (port) {
#define UDI_AUDIO_PORT_TO_DATA_EP_IN(index, unused) \
	case index: \
		ep = UDI_AUDIO_DATA_EP_IN_##index; \
		break;
	MREPEAT(UDI_AUDIO_PORT_NB, UDI_AUDIO_PORT_TO_DATA_EP_IN, ~)
#undef UDI_AUDIO_PORT_TO_DATA_EP_IN
	default:
		ep = UDI_AUDIO_DATA_EP_IN_0;
		break;
	}
	udd_ep_run( ep,
			b_short_packet,
			udi_audio_tx_buf[port][buf_sel_trans],
			udi_audio_tx_buf_nb[port][buf_sel_trans],
			udi_audio_data_sent);
}


//---------------------------------------------
//------- Application interface


//------- Application interface

void udi_audio_ctrl_signal_dcd(bool b_set)
{
	udi_audio_ctrl_state_change(0, b_set, CDC_SERIAL_STATE_DCD);
}

void udi_audio_ctrl_signal_dsr(bool b_set)
{
	udi_audio_ctrl_state_change(0, b_set, CDC_SERIAL_STATE_DSR);
}

void udi_audio_signal_framing_error(void)
{
	udi_audio_ctrl_state_change(0, true, CDC_SERIAL_STATE_FRAMING);
}

void udi_audio_signal_parity_error(void)
{
	udi_audio_ctrl_state_change(0, true, CDC_SERIAL_STATE_PARITY);
}

void udi_audio_signal_overrun(void)
{
	udi_audio_ctrl_state_change(0, true, CDC_SERIAL_STATE_OVERRUN);
}

void udi_audio_multi_ctrl_signal_dcd(uint8_t port, bool b_set)
{
	udi_audio_ctrl_state_change(port, b_set, CDC_SERIAL_STATE_DCD);
}

void udi_audio_multi_ctrl_signal_dsr(uint8_t port, bool b_set)
{
	udi_audio_ctrl_state_change(port, b_set, CDC_SERIAL_STATE_DSR);
}

void udi_audio_multi_signal_framing_error(uint8_t port)
{
	udi_audio_ctrl_state_change(port, true, CDC_SERIAL_STATE_FRAMING);
}

void udi_audio_multi_signal_parity_error(uint8_t port)
{
	udi_audio_ctrl_state_change(port, true, CDC_SERIAL_STATE_PARITY);
}

void udi_audio_multi_signal_overrun(uint8_t port)
{
	udi_audio_ctrl_state_change(port, true, CDC_SERIAL_STATE_OVERRUN);
}

iram_size_t udi_audio_multi_get_nb_received_data(uint8_t port)
{
	irqflags_t flags;
	uint16_t pos;
	iram_size_t nb_received;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif
	flags = cpu_irq_save();
	pos = udi_audio_rx_pos[port];
	nb_received = udi_audio_rx_buf_nb[port][udi_audio_rx_buf_sel[port]] - pos;
	cpu_irq_restore(flags);
	return nb_received;
}

iram_size_t udi_audio_get_nb_received_data(void)
{
	return udi_audio_multi_get_nb_received_data(0);
}

bool udi_audio_multi_is_rx_ready(uint8_t port)
{
	return (udi_audio_multi_get_nb_received_data(port) > 0);
}

bool udi_audio_is_rx_ready(void)
{
	return udi_audio_multi_is_rx_ready(0);
}

int udi_audio_multi_getc(uint8_t port)
{
	irqflags_t flags;
	int rx_data = 0;
	bool b_databit_9;
	uint16_t pos;
	uint8_t buf_sel;
	bool again;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	b_databit_9 = (9 == udi_audio_line_coding[port].bDataBits);

udi_audio_getc_process_one_byte:
	// Check available data
	flags = cpu_irq_save();
	pos = udi_audio_rx_pos[port];
	buf_sel = udi_audio_rx_buf_sel[port];
	again = pos >= udi_audio_rx_buf_nb[port][buf_sel];
	cpu_irq_restore(flags);
	while (again) {
		if (!udi_audio_data_running) {
			return 0;
		}
		goto udi_audio_getc_process_one_byte;
	}

	// Read data
	rx_data |= udi_audio_rx_buf[port][buf_sel][pos];
	udi_audio_rx_pos[port] = pos+1;

	udi_audio_rx_start(port);

	if (b_databit_9) {
		// Receive MSB
		b_databit_9 = false;
		rx_data = rx_data << 8;
		goto udi_audio_getc_process_one_byte;
	}
	return rx_data;
}

int udi_audio_getc(void)
{
	return udi_audio_multi_getc(0);
}

iram_size_t udi_audio_multi_read_buf(uint8_t port, void* buf, iram_size_t size)
{
	irqflags_t flags;
	uint8_t *ptr_buf = (uint8_t *)buf;
	iram_size_t copy_nb;
	uint16_t pos;
	uint8_t buf_sel;
	bool again;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

udi_audio_read_buf_loop_wait:
	// Check available data
	flags = cpu_irq_save();
	pos = udi_audio_rx_pos[port];
	buf_sel = udi_audio_rx_buf_sel[port];
	again = pos >= udi_audio_rx_buf_nb[port][buf_sel];
	cpu_irq_restore(flags);
	while (again) {
		if (!udi_audio_data_running) {
			return size;
		}
		goto udi_audio_read_buf_loop_wait;
	}

	// Read data
	copy_nb = udi_audio_rx_buf_nb[port][buf_sel] - pos;
	if (copy_nb>size) {
		copy_nb = size;
	}
	memcpy(ptr_buf, &udi_audio_rx_buf[port][buf_sel][pos], copy_nb);
	udi_audio_rx_pos[port] += copy_nb;
	ptr_buf += copy_nb;
	size -= copy_nb;
	udi_audio_rx_start(port);

	if (size) {
		goto udi_audio_read_buf_loop_wait;
	}
	return 0;
}

iram_size_t udi_audio_multi_read_no_polling(uint8_t port, void* buf, iram_size_t size)
{
	uint8_t *ptr_buf = (uint8_t *)buf;
	iram_size_t nb_avail_data;
	uint16_t pos;
	uint8_t buf_sel;
	irqflags_t flags;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	//Data interface not started... exit
	if (!udi_audio_data_running) {
		return 0;
	}
	
	//Get number of available data
	// Check available data
	flags = cpu_irq_save(); // to protect udi_audio_rx_pos & udi_audio_rx_buf_sel
	pos = udi_audio_rx_pos[port];
	buf_sel = udi_audio_rx_buf_sel[port];
	nb_avail_data = udi_audio_rx_buf_nb[port][buf_sel] - pos;
	cpu_irq_restore(flags);
	//If the buffer contains less than the requested number of data,
	//adjust read size
	if(nb_avail_data<size) {
		size = nb_avail_data;
	}
	if(size>0) {
		memcpy(ptr_buf, &udi_audio_rx_buf[port][buf_sel][pos], size);
		flags = cpu_irq_save(); // to protect udi_audio_rx_pos
		udi_audio_rx_pos[port] += size;
		cpu_irq_restore(flags);
		
		ptr_buf += size;
		udi_audio_rx_start(port);
	}
	return(nb_avail_data);
}

iram_size_t udi_audio_read_no_polling(void* buf, iram_size_t size)
{
	return udi_audio_multi_read_no_polling(0, buf, size);
}

iram_size_t udi_audio_read_buf(void* buf, iram_size_t size)
{
	return udi_audio_multi_read_buf(0, buf, size);
}

iram_size_t udi_audio_multi_get_free_tx_buffer(uint8_t port)
{
	irqflags_t flags;
	iram_size_t buf_sel_nb, retval;
	uint8_t buf_sel;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	flags = cpu_irq_save();
	buf_sel = udi_audio_tx_buf_sel[port];
	buf_sel_nb = udi_audio_tx_buf_nb[port][buf_sel];
	if (buf_sel_nb == UDI_AUDIO_TX_BUFFERS) {
		if ((!udi_audio_tx_trans_ongoing[port])
			&& (!udi_audio_tx_both_buf_to_send[port])) {
			/* One buffer is full, but the other buffer is not used.
			 * (not used = transfer on-going)
			 * then move to the other buffer to store data */
			udi_audio_tx_both_buf_to_send[port] = true;
			udi_audio_tx_buf_sel[port] = (buf_sel == 0)? 1 : 0;
			buf_sel_nb = 0;
		}
	}
	retval = UDI_AUDIO_TX_BUFFERS - buf_sel_nb;  
	cpu_irq_restore(flags);
	return retval;
}

iram_size_t udi_audio_get_free_tx_buffer(void)
{
	return udi_audio_multi_get_free_tx_buffer(0);
}

bool udi_audio_multi_is_tx_ready(uint8_t port)
{
	return (udi_audio_multi_get_free_tx_buffer(port) != 0);
}

bool udi_audio_is_tx_ready(void)
{
	return udi_audio_multi_is_tx_ready(0);
}

int udi_audio_multi_putc(uint8_t port, int value)
{
	irqflags_t flags;
	bool b_databit_9;
	uint8_t buf_sel;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	b_databit_9 = (9 == udi_audio_line_coding[port].bDataBits);

udi_audio_putc_process_one_byte:
	// Check available space
	if (!udi_audio_multi_is_tx_ready(port)) {
		if (!udi_audio_data_running) {
			return false;
		}
		goto udi_audio_putc_process_one_byte;
	}

	// Write value
	flags = cpu_irq_save();
	buf_sel = udi_audio_tx_buf_sel[port];
	udi_audio_tx_buf[port][buf_sel][udi_audio_tx_buf_nb[port][buf_sel]++] = value;
	cpu_irq_restore(flags);

	if (b_databit_9) {
		// Send MSB
		b_databit_9 = false;
		value = value >> 8;
		goto udi_audio_putc_process_one_byte;
	}
	return true;
}

int udi_audio_putc(int value)
{
	return udi_audio_multi_putc(0, value);
}

iram_size_t udi_audio_multi_write_buf(uint8_t port, const void* buf, iram_size_t size)
{
	irqflags_t flags;
	uint8_t buf_sel;
	uint16_t buf_nb;
	iram_size_t copy_nb;
	uint8_t *ptr_buf = (uint8_t *)buf;

#if UDI_AUDIO_PORT_NB == 1 // To optimize code
	port = 0;
#endif

	if (9 == udi_audio_line_coding[port].bDataBits) {
		size *=2;
	}

udi_audio_write_buf_loop_wait:
	// Check available space
	if (!udi_audio_multi_is_tx_ready(port)) {
		if (!udi_audio_data_running) {
			return size;
		}
		goto udi_audio_write_buf_loop_wait;
	}

	// Write values
	flags = cpu_irq_save();
	buf_sel = udi_audio_tx_buf_sel[port];
	buf_nb = udi_audio_tx_buf_nb[port][buf_sel];
	copy_nb = UDI_AUDIO_TX_BUFFERS - buf_nb;
	if (copy_nb > size) {
		copy_nb = size;
	}
	memcpy(&udi_audio_tx_buf[port][buf_sel][buf_nb], ptr_buf, copy_nb);
	udi_audio_tx_buf_nb[port][buf_sel] = buf_nb + copy_nb;
	cpu_irq_restore(flags);

	// Update buffer pointer
	ptr_buf = ptr_buf + copy_nb;
	size -= copy_nb;

	if (size) {
		goto udi_audio_write_buf_loop_wait;
	}

	return 0;
}

iram_size_t udi_audio_write_buf(const void* buf, iram_size_t size)
{
	return udi_audio_multi_write_buf(0, buf, size);
}

//@}
