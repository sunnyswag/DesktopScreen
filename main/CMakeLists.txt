
idf_component_register(
                        SRCS "app_main.c"


                                "src/net/http_server.c"
                                "src/net/http_request.c"
                                "src/net/wifi_ap_sta.c"
                                "src/net/sntp.c"

                                "src/driver/ft6336.c"
                                "src/driver/screen.c"

                                "src/fs/spiffs.c"

                                "src/system_data.c"

                                "src/hal/timer.c"
                                "src/hal/gpio.c"
                                "src/hal/i2c.c"
                                "src/hal/nvs.c"
                                "src/hal/spi.c"
                                "src/hal/pwm.c"

                                "src/ui/ui_timepage.c"
                                "src/ui/ui_clock.c"
                                "src/ui/ui_page_manage.c"
                                "src/ui/ui_mainpage.c"
                                "src/ui/ui_weatherpage.c"
                                "src/ui/ui_wordpage.c"
                                "src/ui/ui_tomatopage.c"
                                "src/ui/ui_fans.c"

                                "src/data/data_weather.c"
                                "src/data/data_num.c"
                                "src/data/data_icon.c"
                                "src/data/data_page.c"

                                "src/draw/font.c"
                                "src/draw/paint.c"

                        INCLUDE_DIRS 
                                "."
                                "./include"
                                "./include/draw"
                                "./include/data"
                                "./include/hal"
                                "./include/net"
                                "./include/driver"
                                "./include/fs"
                                "./include/ui"

                        EMBED_FILES 
                                "favicon.ico"
                                "setting.html"
                        )
