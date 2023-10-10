# STM32MP157

## Getting Started

## Repository Structure and Usage

### Directory layout

    .
    ├── atk-mp1                                      # Source Code for alientek MP175
    |    ├── alientek_tf-a                           # tf-a trusted fireware for A7
    |    |    ├── TF-A_Released_Images               # Released images for tf-a 
    |    |    ├── tf-a-stm32mp-2.2r1                 # tf-a source code provided by Alientek
    ├── tools                                        # tools for compiling, linking and etc.
    ├──                    
    ├──                         
    ├──                           
    │   ├──      
    │   │   ├──                  
    │   │   │   ├──      
    │   │   │   ├──               
    │   │   │   └──               
    │   │   ├──                  
    │   │   ├──                   
    │   │   └── 
    │   └── ...        
    ├──                 
    ├──                     
    ├──                       
    └──                       

## Branch and Release

## Building and using

### Build TF-A

1. tf-a-stm32mp157d-atk-trusted.stm32

    ```bash
    cd tf-a-stm32mp-2.2.r1/
    make -f ../Makefile.sdk all TFA_DEVICETREE=stm32mp157d-atk all
    ```

2. tf-a-stm32mp157d-atk-serialboot.stm32

    - open Makefile.sdk
    - set EXTRA_OEMAKE_SERIAL as below

        ```make
        EXTRA_OEMAKE_SERIAL=$(filter-out STM32MP_SDMMC=1 STM32MP_EMMC=1 STM32MP_SPI_NOR=1 STM32MP_RAW_NAND=1 STM32MP_SPI_NAND=1,$(EXTRA_OEMAKE)) STM32MP_UART_PROGRAMMER=1 STM32MP_USB_PROGRAMMER=1
        ```

        ```bash
        cd tf-a-stm32mp-2.2.r1/
        make -f ../Makefile.sdk all TFA_DEVICETREE=stm32mp157d-atk TF_A_CONFIG=serialboot ELF_DEBUG_ENABLE='1' all
         ```
