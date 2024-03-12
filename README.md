# CAN Bus Project - Embedded Real Time Networking

## Landon Colburn

## To build and install (in Docker):

Run

```
make docker-install
```

.elf is located at `./bin/blinky.elf`, and was installed using OpenOCD in Docker.

## To build and install (on host):

Run

```
make install
```

.elf is located at `./bin/blinky.elf`, and was installed using OpenOCD on the host.

## Wifi Click

maximum clock frequency of 48MHz

| Pin Name  | Pin Number | Pin Function | Pin Description     |
| --------- | ---------- | ------------ | ------------------- |
| SPI0_EN   | PA06       | EN           | Chip Enable         |
| SPI0_RST  | PA22       | RST          | Reset               |
| SPI0_CS   | PA23       | CS           | Chip Select         |
| SPI0_SCK  | PA17       | SCK          | Serial Clock        |
| SPI0_MISO | PA19       | MISO/SDO     | Master In Slave Out |
| SPI0_MOSI | PA16       | MOSI/SDI     | Master Out Slave In |
| SPI0_INT  | PA09       | INT          | Interrupt           |

## Mikroe HAL SPI Functions

```c
static void _hal_ll_spi_master_write_bare_metal(hal_ll_spi_master_hw_specifics_map_t *map, uint8_t *write_data_buffer, size_t write_data_length){

    hal_ll_spi_master_base_handle_t *hal_ll_hw_reg = (hal_ll_spi_master_base_handle_t *)map->base;
    size_t loop_counter = 0;

    /* Clear all interrupts. */
    hal_ll_hw_reg->rser &= ~(SPI_MASTER_RSER_FULL_MASK);

    for (loop_counter = 0; loop_counter < write_data_length; loop_counter++ ){

        /* Flush TXFIFO. */
        hal_ll_hw_reg->mcr |= ( 1UL << SPI_MASTER_MCR_DIS_TXF_SHIFT );

        hal_ll_hw_reg->sr = 1UL << SPI_MASTER_SR_TCF_SHIFT;

        hal_ll_hw_reg->pushr = SPI_MASTER_CFG_CMD_CTCNT_CLEAR |  ((uint32_t) write_data_buffer[loop_counter]);

        while((hal_ll_hw_reg->sr & SPI_MASTER_SR_TCF_MASK) == 0)
        {
        }

        hal_ll_hw_reg->mcr &= ~( 1UL << SPI_MASTER_MCR_DIS_TXF_SHIFT );
    }

}

static void _hal_ll_spi_master_read_bare_metal(hal_ll_spi_master_hw_specifics_map_t *map, uint8_t *read_data_buffer, size_t read_data_length, uint8_t dummy_data){

    hal_ll_spi_master_base_handle_t *hal_ll_hw_reg = (hal_ll_spi_master_base_handle_t *)map->base;
    size_t i = 0;

    /* Clear all interrupts. */
    hal_ll_hw_reg->rser &= ~(SPI_MASTER_RSER_FULL_MASK);

    for( i=0; i < read_data_length; i++ ) {
        set_reg_bit(&hal_ll_hw_reg->mcr, SPI_MASTER_MCR_HALT_SHIFT);

        hal_ll_hw_reg->mcr = ( hal_ll_hw_reg->mcr  & ( ~( SPI_MASTER_MCR_CLR_RXF_MASK | SPI_MASTER_MCR_CLR_TXF_MASK ) ) ) |
                             ( 1UL << SPI_MASTER_MCR_CLR_RXF_SHIFT ) | ( 1UL << SPI_MASTER_MCR_CLR_TXF_SHIFT );

        /* Flush RXFIFO. */
        hal_ll_hw_reg->mcr |= ( 1UL << SPI_MASTER_MCR_DIS_RXF_SHIFT );

        clear_reg_bit( &hal_ll_hw_reg->mcr,SPI_MASTER_MCR_HALT_SHIFT );

        hal_ll_hw_reg->sr = SPI_MASTER_SR_TCF_MASK | SPI_MASTER_SR_EOQF_MASK | SPI_MASTER_SR_TFUF_MASK |
                            SPI_MASTER_SR_TFFF_MASK | SPI_MASTER_SR_RFOF_MASK | SPI_MASTER_SR_RFDF_MASK;

        /* Set to 0 always, because one byte is transferred. */
        hal_ll_hw_reg->tcr &= SPI_MASTER_TCR_TCNT_MASK;

        hal_ll_hw_reg->mcr &= ~( 1UL << SPI_MASTER_MCR_DIS_RXF_SHIFT );
        clear_reg_bit( &hal_ll_hw_reg->mcr,SPI_MASTER_MCR_HALT_SHIFT );

        _hal_ll_spi_master_write_bare_metal( map, &dummy_data, sizeof( dummy_data ) );

        // Wait for RFDF to be set.
        while((hal_ll_hw_reg->sr & SPI_MASTER_SR_RFDF_MASK) == 0);

        read_data_buffer[ i ] = (uint8_t)hal_ll_hw_reg->popr;

        hal_ll_hw_reg->sr = 1UL << SPI_MASTER_SR_RFDF_SHIFT;
    }
}
```
