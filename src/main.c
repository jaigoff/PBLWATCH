#include <pebble.h>
#include "main.h"

static Window *s_window;
static TextLayer *s_textlayer_hour;
static TextLayer *s_textlayer_min;
static TextLayer *s_textlayer_month;
static TextLayer *s_textlayer_daynumber;
static TextLayer *s_textlayer_dayletter;
static TextLayer *s_textlayer_steps;
static TextLayer *s_textlayer_distance;
static TextLayer *s_textlayer_sleep;
static Layer *s_canvas_layer_battery;
static Layer *s_canvas_layer_calendar;

static BitmapLayer *s_bitmap_layer_bluetooth;
static BitmapLayer *s_bitmap_layer_charging;
static GBitmap *s_bitmap_bluetooth_ok;
static GBitmap *s_bitmap_bluetooth_ko;
static GBitmap *s_bitmap_charging;

static int16_t ibatterysize;
static uint8_t ubatterycharge;
GColor C_COLOR_TEXT_HOUR;
GColor C_COLOR_TEXT_MIN;
GColor C_COLOR_BACKGROUNG_HOUR;
GColor C_COLOR_BACKGROUNG_MIN;
const int16_t C_REC_BATTERY=17;





//health
static void health_showsteps(TextLayer *textlayer) {
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check data is available
  HealthServiceAccessibilityMask result = health_service_metric_accessible(HealthMetricStepCount, start, end);
  if(result & HealthServiceAccessibilityMaskAvailable) {
    // Data is available! Read it
    HealthValue steps = health_service_sum(HealthMetricStepCount, start, end);
        
    
    static char s_steps[1024];
    snprintf(s_steps, sizeof(s_steps),"Steps: %d" , (int)steps);
    text_layer_set_text(textlayer, s_steps);
    APP_LOG(APP_LOG_LEVEL_INFO, "Steps: %d", (int)steps);
    
  }  
  else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "No data available!");
  }
}

static void health_showdistance(TextLayer *textlayer) {
  time_t start = time_start_of_today();
  time_t end = time(NULL);

  // Check data is available
  HealthServiceAccessibilityMask result = health_service_metric_accessible(HealthMetricWalkedDistanceMeters, start, end);
  if(result & HealthServiceAccessibilityMaskAvailable) {
    // Data is available! Read it
    HealthValue distanceMeters = health_service_sum(HealthMetricWalkedDistanceMeters, start, end);
    float distanceKm = (int)distanceMeters/1000;
    
    static char s_steps[1024];
    snprintf(s_steps, sizeof(s_steps),"Dist: %d.%d km" , (int)distanceKm, (int)(distanceKm)%100 );
    text_layer_set_text(textlayer, s_steps);
    //APP_LOG(APP_LOG_LEVEL_INFO, "Distance: %d",distanceKm);
    
  }  
  else {
      APP_LOG(APP_LOG_LEVEL_ERROR, "No data available!");
  }
}


static void health_handler(HealthEventType event, void *context) {
  // Which type of event occured?
  switch(event) {
    case HealthEventSignificantUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventSignificantUpdate event");
      break;
    case HealthEventMovementUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventMovementUpdate event");
      break;
    case HealthEventSleepUpdate:
      APP_LOG(APP_LOG_LEVEL_INFO, 
              "New HealthService HealthEventSleepUpdate event");
      break;
  }
}



