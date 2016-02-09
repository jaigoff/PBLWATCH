#include <pebble.h>
#include "main.h"

static Window *s_window;
static TextLayer *s_textlayer_hour;
static TextLayer *s_textlayer_min;
static TextLayer *s_textlayer_month;
static TextLayer *s_textlayer_daynumber;
static TextLayer *s_textlayer_dayletter;
static Layer *s_canvas_layer_battery;
static int16_t ibatterysize;
static uint8_t ubatterycharge;
GColor C_COLOR_TEXT_HOUR;
GColor C_COLOR_TEXT_MIN;
GColor C_COLOR_BACKGROUNG_HOUR;
GColor C_COLOR_BACKGROUNG_MIN;
const int16_t C_REC_BATTERY=17;


//Draw the battery status
static void drawbattery(Layer *layer, GContext *ctx ){
  GRect bounds = layer_get_bounds(layer);
  //stroke battery
  graphics_context_set_stroke_color(ctx,C_COLOR_TEXT_HOUR);
  graphics_draw_rect(ctx, GRect(bounds.size.w-24,0,20,10) );
  
  GColor batteryColor= GColorGreen;
  //Select color for battery
  if(ubatterycharge>=20&& ubatterycharge<41)
  {
    batteryColor= GColorOrange;
  }
  else if(ubatterycharge<20)
  {
    batteryColor= GColorRed;
  }
  graphics_context_set_fill_color(ctx, batteryColor);
 
  graphics_fill_rect(ctx, GRect(bounds.size.w-23,2,ibatterysize,6), 1, GCornerNone);
  //add min rect at right
  graphics_context_set_fill_color(ctx, C_COLOR_TEXT_HOUR);
  graphics_fill_rect(ctx, GRect(bounds.size.w-4,3,3,3),3, GCornersRight);
}

//Calculate rectangle width for the battery
static void calculateBatterySize(uint8_t charge){
  ibatterysize=(charge*C_REC_BATTERY)/100;
  ubatterycharge=charge;
}


//Battery handler when the battery status change
static void battery_handler(BatteryChargeState new_state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Battery %d%%",new_state.charge_percent);
  calculateBatterySize(new_state.charge_percent);
  layer_mark_dirty(s_canvas_layer_battery);
}

//Update Month Label
static void updateMonth(struct tm *tick_time){
    static char s_monthbuffer[36];
    strftime(s_monthbuffer, sizeof(s_monthbuffer),"%B" , tick_time);
    text_layer_set_text(s_textlayer_month, s_monthbuffer); 
}

//Update Day Label and Number
static void updateDay(struct tm *tick_time){
    static char s_daynumberbuffer[8];
    strftime(s_daynumberbuffer, sizeof(s_daynumberbuffer),"%e" , tick_time);
    text_layer_set_text(s_textlayer_daynumber, s_daynumberbuffer); 
  
    static char s_dayletterbuffer[36];
    strftime(s_dayletterbuffer, sizeof(s_dayletterbuffer),"%A" , tick_time);
    text_layer_set_text(s_textlayer_dayletter, s_dayletterbuffer); 
}

//Update Hour Label
static void updateHour(struct tm *tick_time){
    static char s_hourbuffer[8];
    strftime(s_hourbuffer, sizeof(s_hourbuffer), clock_is_24h_style() ?
                                          "%H" : "%I", tick_time);
    // Display this time on the TextLayer
     text_layer_set_text(s_textlayer_hour,s_hourbuffer);   
}

//Update Minute Label
static void updateMin(struct tm *tick_time){
    static char s_minbuffer[8];
    strftime(s_minbuffer, sizeof(s_minbuffer),"%M" , tick_time);
    text_layer_set_text(s_textlayer_min, s_minbuffer);
    
}

//Tick handler when time changes
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tm_tick_time = localtime(&temp);

    /*APP_LOG(APP_LOG_LEVEL_DEBUG,"Second %d",SECOND_UNIT);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Minute %d",MINUTE_UNIT);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Hour %d",HOUR_UNIT);
    APP_LOG(APP_LOG_LEVEL_DEBUG,"TimeUnit %d",units_changed);*/
  
  if(units_changed&MINUTE_UNIT)
  {
    updateMin(tm_tick_time);
    //APP_LOG(APP_LOG_LEVEL_DEBUG,"Minute Changed %d",units_changed);
  }
  if(units_changed&HOUR_UNIT)
  {
    updateHour(tm_tick_time);
  }
  if(units_changed&MONTH_UNIT){
    updateMonth(tm_tick_time);
  }
  if(units_changed&DAY_UNIT){
    updateDay(tm_tick_time);
  }
  if(units_changed==0)//first load
  {
    updateMin(tm_tick_time);
    updateHour(tm_tick_time);
    updateMonth(tm_tick_time);
    updateDay(tm_tick_time);
  }
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"TimeUnit END %d",units_changed);
}

