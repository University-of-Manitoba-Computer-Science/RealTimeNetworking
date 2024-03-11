#include "sam.h"
#include "wifi.h"
#include "spi.h"
#include "time.h"
#include "dcc_stdio.h"

uint8_t gu8crc_off = 1;

err_t init_wifi()
{
    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA06;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA22;
    delay_ms(100);
    PORT_REGS->GROUP[0].PORT_OUTCLR = PORT_PA22;
    delay_ms(100);

    return wifi8_init_drv();
}

err_t wifi8_init_drv()
{
    if (WIFI8_OK != wifi8_hold())
    {
        return WIFI8_ERROR;
    }
    else
    {

        return wifi8_start();
    }
}

err_t wifi8_hold()
{
    if (WIFI8_OK != wifi8_init_communication())
    {
        return WIFI8_ERROR;
    }
    return WIFI8_OK;

    // line 1717
}

err_t wifi8_start()
{
    // line 1753
    return WIFI8_OK;
}

err_t wifi8_init_communication()
{
    uint32_t chipid;
    uint32_t reg = 0;

    gu8crc_off = 0;

    if (wifi8_reg_read(NMI_SPI_PROTOCOL_CONFIG, &reg) < 0)
    {
        /* Read failed. Try with CRC off. This might happen when module
        is removed but chip isn't reset*/
        gu8crc_off = 1;
        if (wifi8_reg_read(NMI_SPI_PROTOCOL_CONFIG, &reg) < 0)
        {
            // Read failed with both CRC on and off, something went bad
            return WIFI8_ERROR;
        }
    }
    if (gu8crc_off == 0)
    {
        reg &= ~0xc; /* disable crc checking */
        reg &= ~0x70;
        reg |= (0x5 << 4);

        if (wifi8_reg_write(NMI_SPI_PROTOCOL_CONFIG, reg) < 0)
        {
            return WIFI8_ERROR;
        }
        gu8crc_off = 1;
    }

    /**
        make sure can read back chip id correctly
    **/
    if (wifi8_reg_read(0x1000, &chipid) < 0)
    {
        return WIFI8_ERROR;
    }

    uint32_t val32;

    /* Make sure SPI max. packet buf_len fits the defined DATA_PKT_SZ.  */
    wifi8_reg_read(0xe800 + 0x24, &val32);
    val32 &= ~(0x7 << 4);
    switch (WIFI8_DATA_PKT_SZ)
    {
    case 256:
    {
        val32 |= (0 << 4);
        break;
    }
    case 512:
    {
        val32 |= (1 << 4);
        break;
    }
    case 1024:
    {
        val32 |= (2 << 4);
        break;
    }
    case 2048:
    {
        val32 |= (3 << 4);
        break;
    }
    case 4096:
    {
        val32 |= (4 << 4);
        break;
    }
    case 8192:
    {
        val32 |= (5 << 4);
        break;
    }
    }

    wifi8_reg_write(0xe800 + 0x24, val32);

    return WIFI8_OK;
}

err_t wifi8_reg_read(uint32_t addr, uint32_t *data_out)
{
    err_t result = WIFI8_OK;
    uint8_t cmd = WIFI8_CMD_SINGLE_READ;
    uint8_t tmp[4];
    uint8_t clockless = 0;

    dbg_write_str("wifi8_reg_read\n");

    if (addr <= 0xff)
    {
        cmd = WIFI8_CMD_INTERNAL_READ;
        clockless = 1;
    }

    for (uint8_t retry = SPI_RETRY_COUNT; retry != 0; retry--)
    {
        if (result != WIFI8_OK)
        {
            delay_1ms();
            spi_cmd(WIFI8_CMD_RESET, 0, 0, 0, 0);
            spi_cmd_rsp(WIFI8_CMD_RESET);
            delay_1ms();
        }
        result = spi_cmd(cmd, addr, 0, 4, clockless);
        if (result != WIFI8_OK)
        {
            continue;
        }
        // printf("wifi8_reg_read cmd_rsp ready\n");
        result = spi_cmd_rsp(cmd);
        // printf("wifi8_reg_read cmd_rsp result: %d\n", result);
        if (result != WIFI8_OK)
        {
            continue;
        }
        // printf("wifi8_reg_read retry 6\n");
        result = spi_data_read(tmp, 4, clockless);
        if (result != WIFI8_OK)
        {
            dbg_write_str("wifi8_reg_read retry 8\n");
            continue;
        }

        *data_out = tmp[0] |
                    ((uint32_t)tmp[1] << 8) |
                    ((uint32_t)tmp[2] << 16) |
                    ((uint32_t)tmp[3] << 24);

        if (result == WIFI8_OK)
        {
            dbg_write_str("wifi8_reg_read retry 9\n");
            break;
        }
        dbg_write_str("wifi8_reg_read retry 10\n");
    }

    return result;
}

err_t wifi8_reg_write(uint32_t addr, uint32_t data_in)
{
    err_t result = WIFI8_OK;
    uint8_t cmd = WIFI8_CMD_SINGLE_WRITE;
    uint8_t clockless = 0;

    if (addr <= 0x30)
    {
        cmd = WIFI8_CMD_INTERNAL_WRITE;
        clockless = 1;
    }

    for (uint8_t retry = SPI_RETRY_COUNT; retry != 0; retry--)
    {
        if (result != WIFI8_OK)
        {
            delay_1ms();
            spi_cmd(WIFI8_CMD_RESET, 0, 0, 0, 0);
            spi_cmd_rsp(WIFI8_CMD_RESET);
            delay_1ms();
        }

        result = spi_cmd(cmd, addr, data_in, 4, clockless);
        if (result != WIFI8_OK)
        {
            continue;
        }

        result = spi_cmd_rsp(cmd);
        if (result == WIFI8_OK)
        {
            break;
        }
    }

    return result;
}

