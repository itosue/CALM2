#include "pebble.h"
#include "string.h"
#include "stdlib.h"


#define MINUTE_H 64	//長針の長さ
#define GOLDEN_R 0.618	//黄金比の値(短針の長さに使用
#define NUM_CLOCK_TICKS 12 //文字盤の点の数
	
	
// 文字盤の時間を示す点	
static const struct GPathInfo ANALOG_BG_POINTS[] = {
	//1
  { 4, (GPoint []){
      {104, 7},
      {111, 11},
      {107, 17},
      {100, 13}
    }
  },
	//2
  { 4, (GPoint []){
      {126, 36},
      {133, 31},
      {137, 37},
      {130, 42}
    }
  },
  //3
  { 4, (GPoint []){
      {143, 68},
      {143, 74},
      {136, 74},
      {136, 68}
    }
  },
	//4
	{ 4, (GPoint []){
      {129, 99},
      {135, 102},
      {131, 108},
      {126, 105}
    }
  },
	//5
  { 4, (GPoint []){
      {108, 125},
      {112, 131},
      {105, 136},
      {101, 129}
    }
  },
	//6
  { 4, (GPoint []){
      {68, 137},
      {76, 137},
      {76, 142},
      {68, 142}
    }
  },
  //7
	{ 4, (GPoint []){
      {36, 125},
      {32, 132},
      {41, 136},
      {44, 129}
    }
  },
	//8
	{ 4, (GPoint []){
      {13, 100},
      {8, 103},
      {12, 110},
      {17, 106}
    }
  },
  //9
  { 4, (GPoint []){
      {1, 68},
      {8, 68},
      {8, 74},
      {1, 74}
    }
  },
	//10
	{ 4, (GPoint []){
      {18, 37},
      {11, 34},
      {9, 40},
      {16, 43}
    }
  },
		//11
  { 4, (GPoint []){
      {39, 7},
      {33, 11},
      {37, 17},
      {43, 13}
    }
  },
	//12
	{ 4, (GPoint []){
      {68, 0},
      {76, 0},
      {76, 5},
      {68, 5}
    }
  },
};



Layer *bg_layer; //背景レイヤー
Layer *date_layer; //曜日レイヤー
TextLayer *day_label; //曜日用Textレイヤー
char day_buffer[6]; //曜日
TextLayer *num_label; //日付用Textレイヤー


static GPath *minute_arrow; //長針描画情報
static GPath *hour_arrow; //短針描画情報
static GPath *tick_paths[NUM_CLOCK_TICKS]; //文字盤の点描画情報
static GPath *center_sqr; //針の軸に置く四角の描画情報
static Layer *hands_layer; //ハンドルのレイヤー
TextLayer *day_label;
static TextLayer *title_text_layer;

static Window *window; //Window

static Layer *anim_layer;
static PropertyAnimation *prop_animation;
static int toggle;

static GPoint battop; //バッテリーインジケータの上点
static GPoint batbottom; //バッテリーインジケータの下点
static int batnum = 100; //バッテリーの値
static bool btcon = true; //Bluetooth接続状態
static GBitmap *image; //各種画像ハンドリング用
static BitmapLayer *image_layer;  //画像表示用

static Layer *bt_layer;

char num_buffer[4]; //整数デバッグ表示用バッファー
TextLayer *str_label; //デバッグ用文字列表示用Textレイヤー
char str_buffer[4]; //デバッグ出力用文字列
static int dbgnum = 0;


// 数字を渡されたらデジタル表記用の画像ハンドラを返す関数
static GBitmap* rtnImg(int num){
	GBitmap *img=NULL;
	if ( num == 0){
		img = gbitmap_create_with_resource(RESOURCE_ID_0);
	}
	else if (num == 1){
		img = gbitmap_create_with_resource(RESOURCE_ID_1);
	}
	else if (num == 2){
		img = gbitmap_create_with_resource(RESOURCE_ID_2);
	}
	else if (num == 3){
		img = gbitmap_create_with_resource(RESOURCE_ID_3);
	}
	else if (num == 4){
		img = gbitmap_create_with_resource(RESOURCE_ID_4);
	}
	else if (num == 5){
		img = gbitmap_create_with_resource(RESOURCE_ID_5);
	}
	else if (num == 6){
		img = gbitmap_create_with_resource(RESOURCE_ID_6);
	}
	else if (num == 7){
		img = gbitmap_create_with_resource(RESOURCE_ID_7);
	}
	else if (num == 8){
		img = gbitmap_create_with_resource(RESOURCE_ID_8);
	}
	else if (num == 9){
		img = gbitmap_create_with_resource(RESOURCE_ID_9);
	}	
	return img;
}


// バッテリー周りのハンドリング
static void handle_battery(BatteryChargeState charge_state) {
  batnum = charge_state.charge_percent;
}

// Bluetooth切断時の画面描画やバイブ
static void bt_update_proc(Layer *layer, GContext *ctx) {

	if(!btcon){
		graphics_context_set_compositing_mode	(	ctx,GCompOpOr);
		vibes_long_pulse();	
		psleep(1000);
		graphics_draw_bitmap_in_rect(ctx, gbitmap_create_with_resource(RESOURCE_ID_NOISE_WHITE) , (GRect) { .origin = { 0, 0 }, .size = { 144,168 } });
		gbitmap_destroy(image);
	}

}

