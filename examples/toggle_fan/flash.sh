SONOFF_PORT="/dev/cu.wchusbserial14310"

esptool.py -p $SONOFF_PORT erase_flash

esptool.py \
            -p $SONOFF_PORT \
            --baud 115200 \
            write_flash \
            -fs 32m \
            -fm dio \
            -ff 40m \
            0x0 ../../rboot.bin \
            0x1000 ../../blank_config.bin \
            0x2000 ./firmware/toggle_fan.bin
