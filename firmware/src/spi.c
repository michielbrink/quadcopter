/*
 * =====================================================================================
 *
 *       Filename:  spi.c
 *
 *    Description:  Communication functions 
 *
 *        Version:  1.0
 *        Created:  06/29/2013 05:12:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Reinder Cuperus (reinder@snt.utwente.nl), 
 *
 * =====================================================================================
 */

/*
 * Workflow for communication with raspi:
 * # Initialize the SPI2 bus
 * # Setup dma
 * # Setup interupt-handler
 * # Clear receive/send circular buffers
 * # [ Get alignment of messages between raspi/stm ]
 * # Start message-handling by setting dma-buffer to message_size bytes, buffer-pointer to correct point in receive-buffer
 * # After the message is received, success-dma-interupt is raised
 *
 * Success interupt handler:
 * # close dma-transaction
 * # update circular buffers
 * # start next dma-transaction
 * 
 * parity-failure can happen because of:
 * # line-noise
 * # missed a byte
 * # raspi has reset
 * 
 * Re-aliging [TODO]:
 * # If the raspi detects the comms are broken, it starts sending 0x55 continuously
 * # If the stm detects are broken, it will only send back 0x55
 * # The sending of 0x55 by the stm indicates, it will wait for a 0xF0
 *   following one of more 0x55 to indicate that a new packet will start 
 * # The raspi detects that the stm is ready by receiving 0x55 from the stm
 * # The raspi will send one 0xFE
 * # The raspi will send the first message
 * # The stm will at the same time send its next message
 *
 * Message templates (all uint8_t):
 * Raspi <-> STM32 :
 * { command, var0, var1, var2, var3, var4, var5, var6, var7, parity }
 *
 */
#include "stm32f30x.h"
#include "stm32f3_discovery.h"
#include "spi.h"
#include "helpers.h"

/* Private variables */
raspiMessage emptyMessage = {.command = RASPI_CMD_NOP, .payload = { 0 }, .parity = RASPI_CMD_NOP};
raspiMessage errorMessage = {.command = RASPI_CMD_ERR, .payload = { RASPI_CMD_ERR }, .parity = RASPI_CMD_ERR};
uint8_t reinitMessage[4] = {:x55, 0x54, 0x53, 0x52 };

// This sets up a basic ring-buffer for receiving/transmitting messages
// Setup as __IO because it's written to in DMA and interrupts
__IO raspiMessage raspi_tx_buffer[RASPI_MSG_BUF_SIZE];
__IO raspiMessage raspi_rx_buffer[RASPI_MSG_BUF_SIZE];
__IO uint8_t raspi_tx_writep = 0;
__IO uint8_t raspi_tx_readp  = 0;
__IO uint8_t raspi_rx_writep = 0;
__IO uint8_t raspi_rx_readp  = 0;
__IO raspiMessage raspi_rx_dummy_buffer; // Used when rxBuffer is full

__IO uint8_t raspi_rx_used_buffer = 0;
__IO uint8_t raspi_tx_used_buffer = 0;
__IO uint8_t raspi_working_comms = 1;

__IO uint8_t raspi_state = SPI_STATE_UNINITIALIZED;

NVIC_InitTypeDef NVIC_InitStructure;

// Private functions
void startDmaTransfer(void);
void endDmaTransfer(void);

/* 
 * Sets up the SPI-communication:
 * - enables clocks/ports
 * - sets the correct modes
 * - enabled the IRQ of the DMA-receive-channel
 * - starts the first dma-transaction
 *
 * After this function has been called, SPI-communication is working in the
 * background using interrupts and the receive/transmit-queues can be
 * read/filled with their respective functions
 * */
void raspiComInit() {
  SPI_InitTypeDef SPIS;
  GPIO_InitTypeDef GPIOS;

  /*  Configure clocks */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

  /* Initialize the output pins */
  GPIOS.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIOS.GPIO_Mode = GPIO_Mode_AF;
  GPIOS.GPIO_Speed = GPIO_Speed_50MHz;
  GPIOS.GPIO_OType = GPIO_OType_PP;
  GPIOS.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIOS);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_5);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_5);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_5);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_5);

  // SPI
  SPI_I2S_DeInit(SPI2);
  SPI_StructInit(&SPIS);
  SPIS.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPIS.SPI_DataSize = SPI_DataSize_8b;
  SPIS.SPI_CPOL = SPI_CPOL_Low;
  SPIS.SPI_CPHA = SPI_CPHA_1Edge;
  SPIS.SPI_NSS = SPI_NSS_Hard;
  SPIS.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPIS.SPI_FirstBit = SPI_FirstBit_MSB;
  SPIS.SPI_CRCPolynomial = 7;
  SPIS.SPI_Mode = SPI_Mode_Slave;
  SPI_Init(SPI2, &SPIS);

  SPI_RxFIFOThresholdConfig(SPI2, SPI_RxFIFOThreshold_HF);
  SPI_Cmd(SPI2, ENABLE);

  /* Enable DMA1 channel4 IRQ Channel = receive-channel */
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  raspi_state = SPI_STATE_UNINITIALIZED;

  /* Start the first DMA-transfer, which keeps the comms nicely looping */
  startDmaTransfer();
}

