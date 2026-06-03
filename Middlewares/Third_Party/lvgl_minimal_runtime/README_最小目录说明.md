# LVGL 最小可运行文件目录说明

本目录是在官方 `lvgl.zip` 基础上删除文档、示例、Demo、测试、CI、脚本等非运行必需内容后形成的轻量目录。

## 目录结构

```text
lvgl_minimal_runtime/
├── lv_conf.h                 # 用户配置文件，由 lv_conf_template.h 启用后生成
├── lvgl/
│   ├── lvgl.h                # LVGL 总入口头文件
│   ├── lvgl_private.h        # 内部头文件入口，一般用户不直接 include
│   ├── lv_conf_template.h    # 配置模板
│   ├── include/              # 公开头文件，对外 API 声明
│   ├── src/                  # LVGL 核心源码实现
│   ├── LICENCE.txt           # 许可证
│   └── README.md             # 官方说明
└── port/
    ├── lv_port_disp.c        # 显示移植占位文件，需要用户补充
    └── lv_port_indev.c       # 输入移植占位文件，需要用户补充
```

## 使用时还需要你自己完成

1. 将 `lvgl/` 加入工程。
2. 将 `lv_conf.h` 所在目录加入头文件搜索路径。
3. 编译 `lvgl/src/` 下需要的 `.c` 文件。
4. 完成显示移植 `lv_port_disp.c`，至少包括 `draw buffer`、`flush_cb()`、`lv_display_set_buffers()`。
5. 如果有触摸屏、按键或编码器，完成输入移植 `lv_port_indev.c`，至少包括 `read_cb()`。
6. 在应用层周期调用 `lv_timer_handler()`。

## 注意

这不是官方完整仓库，只适合作为嵌入式工程集成 LVGL 的轻量源码目录。  
如果要查看官方示例、Demo、文档、测试或脚本，请使用原始完整 `lvgl.zip`。
