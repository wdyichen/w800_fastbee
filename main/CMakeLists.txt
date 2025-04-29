list(APPEND ADD_SRCS "src/main.c"
                     "src/ota.c"
                     "src/wps_app.c"
                     "src/wps_utils.c"
                     "src/led.c"
                     )

set(ADD_DT_C_FILES "dt/wm_dt_hw_w800_arduino.c")
set(ADD_DT_H_FILES "dt")

register_component()
