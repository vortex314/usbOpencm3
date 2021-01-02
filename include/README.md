# Goal
- Run FreeRTOS and opencm3 on a maple st32f103 
- use MQTT serial to connect to a Raspberry Pi as central controller
- have one protocol to drive diverse sensors and actuators and bring it together in a single view
# references to get this build
- Link Opencm3 to FreeRTOS on STM32 : 
https://community.platformio.org/t/translating-make-file-for-freertos-libopencm3-to-platformio-ini/11177/12
- Extract FreeRTOS from github https://github.com/FreeRTOS/FreeRTOS-Kernel
- take the GCC related files
https://github.com/FreeRTOS/FreeRTOS-Kernel/tree/main/portable/GCC/ARM_CM3
- adapt some in FreeRTOSConfig.h
```
#define configCPU_CLOCK_HZ          ( ( unsigned long ) SystemCoreClock )
#define configTICK_RATE_HZ          ( ( portTickType ) 1000 )
#define configTOTAL_HEAP_SIZE       ( ( size_t ) TotalHeapSize )
```
This makes it configurable from main code. 
- add limero framework to make it fly ;-)