/* 
 * This setups a SPI transfer for the next message in the transfer-queue.
 * If no outgoing message is present in the txBuffer, the no-message block is send.
 * DMA-channels 4 and 5 are used for this transfer.
 * After the transfer is finished an interupt will be raised.
 *
 * To setup a dma-transfer, the following things need to be done:
 * - reset dma-channel
 * - configure both dma-channels
 * - enable the interrupts
 * */
void startDmaTransfer(void) {
    // Setup DMA : channels from migration-doc
    // SPI2_Rx : DMA1_Channel4
    // SPI2_Tx : DMA1_Channel5
    DMA_InitTypeDef DMAS;
    DMA_DeInit(DMA1_Channel4);
    DMA_DeInit(DMA1_Channel5);

    // Common config
    DMAS.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI2->DR));
    DMAS.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMAS.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMAS.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMAS.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMAS.DMA_Mode = DMA_Mode_Normal;
    DMAS.DMA_Priority = DMA_Priority_VeryHigh;
    DMAS.DMA_M2M = DMA_M2M_Disable;

    // rxChannel
    switch ( raspi_state ) {
	case (SPI_STATE_INITIALIZED):
	    if ( ((RASPI_MSG_BUF_SIZE - raspi_rx_writep + raspi_rx_readp) % RASPI_MSG_BUF_SIZE) == 1) {
		// if all room is taken, assign the discard-dummy-buffer
		DMAS.DMA_MemoryBaseAddr = (uint32_t)&raspi_rx_dummy_buffer;
		raspi_rx_used_buffer = 0;
	    } else {
		DMAS.DMA_MemoryBaseAddr = (uint32_t)&(raspi_rx_buffer[raspi_rx_writep]);
		raspi_rx_used_buffer = 1;
	    }
	    DMAS.DMA_BufferSize = RASPI_PAYLOAD_LENGTH + 2;
	    break;
	case (SPI_STATE_INITIALIZING):
	case (SPI_STATE_UNINITIALIZED):
	    DMAS.DMA_MemoryBaseAddr = (uint32_t)&(raspi_rx_dummy_buffer);
	    DMAS.DMA_BufferSize = 1;
	    break;
    }
    DMAS.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_Init(DMA1_Channel4, &DMAS);

    // txChannel
    switch ( raspi_state ) {
	case (SPI_STATE_INITIALIZED):
	    if ( raspi_tx_readp == raspi_tx_writep ) {
		// if the out-queue is empty, send an empty message
		DMAS.DMA_MemoryBaseAddr = (uint32_t)&emptyMessage;
		raspi_tx_used_buffer = 0;
	    } else {
		DMAS.DMA_MemoryBaseAddr = (uint32_t)&(raspi_tx_buffer[raspi_tx_readp]);
		raspi_tx_used_buffer = 1;
	    }
	    break;
	case (SPI_STATE_INITIALIZING):
	case (SPI_STATE_UNINITIALIZED):
	    DMAS.DMA_MemoryBaseAddr = (uint32_t)&reinitMessage;
	    raspi_tx_used_buffer = 0;
	    break;
    }
    DMAS.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_Init(DMA1_Channel5, &DMAS);

    // Enable comms
    DMA_Cmd(DMA1_Channel4, ENABLE);
    DMA_Cmd(DMA1_Channel5, ENABLE);
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, ENABLE);

    /* Enable DMA1 Channel4 Transfer Complete interrupt */
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
}

/*
 * This function checks if the received message has correct parity
 */
inline uint8_t checkParity(__IO raspiMessage *msg) {
    uint8_t par = 0, i = 0;
    par ^= msg->command;
    for (i=0; i<RASPI_PAYLOAD_LENGTH; i++) {
	par ^= msg->payload[i];
    }
    return ( par == msg->parity);
}

/*
 * This functions handles stopping the DMA-transfer and doing basic message-parsing
 *
 * - It stops the DMA
 * - It checks if the parity is correct
 * - If the command is a NOP, the rx-pointer is not updated
 * - The rx/tx-buffer pointers are updated
 */
