idf.py set-target esp32s3
idf.py fullclean
idf.py build
idf.py flash monitor

python scripts/release.py nodemcu32-lcd1602