// Bluetooth接続周りのハンドリング
static void handle_bluetooth(bool connected) {
	btcon = connected;
}

// 背景処理 ＃ctxの扱い次第で初期化処理のどっかに入れれるかも？
static void bg_update_proc(Layer *layer, GContext *ctx) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "tic");

		GRect bounds = layer_get_bounds(layer);
	  graphics_context_set_fill_color(ctx, GColorBlack);
	  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	  graphics_context_set_fill_color(ctx, GColorWhite);
	
	  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
			gpath_draw_filled(ctx, tick_paths[i]);		
		}

}

// 針をアップデートする処理 ＃多くの物が針の動きと同期してるので、ここの記述が多いです
static void sec_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
	GPoint center = grect_center_point(&bounds);
	center.x= center.x;
	center.y = center.x - 1;

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
	int anglem = t->tm_min * 6;
	int angleh = ((((t->tm_hour) % 12) * 30) + t->tm_min / 2);

	graphics_context_set_fill_color(ctx, GColorWhite);
	int hand_w = 4;
	
	if ((anglem > 0 && anglem < 60) || (anglem > 180 && anglem < 240)){
			hand_w = 5;
			
	}
		else if (anglem == 90 || anglem == 270){
			hand_w = 3;			
	}
	GPathInfo MHP = {
	  4,
		(GPoint []){
 	   {  hand_w, 0 },
 	   {  hand_w, MINUTE_H * -1 },
 	   { hand_w * -1, MINUTE_H * -1 },
		{ hand_w * -1, 0 }
	  }
  };
	
	minute_arrow = gpath_create(&MHP);
	gpath_move_to(minute_arrow, center);
	gpath_rotate_to(minute_arrow, anglem * TRIG_MAX_ANGLE / 360);
  gpath_draw_filled(ctx, minute_arrow);
  gpath_destroy(minute_arrow);	

	hand_w = 4;
	if ((angleh > 0 && angleh <= 60) || (angleh > 180 && angleh <= 240)){
			hand_w = 5;
	}
		else if (angleh == 90 || angleh == 270){
			hand_w = 3;			
	}	
  
	GPathInfo HHP = {
    4,
		(GPoint []){
  	  { hand_w, 0 },
	    { hand_w, MINUTE_H*GOLDEN_R * -1 },
	    { hand_w * -1, MINUTE_H*GOLDEN_R * -1 },
			{ hand_w * -1, 0 }
	  }
  };	

	hour_arrow = gpath_create(&HHP);
	gpath_move_to(hour_arrow, center);

  gpath_rotate_to(hour_arrow, angleh * TRIG_MAX_ANGLE / 360);
	gpath_draw_filled(ctx, hour_arrow);
	
	gpath_destroy(hour_arrow);
	
	// Drawing a digital numbers
	int m1 = t->tm_min % 10;
	int m10 = t->tm_min / 10;
	int h1 = t->tm_hour % 10;
	int h10 = t->tm_hour /10;

	image = rtnImg(m1);
	graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 128, 143 }, .size = { 16,25 } });
	gbitmap_destroy(image);
	image = rtnImg(m10);
	graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 110, 143 }, .size = { 16,25 } });
	gbitmap_destroy(image);
	image = rtnImg(h1);
	graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 90, 143 }, .size = { 16,25 } });
	gbitmap_destroy(image);
	if (h10 != 0){
		image = rtnImg(h10);
		graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 72, 143 }, .size = { 16,25 } });
		gbitmap_destroy(image);
	}
	
	
	
	// 時報 
	if (t->tm_min % 60 == 0 && t->tm_sec == 0){
		vibes_double_pulse();
		psleep(900);
	}
	vibes_cancel();
	
	
	// 真ん中の四角
	GPathInfo CNTR = {
  4,
	(GPoint []){
    { 68, 69 },
    { 68, 74 },
    { 75, 74 },
		{ 75, 69 }
  }
  };	
	center_sqr = gpath_create(&CNTR);
	gpath_draw_filled(ctx, center_sqr);
	gpath_destroy(center_sqr);
	
	// バッテリー関連の値取得と表示
	handle_battery(battery_state_service_peek());
	battop.x = 0;
	if(batnum == 60){
		battop.y = 68;
	}
	else {
		battop.y = (100 - batnum) * 168 / 100;
	}
	batbottom.x = 0;
	batbottom.y = 167;
	graphics_context_set_stroke_color(ctx,GColorWhite);
	graphics_draw_line(ctx,battop,batbottom);	

	

	//APP_LOG(APP_LOG_LEVEL_DEBUG, "%d%%",batnum);
}

// 日付更新時の処理
static void date_update_proc(Layer *layer, GContext *ctx) {
	
	
}

