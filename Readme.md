/home/aleksander/.espressif/v6.1-beta1/esp-idf

idf.py build

idf.py -p /dev/your_actual_port flash

ls -l /dev/tty[UA]*

sudo chmod a+rw /dev/ttyACM0
sudo usermod -aG dialout,plugdev $USER

. /home/aleksander/.espressif/v6.1-beta1/esp-idf/export.sh

idf.py set-target esp32s3
idf.py add-dependency "espressif/esp32-camera"

idf.py reconfigure
idf.py build