//Bluetooth
static void bt_handler(bool isConnected){
  if(isConnected)
  {
     bitmap_layer_set_bitmap(s_bitmap_layer_bluetooth, s_bitmap_bluetooth_ok);  
     APP_LOG(APP_LOG_LEVEL_INFO,"Bluetooth connected");
  }
  else{
    bitmap_layer_set_bitmap(s_bitmap_layer_bluetooth, s_bitmap_bluetooth_ko); 
    APP_LOG(APP_LOG_LEVEL_INFO,"Bluetooth disconnected");
  }
}
//Draw calendar
static void drawcalendarstroke (Layer *layer, GContext *ctx ){
  graphics_context_set_stroke_color(ctx,GColorBlack);
  graphics_draw_line(ctx,GPoint(79,99+26),GPoint(79,168));
  graphics_draw_line(ctx,GPoint(143,99+26),GPoint(143,168));
}
//Draw the battery status
static void drawbattery(Layer *layer, GContext *ctx ){
  GRect bounds = layer_get_bounds(layer);
  //background
    graphics_context_set_fill_color(ctx,C_COLOR_BACKGROUNG_HOUR);
  graphics_fill_rect(ctx, GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),0,65,11),0,GCornerNone );
  //stroke battery
  graphics_context_set_stroke_color(ctx,C_COLOR_TEXT_HOUR);
  graphics_draw_rect(ctx, GRect(bounds.size.w-24,2,20,10) );
  
  GColor batteryColor= GColorGreen;
  //Select color for battery
  if(ubatterycharge>=20&& ubatterycharge<41)
  {
    batteryColor= GColorYellow;
  }
  else if(ubatterycharge<20)
  {
    batteryColor= GColorRed;
  }
  graphics_context_set_fill_color(ctx, batteryColor);
 
  graphics_fill_rect(ctx, GRect(bounds.size.w-23,4,ibatterysize,6), 1, GCornerNone);
  //add min rect at right
  graphics_context_set_fill_color(ctx, C_COLOR_TEXT_HOUR);
  graphics_fill_rect(ctx, GRect(bounds.size.w-4,5,3,3),3, GCornersRight);
  
  
}

//Calculate rectangle width for the battery
static void calculateBatterySize(uint8_t charge){
  ibatterysize=(charge*C_REC_BATTERY)/100;
  ubatterycharge=charge;
}

//show charging
static void showCharging(bool ischarging){
  Layer *rlayer=  bitmap_layer_get_layer(s_bitmap_layer_charging);
  if(ischarging)
  {
     layer_set_hidden(rlayer,0);
  }
  else
  {
    layer_set_hidden(rlayer,1);
  }
}

