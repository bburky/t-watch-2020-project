/*
Copyright (c) 2019 lewis he
This is just a demonstration. Most of the functions are not implemented.
The main implementation is low-power standby. 
The off-screen standby (not deep sleep) current is about 4mA.
Select standard motherboard and standard backplane for testing.
Created by Lewis he on October 10, 2019.
*/

#ifndef __GUI_H
#define __GUI_H

typedef enum {
    LV_ICON_BAT_EMPTY,
    LV_ICON_BAT_1,
    LV_ICON_BAT_2,
    LV_ICON_BAT_3,
    LV_ICON_BAT_FULL,
    LV_ICON_CHARGE,
    LV_ICON_CALCULATION
} lv_icon_battery_t;

typedef enum {
    LV_STATUS_BAR_BATTERY_LEVEL = 0,
    LV_STATUS_BAR_BATTERY_ICON = 1,
    LV_STATUS_BAR_WIFI = 2,
    LV_STATUS_BAR_BLUETOOTH = 3,
} lv_icon_status_bar_t;

class Switch
{
public:
    typedef struct {
        const char *name;
        void (*cb)(uint8_t, bool);
    } switch_cfg_t;
    typedef void (*exit_cb)();
    Switch();
    ~Switch();
    void create(switch_cfg_t *cfg, uint8_t count, exit_cb cb, lv_obj_t *parent = nullptr);
    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0);
    void hidden(bool en = true);
    static void __switch_event_cb(lv_obj_t *obj, lv_event_t event);
    void setStatus(uint8_t index, bool en);
private:
    static Switch *_switch;
    lv_obj_t *_swCont = nullptr;
    uint8_t _count;
    lv_obj_t **_sw = nullptr;
    switch_cfg_t *_cfg = nullptr;
    lv_obj_t *_exitBtn = nullptr;
    exit_cb _exit_cb = nullptr;
};
class MBox
{
public:
    MBox();
    ~MBox();
    void create(const char *text, lv_event_cb_t event_cb, const char **btns = nullptr, lv_obj_t *par = nullptr);
    void setData(void *data);
    void *getData();
    void setBtn(const char **btns);
private:
    lv_obj_t *_mbox = nullptr;
};

class Keyboard
{
public:
    typedef enum {
        KB_EVENT_OK,
        KB_EVENT_EXIT,
    } kb_event_t;
    typedef void (*kb_event_cb)(kb_event_t event);
    Keyboard();
    ~Keyboard();
    void create(lv_obj_t *parent =  nullptr);
    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0);
    static void __kb_event_cb(lv_obj_t *kb, lv_event_t event);
    void setKeyboardEvent(kb_event_cb cb);
    const char *getText();
    void hidden(bool en = true);
private:
    lv_obj_t *_kbCont = nullptr;
    kb_event_cb _cb = nullptr;
    static const char *btnm_mapplus[10][23];
    static Keyboard *_kb;
    static char __buf[128];
};

class Preload
{
public:
    Preload();
    ~Preload();
    void create(lv_obj_t *parent = nullptr);
    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0);
    void hidden(bool en = true);
private:
    lv_obj_t *_preloadCont = nullptr;
};

class List
{
public:
    typedef void(*list_event_cb)(const char *);
    List();
    ~List();
    void create(lv_obj_t *parent = nullptr);
    void add(const char *txt, void *imgsrc = (void *)LV_SYMBOL_WIFI);
    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0);
    void hidden(bool en = true);
    static void __list_event_cb(lv_obj_t *obj, lv_event_t event);
    void setListCb(list_event_cb cb);
private:
    lv_obj_t *_listCont = nullptr;
    static List *_list ;
    list_event_cb _cb = nullptr;
};

class Task
{
public:
    Task();
    ~Task();
    void create(lv_task_cb_t cb, uint32_t period = 1000, lv_task_prio_t prio = LV_TASK_PRIO_LOW);
private:
    lv_task_t *_handler = nullptr;
    lv_task_cb_t _cb = nullptr;
};

class StatusBar
{
    typedef struct {
        bool vaild;
        lv_obj_t *icon;
    } lv_status_bar_t;
public:
    StatusBar();
    void createIcons(lv_obj_t *par);
    void setStepCounter(uint32_t counter);
    void updateLevel(int level);
    void updateBatteryIcon(lv_icon_battery_t icon);
    void show(lv_icon_status_bar_t icon);
    void hidden(lv_icon_status_bar_t icon);
    uint8_t height();
    lv_obj_t *self();
private:
    void refresh();
    lv_obj_t *_bar = nullptr;
    lv_obj_t *_par = nullptr;
    uint8_t _barHeight = 30;
    lv_status_bar_t _array[6];
    const int8_t iconOffset = -5;
};

class MenuBar
{
public:
    typedef struct {
        const char *name;
        void *img;
        void (*event_cb)();
    } lv_menu_config_t;
    MenuBar();
    ~MenuBar();
    void createMenu(lv_menu_config_t *config, int count, lv_event_cb_t event_cb, int direction = 1);
    lv_obj_t *exitBtn() const;
    lv_obj_t *self() const;
    void hidden(bool en = true);
    lv_obj_t *obj(int index) const;
private:
    lv_obj_t *_cont, *_view, *_exit, * *_obj;
    lv_point_t *_vp ;
    int _count = 0;
};

void setupGui();
void updateStepCounter(uint32_t counter);
void updateBatteryIcon(lv_icon_battery_t index);
void wifi_list_add(const char *ssid);
void wifi_connect_status(bool result);
void updateBatteryLevel();

#endif /*__GUI_H */