void endDmaTransfer(void) {
    // Disable comms
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx | SPI_I2S_DMAReq_Tx, DISABLE);

    switch ( raspi_state ) {
	case (SPI_STATE_UNINITIALIZED):
	    if ( raspi_rx_dummy_buffer.command == RASPI_REINIT ) {
		raspi_state = SPI_STATE_INITIALIZING;
	    }
	    break;
	case (SPI_STATE_INITIALIZING):
	    switch ( raspi_rx_dummy_buffer.command ) {
		case (RASPI_CMD_START):
		    raspi_state = SPI_STATE_INITIALIZED;
		    break;
		case (RASPI_REINIT):
		    break;
		default:
		    raspi_state = SPI_STATE_UNINITIALIZED;
		    break;
	    }
	    break;
	case (SPI_STATE_INITIALIZED):
	    if (raspi_rx_used_buffer) {
		if ( checkParity( &(raspi_rx_buffer[raspi_rx_writep]) ) ) {
		    switch ( raspi_rx_buffer[raspi_rx_writep].command ) {
			case (RASPI_CMD_NOP):
			    break;
			case (RASPI_CMD_WIRE_SHORTED):
			case (RASPI_CMD_WIRE_BROKEN):
			case (RASPI_REINIT):
			case (RASPI_CMD_START):
			    raspi_state = SPI_STATE_UNINITIALIZED;
			    break;
			default:
			    raspi_rx_writep = (raspi_rx_writep + 1) % RASPI_MSG_BUF_SIZE;
			    raspi_rx_num_messages = (RASPI_MSG_BUF_SIZE - raspi_rx_readp + raspi_rx_writep)%RASPI_MSG_BUF_SIZE;
			    break;
		    }
		} else {
		    // parity-error, reinitialization needed
		    raspi_state = SPI_STATE_UNINITIALIZED;
		}
	    } else {
		if ( checkParity( &emptyMessage ) ) {
		    raspi_state = SPI_STATE_UNINITIALIZED;
		    switch ( raspi_rx_buffer[raspi_rx_writep].command ) {
			case (RASPI_CMD_WIRE_SHORTED):
			case (RASPI_CMD_WIRE_BROKEN):
			case (RASPI_REINIT):
			case (RASPI_CMD_START):
			    raspi_state = SPI_STATE_UNINITIALIZED;
			    break;
		    }
		}
	    }
	    break;
    }
}

/* 
 * This handler is called when the transfer has finished
 * - The last transfer is processed
 * - The next transfer is prepared
 * */
void DMA1_Channel4_IRQHandler(void) {
    endDmaTransfer();
    startDmaTransfer();
}

/* 
 * Get the next message from the receive-queue.
 * 
 * This functions returns a 0 when the queue is empty.
 * Otherwise it places the next message in rxBuffer and returns a 1.
 * */
uint8_t raspiGetMessage(raspiMessage *rxBuffer) {
    uint8_t i;
    if ( raspi_rx_writep == raspi_rx_readp ) {
	return 0;
    }
    rxBuffer->command = raspi_rx_buffer[raspi_rx_readp].command;
    for (i=0; i<RASPI_PAYLOAD_LENGTH; i++) {
	rxBuffer->payload[i] = raspi_rx_buffer[raspi_rx_readp].payload[i];
    }
    raspi_rx_readp = (raspi_rx_readp+1)%RASPI_MSG_BUF_SIZE;
    raspi_rx_num_messages = (RASPI_MSG_BUF_SIZE - raspi_rx_readp + raspi_rx_writep)%RASPI_MSG_BUF_SIZE;
    return 1;
}
    
/* 
 * Place a message in the outgoing queue.
 *
 * If the queue is full, a 0 is returned, otherwise a 1.
 * The parity-byte is always calculated when the message is accepted.
 * */
uint8_t raspiSendMessage(raspiMessage *txBuffer) {
    uint8_t i;
    uint8_t par=0;
    if ( ((RASPI_MSG_BUF_SIZE - raspi_tx_writep + raspi_tx_readp) % RASPI_MSG_BUF_SIZE) == 1) {
	return 0;
    }
    raspi_tx_buffer[raspi_tx_writep].command = txBuffer->command;
    par ^= txBuffer->command;
    for (i=0; i<RASPI_PAYLOAD_LENGTH; i++) {
	raspi_tx_buffer[raspi_tx_writep].payload[i] = txBuffer->payload[i];
	par ^= txBuffer->payload[i];
    }
    raspi_tx_buffer[raspi_tx_writep].parity = par;
    raspi_tx_writep = (raspi_tx_writep+1)%RASPI_MSG_BUF_SIZE;
    raspi_tx_num_messages = (raspi_tx_writep - raspi_tx_readp)%RASPI_MSG_BUF_SIZE;
    return 1;
}