//Load the window 
static void main_window_load(Window *window) {
   
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //create TextLayer
  s_textlayer_hour=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),0 , 65 , bounds.size.h/3 ));
  
  text_layer_set_background_color(s_textlayer_hour, C_COLOR_BACKGROUNG_HOUR);
  text_layer_set_text_color(s_textlayer_hour, C_COLOR_TEXT_HOUR);
  text_layer_set_text(s_textlayer_hour, "HH");
  text_layer_set_font(s_textlayer_hour, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_textlayer_hour, GTextAlignmentRight);
  
  
  
  s_textlayer_min=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),bounds.size.h/3 , 65 , bounds.size.h/3 ));
  
  text_layer_set_background_color(s_textlayer_min, C_COLOR_BACKGROUNG_MIN);
  text_layer_set_text_color(s_textlayer_min, C_COLOR_TEXT_MIN);
  text_layer_set_text(s_textlayer_min, "00");
  text_layer_set_font(s_textlayer_min, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_textlayer_min, GTextAlignmentRight);
  
  
  s_textlayer_month=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),(bounds.size.h/3)*2 , 65 , bounds.size.h/9 ));
  text_layer_set_background_color(s_textlayer_month, GColorRed);
  text_layer_set_text_color(s_textlayer_month, GColorWhite);
  text_layer_set_text(s_textlayer_month, "MM");
  text_layer_set_font(s_textlayer_month, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_textlayer_month, GTextAlignmentCenter);
  
  s_textlayer_dayletter=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),(bounds.size.h/3)*2+bounds.size.h/9, 65 , bounds.size.h/9 ));
  text_layer_set_background_color(s_textlayer_dayletter, GColorWhite);
  text_layer_set_text_color(s_textlayer_dayletter, GColorBlack);
  text_layer_set_text(s_textlayer_dayletter, "Wednesday");
  text_layer_set_font(s_textlayer_dayletter, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_textlayer_dayletter, GTextAlignmentCenter);
  
  s_textlayer_daynumber=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),(bounds.size.h/3)*2+(bounds.size.h/9)*2, 65 , bounds.size.h/9 ));
  text_layer_set_background_color(s_textlayer_daynumber, GColorWhite);
  text_layer_set_text_color(s_textlayer_daynumber, GColorBlack);
  text_layer_set_text(s_textlayer_daynumber, "31");
  text_layer_set_font(s_textlayer_daynumber, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_textlayer_daynumber, GTextAlignmentCenter);
  
  //add text to the window
  
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_hour));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_min));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_month));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_dayletter));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_daynumber));
  
  // Create Layer
  s_canvas_layer_battery = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, s_canvas_layer_battery);

  // Set the update_proc
  layer_set_update_proc(s_canvas_layer_battery, drawbattery);
}

//Unload window and so destroy everythings added in
static void main_window_unload(Window *window) {
  //destroy time
  text_layer_destroy(s_textlayer_hour);
  text_layer_destroy(s_textlayer_min);
  text_layer_destroy(s_textlayer_month);
  text_layer_destroy(s_textlayer_dayletter);
  text_layer_destroy(s_textlayer_daynumber);
    // Destroy Layer
  layer_destroy(s_canvas_layer_battery);
}

//init for the program, creat window and get watch event
static void init(){
    //init color
  C_COLOR_TEXT_HOUR=GColorWhite;
  C_COLOR_TEXT_MIN=GColorWhite;
  C_COLOR_BACKGROUNG_HOUR=GColorDarkGray;
  C_COLOR_BACKGROUNG_MIN=GColorDarkGray;
  //init locale
  setlocale(LC_ALL, i18n_get_system_locale());
  APP_LOG(APP_LOG_LEVEL_INFO,"Local %s", i18n_get_system_locale());
  s_window=window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);
  BatteryChargeState bcs=battery_state_service_peek();
  calculateBatterySize(bcs.charge_percent);
  

}

//End watch face
static void deinit(){
  window_destroy(s_window);
}

//main program for the watch
int main(void) {
  init();
  app_event_loop();
  deinit();
}

