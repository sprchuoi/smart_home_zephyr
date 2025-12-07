# Smart Home On Development #
## Structure
    software/app/src/
    ├── main.c                           (initialization only)
    │
    ├── modules/                         (Library layer - core functionality)
    │   ├── blink/
    │   │   ├── blink_module.h          (config & API)
    │   │   └── blink_module.c          (LED control logic)
    │   ├── sensor/
    │   │   ├── sensor_module.h         (config & API)
    │   │   └── sensor_module.c         (sensor reading logic)
    │   └── ble/
    │       ├── ble_module.h            (config & API)
    │       └── ble_module.c            (BLE communication logic)
    │
    └── thread/                          (Thread layer - orchestration)
        ├── blink_task.c/h              (runs blink module in thread)
        ├── sensor_task.c/h             (runs sensor module in thread)
        └── ble_task.c/h                (runs BLE module in thread)
    '''
## To Do with software
    cd software
### Complete workflow
    ./make.sh setup        # One time
    ./make.sh config       # Edit WiFi credentials
    ./make.sh all          # Build and flash
    ./make.sh monitor      # Watch output
### If have issue with hal espressif    
    west blobs fetch hal_espressif