//Battery handler when the battery status change
static void battery_handler(BatteryChargeState new_state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Battery %d%%",new_state.charge_percent);
  calculateBatterySize(new_state.charge_percent);
  layer_mark_dirty(s_canvas_layer_battery);
  //battery charging
  showCharging(new_state.is_charging);
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
    //update health
    health_showsteps(s_textlayer_steps);
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
  APP_LOG(APP_LOG_LEVEL_INFO,"w %d h%d", bounds.size.w, bounds.size.h);
  //create TextLayer
  s_textlayer_hour=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),11, 65 ,50 ));
  
  text_layer_set_background_color(s_textlayer_hour, C_COLOR_BACKGROUNG_HOUR);
  text_layer_set_text_color(s_textlayer_hour, C_COLOR_TEXT_HOUR);
  text_layer_set_text(s_textlayer_hour, "");
  text_layer_set_font(s_textlayer_hour, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_textlayer_hour, GTextAlignmentCenter);
  
  
  
  s_textlayer_min=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),53, 65 , 54 ));
  
  text_layer_set_background_color(s_textlayer_min, C_COLOR_BACKGROUNG_MIN);
  text_layer_set_text_color(s_textlayer_min, C_COLOR_TEXT_MIN);
  text_layer_set_text(s_textlayer_min, "");
  text_layer_set_font(s_textlayer_min, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(s_textlayer_min, GTextAlignmentCenter);
  
  
  s_textlayer_month=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),107, 65 , 18 ));
  text_layer_set_background_color(s_textlayer_month, GColorRed);
  text_layer_set_text_color(s_textlayer_month, GColorWhite);
  text_layer_set_text(s_textlayer_month, "");
  text_layer_set_font(s_textlayer_month, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_textlayer_month, GTextAlignmentCenter);
  
  s_textlayer_dayletter=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),99+26, 65 ,18 ));
  text_layer_set_background_color(s_textlayer_dayletter, GColorWhite);
  text_layer_set_text_color(s_textlayer_dayletter, GColorBlack);
  text_layer_set_text(s_textlayer_dayletter, "");
  text_layer_set_font(s_textlayer_dayletter, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_textlayer_dayletter, GTextAlignmentCenter);
  
  s_textlayer_daynumber=text_layer_create(GRect(PBL_IF_ROUND_ELSE(bounds.size.w-59,bounds.size.w-65),99+18+26-7, 65 , 35 ));
  text_layer_set_background_color(s_textlayer_daynumber, GColorWhite);
  text_layer_set_text_color(s_textlayer_daynumber, GColorBlack);
  text_layer_set_text(s_textlayer_daynumber, "");
  text_layer_set_font(s_textlayer_daynumber, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_textlayer_daynumber, GTextAlignmentCenter);
  
  //add text to the window
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_min));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_hour));
 
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_month));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_daynumber));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_dayletter));
 
  
  // Create draw Layer
  s_canvas_layer_battery = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, s_canvas_layer_battery);

  s_canvas_layer_calendar = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_add_child(window_layer, s_canvas_layer_calendar);
  
  //Create bitmap layer
  s_bitmap_bluetooth_ok = gbitmap_create_with_resource(RESOURCE_ID_BT_OK);
  s_bitmap_bluetooth_ko = gbitmap_create_with_resource(RESOURCE_ID_BT_KO);
  s_bitmap_layer_bluetooth = bitmap_layer_create(GRect(75, 0, 24, 20));
  bitmap_layer_set_compositing_mode(s_bitmap_layer_bluetooth, GCompOpSet);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_bluetooth));
   // Show current connection state
  bt_handler(connection_service_peek_pebble_app_connection());
  
  //battery charging
  s_bitmap_charging = gbitmap_create_with_resource(RESOURCE_ID_IMG_CHARGING);
  s_bitmap_layer_charging = bitmap_layer_create(GRect(109, 0, 15, 15));
  bitmap_layer_set_compositing_mode(s_bitmap_layer_charging, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer_charging, s_bitmap_charging); 
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_charging));
  
  //health part
  #if defined(PBL_HEALTH)
  s_textlayer_steps=text_layer_create(GRect(0,100, 70 ,20 ));
  text_layer_set_background_color(s_textlayer_steps, GColorWhite);
  text_layer_set_text_color(s_textlayer_steps, GColorBlack);
  text_layer_set_text(s_textlayer_steps, "");
  text_layer_set_font(s_textlayer_steps, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_textlayer_steps, GTextAlignmentLeft);
  
  s_textlayer_distance=text_layer_create(GRect(0,115, 70 ,20 ));
  text_layer_set_background_color(s_textlayer_distance, GColorWhite);
  text_layer_set_text_color(s_textlayer_distance, GColorBlack);
  text_layer_set_text(s_textlayer_distance, "");
  text_layer_set_font(s_textlayer_distance, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_textlayer_distance, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_steps));
  layer_add_child(window_layer, text_layer_get_layer(s_textlayer_distance));
  #endif
  // Set the update_proc
  layer_set_update_proc(s_canvas_layer_battery, drawbattery);
  layer_set_update_proc(s_canvas_layer_calendar,drawcalendarstroke);
}

//Unload window and so destroy everythings added in
static void main_window_unload(Window *window) {
  //destroy time
  text_layer_destroy(s_textlayer_hour);
  text_layer_destroy(s_textlayer_min);
  text_layer_destroy(s_textlayer_month);
  text_layer_destroy(s_textlayer_dayletter);
  text_layer_destroy(s_textlayer_daynumber);
  #if defined(PBL_HEALTH)
  text_layer_destroy(s_textlayer_steps);
  text_layer_destroy(s_textlayer_distance);
  #endif
    // Destroy Layer
  layer_destroy(s_canvas_layer_battery);
  layer_destroy(s_canvas_layer_calendar);
  
   bitmap_layer_destroy(s_bitmap_layer_bluetooth);
   bitmap_layer_destroy(s_bitmap_layer_charging);
   gbitmap_destroy(s_bitmap_bluetooth_ok);
   gbitmap_destroy(s_bitmap_bluetooth_ko);
   gbitmap_destroy(s_bitmap_charging);
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
  showCharging(bcs.is_charging);
  
  //Register
  // Subscribe to Bluetooth updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bt_handler
  });
  
  //Health registration
  #if defined(PBL_HEALTH)
  health_showsteps(s_textlayer_steps);
  health_showdistance(s_textlayer_distance);
  // Attempt to subscribe 
 // if(!health_service_events_subscribe(health_handler, NULL)) {
   // APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  //}
  #else
  APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  #endif
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

