SONOFF_PORT="/dev/cu.wchusbserial1420"

esptool.py -p $SONOFF_PORT erase_flash

esptool.py \
            -p $SONOFF_PORT \
            --baud 460800 \
            write_flash \
            -fs 32m \
            -fm dio \
            -ff 40m \
            0x0 ../../rboot.bin \
            0x1000 ../../blank_config.bin \
            0x2000 ./firmware/toggle_fan.bin