err_t spi_cmd(uint8_t cmd, uint32_t adr, uint32_t data_in, uint32_t sz, uint8_t clockless)
{
    printf("spi_cmd\n");
    uint8_t bc[9];
    uint8_t len = 5;
    err_t result = WIFI8_OK;

    bc[0] = cmd;
    switch (cmd)
    {
    case WIFI8_CMD_SINGLE_READ: /* single word (4 bytes) read */
    {
        bc[1] = (uint8_t)(adr >> 16);
        bc[2] = (uint8_t)(adr >> 8);
        bc[3] = (uint8_t)(adr);
        len = 5;
        break;
    }
    case WIFI8_CMD_INTERNAL_READ: /* internal register read */
    {
        bc[1] = (uint8_t)(adr >> 8);
        if (clockless)
            bc[1] |= (1 << 7);
        bc[2] = (uint8_t)adr;
        bc[3] = 0x00;
        len = 5;
        break;
    }
    case WIFI8_CMD_RESET: /* reset */
    {
        bc[1] = 0xff;
        bc[2] = 0xff;
        bc[3] = 0xff;
        len = 5;
        break;
    }
    case WIFI8_CMD_DMA_EXT_WRITE: /* dma extended write */
    case WIFI8_CMD_DMA_EXT_READ:  /* dma extended read */
    {
        bc[1] = (uint8_t)(adr >> 16);
        bc[2] = (uint8_t)(adr >> 8);
        bc[3] = (uint8_t)(adr);
        bc[4] = (uint8_t)(sz >> 16);
        bc[5] = (uint8_t)(sz >> 8);
        bc[6] = (uint8_t)(sz);
        len = 8;
        break;
    }
    case WIFI8_CMD_INTERNAL_WRITE: /* internal register write */
    {
        bc[1] = (uint8_t)(adr >> 8);
        if (clockless)
            bc[1] |= (1 << 7);
        bc[2] = (uint8_t)(adr);
        bc[3] = (uint8_t)(data_in >> 24);
        bc[4] = (uint8_t)(data_in >> 16);
        bc[5] = (uint8_t)(data_in >> 8);
        bc[6] = (uint8_t)(data_in);
        len = 8;
        break;
    }
    case WIFI8_CMD_SINGLE_WRITE: /* single word write */
    {
        bc[1] = (uint8_t)(adr >> 16);
        bc[2] = (uint8_t)(adr >> 8);
        bc[3] = (uint8_t)(adr);
        bc[4] = (uint8_t)(data_in >> 24);
        bc[5] = (uint8_t)(data_in >> 16);
        bc[6] = (uint8_t)(data_in >> 8);
        bc[7] = (uint8_t)(data_in);
        len = 9;
        break;
    }
    default:
    {
        result = WIFI8_ERROR;
        break;
    }
    }

    if (WIFI8_OK == result)
    {
        if (!gu8crc_off)
        {
            // bc[len - 1] = (crc7(0x7f, (uint8_t *)&bc[0], len - 1)) << 1;
            bc[len - 1] = 0;
        }
        else
        {
            len -= 1;
        }

        if (spi_write(bc, len) < 0)
        {
            result = WIFI8_ERROR;
        }
    }

    return result;
}

err_t spi_cmd_rsp(uint8_t cmd)
{
    printf("spi_cmd_rsp\n");
    uint8_t rsp;
    err_t result = WIFI8_OK;
    int8_t s8RetryCnt;

    s8RetryCnt = 10;
    do
    {
        if (WIFI8_OK != spi_read(&rsp, 1))
        {
            return WIFI8_ERROR;
        }
        printf("spi_cmd_rsp rsp: %d\n", rsp);
    } while ((rsp != cmd) && (s8RetryCnt-- > 0));

    if (s8RetryCnt < 0)
    {
        return WIFI8_ERROR;
    }

    s8RetryCnt = 10;
    do
    {
        if (WIFI8_OK != spi_read(&rsp, 1))
        {
            return WIFI8_ERROR;
        }
    } while ((rsp != 0x00) && (s8RetryCnt-- > 0));

    if (s8RetryCnt < 0)
    {
        return WIFI8_ERROR;
    }

    return result;
}

err_t spi_data_read(uint8_t *b, uint16_t sz, uint8_t clockless)
{
    printf("spi_data_read\n");
    int16_t retry, ix, nbytes;
    uint8_t crc[2];
    uint8_t rsp;

    ix = 0;
    do
    {
        if (sz <= WIFI8_DATA_PKT_SZ)
        {
            nbytes = sz;
        }
        else
        {
            nbytes = WIFI8_DATA_PKT_SZ;
        }

        retry = 10;
        do
        {
            if (spi_read(&rsp, 1) < 0)
            {
                return WIFI8_ERROR;
            }

            if (((rsp >> 4) & 0xf) == 0xf)
            {
                if ((rsp & 0xf0) == 0xf0)
                {
                    break;
                }
            }
        } while (retry--);

        if (retry <= 0)
        {
            return WIFI8_ERROR;
        }

        if (spi_read(&b[ix], nbytes) < 0)
        {
            return WIFI8_ERROR;
        }

        if (!clockless)
        {
            if (!gu8crc_off)
            {
                if (spi_read(crc, 2) < 0)
                {
                    return WIFI8_ERROR;
                }
            }
        }

        ix += nbytes;
        sz -= nbytes;

    } while (sz);

    return WIFI8_OK;
}