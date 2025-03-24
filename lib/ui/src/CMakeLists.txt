SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_Screen3.c
    screens/ui_Screen7.c
    screens/ui_Screen8.c
    screens/ui_Screen4.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_i03n_t_png.c
    images/ui_img_qr_png.c
    fonts/ui_font_NanumGothic.c)

add_library(ui ${SOURCES})
