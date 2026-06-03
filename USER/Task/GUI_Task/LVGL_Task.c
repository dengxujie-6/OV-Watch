#include "LVGL_Task.h"

#include "cmsis_os2.h"
#include "lvgl.h"
#include "main.h"

#define LVGL_TASK_DELAY_MS 5U

static osThreadId_t lvglTaskHandle;

static const osThreadAttr_t lvglTaskAttributes = {
    .name = "lvglTask",
    .stack_size = 1024 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};

static void LVGL_Task(void *argument);
static void LVGL_Demo(void);
void lv_port_disp_init(void);

void LVGL_Task_Create(void)
{
    lvglTaskHandle = osThreadNew(LVGL_Task, NULL, &lvglTaskAttributes);
}

static void LVGL_Task(void *argument)
{
    (void)argument;

    lv_init();
    lv_tick_set_cb(HAL_GetTick);
    lv_port_disp_init();
    LVGL_Demo();

    for(;;) {
        (void)lv_timer_handler();
        osDelay(LVGL_TASK_DELAY_MS);
    }
}

/**
  * @brief Create a small LVGL screen for display bring-up.
  */
static void LVGL_Demo(void)
{
    lv_obj_t *screen = lv_screen_active();

    lv_obj_set_style_bg_color(screen, lv_color_hex(0x101820), LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(screen);
    lv_label_set_text(label, "LVGL Ready");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);
}
