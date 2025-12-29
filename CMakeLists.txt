
idf_component_register(
    SRCS
        "main.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        esp32_a2dp
        bt
        fatfs
        sdmmc
        vfs
        driver
		esp_ringbuf
)
