SONOFF_PORT="/dev/cu.wchusbserial1430"

esptool.py -p $SONOFF_PORT erase_flash

esptool.py \
            -p $SONOFF_PORT \
            --baud 230400 \
            write_flash \
            -fs 32m \
            -fm dio \
            -ff 40m \
            0x0 ../../firmware/rboot.bin \
            0x1000 ../../firmware/blank_config.bin \
            0x2000 ./firmware/toggle_fan.bin
