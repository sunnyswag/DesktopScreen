# 性能探针组件：只由 tools/perf/device_probe.sh 通过 EXTRA_COMPONENT_DIRS 加入测量固件，正式固件不包含。
# 用链接器 --wrap 截获下面这些函数，业务代码不需要任何改动。
# 某个函数在被测代码里不存在时，对应的包装函数不会被引用，也不影响链接（见 perf_wrap.c 里的 weak 声明）。
idf_component_register(SRCS "perf_probe.c" "perf_wrap.c" "perf_scenario.c" "perf_touch.c"
                       PRIV_REQUIRES esp_timer esp_event esp_netif esp_wifi esp_driver_gpio heap lwip)

set(PERF_PROBE_WRAPS
    app_main
    xTaskCreatePinnedToCore
    xTaskCreatePinnedToCoreWithCaps
    ds_screen_full_display
    ds_screen_full_display_bydata
    ds_screen_refresh
    ds_screen_partial_data_copy
    esp_http_client_perform
    esp_light_sleep_start
    # 触摸模拟（perf_touch.c）
    gpio_isr_handler_add
    gpio_isr_handler_remove
    gpio_get_level
    i2c_master_read_slave)

foreach(sym ${PERF_PROBE_WRAPS})
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=${sym}")
endforeach()

# device_probe.sh 传入：PERF_PROBE_AUTO=1 开启自动剧本；PERF_PROBE_HTTP_LOOPBACK=1 配网页改由设备自己请求
if(PERF_PROBE_AUTO)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PERF_PROBE_AUTO=1)
endif()
if(PERF_PROBE_HTTP_LOOPBACK)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PERF_PROBE_HTTP_LOOPBACK=1)
endif()
