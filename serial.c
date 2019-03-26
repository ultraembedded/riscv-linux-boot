#include "serial.h"

//-----------------------------------------------------------------
// Defines
//-----------------------------------------------------------------
#define ULITE_RX          0x0
    #define ULITE_RX_DATA_SHIFT                  0
    #define ULITE_RX_DATA_MASK                   0xff

#define ULITE_TX          0x4
    #define ULITE_TX_DATA_SHIFT                  0
    #define ULITE_TX_DATA_MASK                   0xff

#define ULITE_STATUS      0x8
    #define ULITE_STATUS_IE                      4
    #define ULITE_STATUS_IE_SHIFT                4
    #define ULITE_STATUS_IE_MASK                 0x1

    #define ULITE_STATUS_TXFULL                  3
    #define ULITE_STATUS_TXFULL_SHIFT            3
    #define ULITE_STATUS_TXFULL_MASK             0x1

    #define ULITE_STATUS_TXEMPTY                 2
    #define ULITE_STATUS_TXEMPTY_SHIFT           2
    #define ULITE_STATUS_TXEMPTY_MASK            0x1

    #define ULITE_STATUS_RXFULL                  1
    #define ULITE_STATUS_RXFULL_SHIFT            1
    #define ULITE_STATUS_RXFULL_MASK             0x1

    #define ULITE_STATUS_RXVALID                 0
    #define ULITE_STATUS_RXVALID_SHIFT           0
    #define ULITE_STATUS_RXVALID_MASK            0x1

#define ULITE_CONTROL     0xc
    #define ULITE_CONTROL_IE                     4
    #define ULITE_CONTROL_IE_SHIFT               4
    #define ULITE_CONTROL_IE_MASK                0x1

    #define ULITE_CONTROL_RST_RX                 1
    #define ULITE_CONTROL_RST_RX_SHIFT           1
    #define ULITE_CONTROL_RST_RX_MASK            0x1

    #define ULITE_CONTROL_RST_TX                 0
    #define ULITE_CONTROL_RST_TX_SHIFT           0
    #define ULITE_CONTROL_RST_TX_MASK            0x1

//-----------------------------------------------------------------
// Locals
//-----------------------------------------------------------------
static volatile uint32_t *m_uart;

//-----------------------------------------------------------------
// serial_init: Initialise UART peripheral
//-----------------------------------------------------------------
void serial_init(uint32_t base_addr, uint32_t baud_rate)           
{
    uint32_t cfg = 0;
    m_uart = (volatile uint32_t *)base_addr;

    // Soft reset
    cfg += (1 << ULITE_CONTROL_RST_RX_SHIFT);
    cfg += (1 << ULITE_CONTROL_RST_TX_SHIFT);
    cfg += (1 << ULITE_CONTROL_IE_SHIFT);

    m_uart[ULITE_CONTROL/4]  = cfg;
}
//-----------------------------------------------------------------
// serial_putchar: Polled putchar
//-----------------------------------------------------------------
int serial_putchar(char c)
{
    // While TX FIFO full
    while (m_uart[ULITE_STATUS/4] & (1 << ULITE_STATUS_TXFULL_SHIFT))
        ;

    m_uart[ULITE_TX/4] = c;

    return 0;
}
//-------------------------------------------------------------
// serial_putstr: Print string (NULL terminated)
//-------------------------------------------------------------
void serial_putstr(const char *str)
{
    while (*str)
        serial_putchar(*str++);
}
//-----------------------------------------------------------------
// serial_haschar:
//-----------------------------------------------------------------
int serial_haschar(void)
{
    return (m_uart[ULITE_STATUS/4] & (1 << ULITE_STATUS_RXVALID_SHIFT)) != 0;
}
//-----------------------------------------------------------------
// serial_getchar: Read character from UART
//-----------------------------------------------------------------
int serial_getchar(void)
{
    if (serial_haschar())
        return (uint8_t)m_uart[ULITE_RX/4];
    else
        return -1;
}