// 日付表示処理
static void anim_update_proc(Layer *layer, GContext *ctx) {
		GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

	time_t now = time(NULL);
  struct tm *t = localtime(&now);
	int date1 = t->tm_mday % 10;
	int date10 = t->tm_mday / 10;
	graphics_context_set_compositing_mode(ctx,GCompOpAssignInverted);
	image = rtnImg(date1);
	graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 53, 0 }, .size = { 16,25 } });
	//gbitmap_destroy(image);
	if(date10 != 0){
	image = rtnImg(date10);
	graphics_draw_bitmap_in_rect(ctx, image, (GRect) { .origin = { 36, 0 }, .size = { 16,25 } });
	//gbitmap_destroy(image);	
	}
  
	strftime(day_buffer, sizeof(day_buffer), "%a", t);
  text_layer_set_text(day_label, day_buffer);
}

static void animation_started(Animation *animation, void *data) {

}

static void animation_stopped(Animation *animation, bool finished, void *data) {

	psleep(3000);
	GRect to_rect;
	to_rect = GRect(1, 186, 71, 25);
	//to_rect = GRect(1, 143, 71, 25);
//	animation_set_curve(&prop_animation->animation, AnimationCurveEaseIn);
	animation_set_curve(property_animation_get_animation(prop_animation), AnimationCurveEaseIn);
	prop_animation = property_animation_create_layer_frame(anim_layer, NULL, &to_rect);
	animation_schedule((Animation*) prop_animation);
}

static void destroy_property_animation(PropertyAnimation **prop_animation) {
  if (*prop_animation == NULL) {
    return;
  }
  if (animation_is_scheduled((Animation*) *prop_animation)) {
    animation_unschedule((Animation*) *prop_animation);
  }
  property_animation_destroy(*prop_animation);
  *prop_animation = NULL;
}

static void handle_tap(AccelAxisType axis, int32_t direction){
	
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
	
  GRect to_rect;
	to_rect = GRect(1, 143, 71, 25);


  destroy_property_animation(&prop_animation);
		
	prop_animation = property_animation_create_layer_frame(anim_layer, NULL, &to_rect);
  animation_set_duration(property_animation_get_animation(prop_animation), 200);
  animation_set_curve(property_animation_get_animation(prop_animation), AnimationCurveEaseOut);

	animation_set_handlers((Animation*) prop_animation, (AnimationHandlers) {

	.started = (AnimationStartedHandler) animation_started,
    .stopped = (AnimationStoppedHandler) animation_stopped,
  }, NULL /* callback data */);
	if(btcon){
		animation_schedule((Animation*) prop_animation);	
	}
 
}

// Windowを更新用に定義（？）
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(window));
}

// Window初期化時の処理
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
	
  // init layers　背景レイヤーをProcにアタッチ
  bg_layer = layer_create(bounds);
  layer_set_update_proc(bg_layer, bg_update_proc);
  layer_add_child(window_layer, bg_layer);

	// 針レイヤーをウィンドウに追加してプロシージャをセット
	hands_layer = layer_create(bounds);
  layer_set_update_proc(hands_layer, sec_update_proc);
	layer_add_child(window_layer, hands_layer);

	// 加速度センサー有効化
	accel_tap_service_subscribe	(handle_tap);	

	// init clock face paths
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }
	
	
	//アニメーションレイヤー
	anim_layer = layer_create(GRect(0, 0, 72, 25));
  layer_set_update_proc(anim_layer,anim_update_proc);
	
	day_label = text_layer_create(GRect(0, 4, 90, 25));
  text_layer_set_text(day_label, day_buffer);
  text_layer_set_background_color(day_label, GColorWhite);
  text_layer_set_text_color(day_label, GColorBlack);
  GFont norm18 = fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21);
  text_layer_set_font(day_label, norm18);
	layer_add_child(anim_layer, text_layer_get_layer(day_label));
	
	layer_add_child(window_layer, anim_layer);
	GRect to_rect = GRect(1, 168, 71, 25);
//	GRect to_rect = GRect(1, 143, 71, 25);
  prop_animation = property_animation_create_layer_frame(anim_layer, NULL, &to_rect);
  animation_schedule((Animation*) prop_animation);
	
	// Bluetoothエラー時用のハンドラとレイヤーを登録
	bt_layer = layer_create(bounds);
	layer_set_update_proc(bt_layer, bt_update_proc);
	layer_add_child(window_layer, bt_layer);
	
}

// WindowExit時の後処理
static void window_unload(Window *window) {
  layer_destroy(bg_layer);
  layer_destroy(date_layer);
  text_layer_destroy(day_label);
  text_layer_destroy(num_label);
  layer_destroy(hands_layer);
	layer_destroy(anim_layer);
}

//初期化
static void init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  // Push the window onto the stack
  const bool animated = true;
  window_stack_push(window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

	battery_state_service_subscribe(&handle_battery);
	bluetooth_connection_service_subscribe(&handle_bluetooth);

}

// 後処理
static void deinit(void) {
  gpath_destroy(minute_arrow);
  gpath_destroy(hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
  window_destroy(window);
}

// エントリーポイント
int main(void) {
  init();
  app_event_loop();
  deinit();
}