void GP_PAGE_TITLE(const String& name) {
  GP.PAGE_TITLE(((settings.namePrefix) ? (settings.name + String(" - ")) : "") + name + ((settings.namePostfix) ? (String(" - ") + settings.name) : ""));
}
void GP_LABEL_BLOCK_W(const String& val, const String& name = "", PGM_P st = GP_GREEN, int size = 0, bool bold = 0) {
  GP.TAG_RAW(F("label class='display'"), val, name, GP_WHITE, size, bold, 0, st);
}
String GP_FLOAT_DEC(float val, uint16_t dec) {
  String data = "";
  if (!dec) data += (int)round(val);
  else data += String(val, (uint16_t)dec);
  return data;
}
void GP_SLIDER_MAX(const String& lable, const String& min_lable, const String& max_lable, const String& name, float value = 0, float min = 0, float max = 100, float step = 1, uint8_t dec = 0, PGM_P st = GP_GREEN, bool dis = 0, bool oninp = 0) {
  String data = "";
  data += F("<lable style='color:#fff;position:relative;z-index:1;left:17px;bottom:1px;width:0px;pointer-events:none'");
  if (dis) data += F(" class='dsbl'");
  data += '>';
  data += lable;
  data += F("</lable>\n<input type='range' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += F("' value='");
  data += value;
  data += F("' min='");
  data += min;
  data += F("' max='");
  data += max;
  data += F("' step='");
  data += step;
  data += F("' style='background-image:linear-gradient(");
  data += FPSTR(st);
  data += ',';
  data += FPSTR(st);
  data += F(");background-size:0% 100%;height:30px;width:100%;max-width:430px;margin:10px 4px;border-radius:20px;box-shadow:0 0 15px rgba(0, 0, 0, 0.7)' onload='GP_change(this)' ");
  if (oninp) data += F("oninput='GP_change(this);GP_click(this)'");
  else data += F("onchange='GP_click(this)' oninput='GP_change(this)'");
  data += F(" onmousewheel='GP_wheel(this);GP_change(this);GP_click(this)' ");
  if (dis) data += F("class='dsbl' disabled");
  data += F(">\n<output align='center' id='");
  data += name;
  data += F("_val' name='");
  data += min_lable;
  data += ',';
  data += max_lable;
  data += F("' style='position:relative;right:70px;margin-right:-55px;background:none;display:inline-flex;justify-content:end;pointer-events:none'");
  if (dis) data += F(" class='dsbl'");
  data += F(">");
  data += GP_FLOAT_DEC(value, dec);
  data += F("</output>\n");
  GP.SEND(data);
}
String GP_SPINNER_BTN(const String& name, float step, PGM_P st, uint8_t dec, bool dis) {
  String data = "";
  data += F("<input type='button' class='spinBtn ");
  data += (step > 0) ? F("spinR") : F("spinL");
  data += F("' name='");
  data += name;
  data += F("' min='");
  data += step;
  data += F("' max='");
  data += dec;
  data += F("' onmouseleave='if(_pressId)clearInterval(_spinInt);_spinF=_pressId=null' onmousedown='_pressId=this.name;_spinInt=setInterval(()=>{GP_spin(this);_spinF=1},");
  data += 200;
  data += F(")' onmouseup='clearInterval(_spinInt)' onclick='if(!_spinF)GP_spin(this);_spinF=0' value='");
  data += (step > 0) ? '+' : '-';
  data += F("' ");
  if (st != GP_GREEN) {
    data += F(" style='background:");
    data += FPSTR(st);
    data += F(";'");
  }
  if (dis) data += F(" disabled");
  data += F(">\n");
  return data;
}
void GP_SPINNER_MAIN(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  String data = "";
  data += F("<div id='spinner' class='spinner'>\n");
  data += GP_SPINNER_BTN(name, -step, st, dec, dis);
  data += F("<input type='number' name='");
  data += name;
  data += F("' id='");
  data += name;
  if (w.length()) {
    data += F("' style='width:");
    data += w;
  }
  data += F("' step='");
  data += GP_FLOAT_DEC(step, dec);
  data += F("' onkeyup='GP_spinw(this)' onkeydown='GP_spinw(this)' onchange='");
  if (!dec) data += F("GP_spinc(this);");
  data += F("GP_click(this);GP_spinw(this)' value='");
  data += GP_FLOAT_DEC(value, dec);
  if (!isnan(min)) {
    data += F("' min='");
    data += GP_FLOAT_DEC(min, dec);
  }
  if (!isnan(max)) {
    data += F("' max='");
    data += GP_FLOAT_DEC(max, dec);
  }
  data += F("' ");
  if (dis) data += F("disabled ");
  if (!w.length()) data += F("class='spin_inp'");
  data += F(">\n");
  data += GP_SPINNER_BTN(name, step, st, dec, dis);
  data += F("</div>\n");
  GP.SEND(data);
}
void GP_SPINNER_MID(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-left:-10px;margin-right:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_SPINNER_LEFT(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-left:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_SPINNER_RIGHT(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  GP.SEND("<div style='margin-right:-10px;'>\n"); GP_SPINNER_MAIN(name, value, min, max, step, dec, st, w, dis); GP.SEND("</div>\n");
}
void GP_BUTTON_MINI_LINK(const String& url, const String& text, PGM_P color) {
  GP.SEND(String("<button class='miniButton' style='background:") + FPSTR(color) + ";line-height:100%;' onclick='location.href=\"" + url + "\";'>" + text + "</button>\n");
}
void GP_TEXT_LINK(const String& url, const String& text, const String& id, PGM_P color) {
  String data = "";
  data += F("<style>a:link.");
  data += id;
  data += F("_link{color:");
  data += FPSTR(color);
  data += F(";text-decoration:none;} a:visited.");
  data += id;
  data += F("_link{color:");
  data += FPSTR(color);
  data += F(";} a:hover.");
  data += id;
  data += F("_link{filter:brightness(0.75);}</style>\n<a href='");
  data += url;
  data += F("' class='");
  data += id + "_link";
  data += F("'>");
  data += text;
  data += F("</a>\n");
  GP.SEND(data);
}
void GP_UI_LINK(const String& url, const String& name, PGM_P color) {
  String data = "";
  data += F("<a href='http://");
  data += url;
  data += "'";
  if (WiFi.localIP().toString().equals(url)) {
    data += F(" class='sbsel' style='background:");
    data += FPSTR(color);
    data += F(" !important;'");
  }
  data += ">";
  data += name;
  data += F("</a>\n");
  GP.SEND(data);
}
void GP_CHECK_ICON(const String& name, const String& uri, bool state = 0, int size = 30, PGM_P st_0 = GP_GRAY, PGM_P st_1 = GP_GREEN, bool dis = false) {
  String data = "";
  data += F("<style>#__");
  data += name;
  data += F(" span::before{background-color:");
  data += FPSTR(st_0);
  data += F(";border:none;padding:");
  data += (size / 2) + 2;
  data += F("px;}</style>\n");

  data += F("<style>#__");
  data += name;
  data += F(" input:checked+span::before{background-color:");
  data += FPSTR(st_1);
  data += F(";background-image:none;}</style>\n");

  data += F("<label id='__");
  data += name;
  data += F("' class='check_c'");
  data += F(" style='-webkit-mask:center/contain no-repeat url(");
  data += uri;
  data += F(");display:inline-block;width:");
  data += size;
  data += F("px;'>");
  data += F("<input type='checkbox' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += "' ";
  if (state) data += F("checked ");
  if (dis) data += F("disabled ");
  data += F("onclick='GP_click(this)' style='height:");
  data += size;
  data += F("px;'><span></span></label>\n"
            "<input type='hidden' value='0' name='");
  data += name;
  data += "'>\n";
  GP.SEND(data);
}
void GP_HR(PGM_P st, int width = 0) {
  String data = "";
  data += F("<hr style='border-color:");
  data += FPSTR(st);
  data += F(";margin:5px ");
  data += width;
  data += F("px'>\n");
  GP.SEND(data);
}
void GP_HR_TEXT(const String& text, const String& name, PGM_P st_0, PGM_P st_1) {
  String data = "";

  data += F("<label id='");
  data += name;
  data += F("' class='thinText' style='color:");
  data += FPSTR(st_0);
  data += F("'>");
  data += text;
  data += F("</label>\n");
  data += F("<hr style='border-color:");
  data += FPSTR(st_1);
  data += F(";margin-top:-17px;padding-bottom:17px'>\n");

  GP.SEND(data);
}
void GP_LINE_LED(const String& name, bool state = 0, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  String data = "";

  data += F("<style>#__");
  data += name;
  data += F(" input:checked+span::before{background-color:");
  data += FPSTR(st_1);
  data += F(";background-image:none}\n");

  data += F("#__");
  data += name;
  data += F(" span::before{background-color:");
  data += FPSTR(st_0);
  data += F(";border:none;display:inline-block;width:100px;height:0px;cursor:default;filter:brightness(1)!important;box-shadow:0 0 15px rgba(0, 0, 0, 0.7);}\n");

  data += F("#__");
  data += name;
  data += F(" input[type=checkbox]{cursor:default;}</style>\n");

  data += F("<label id='__");
  data += name;
  data += F("' class='check_c' style='display:block;height:30px;margin-top:-13px;cursor:default'><input type='checkbox' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += "' ";
  if (state) data += F("checked ");
  data += F("disabled ");
  data += F("onclick='GP_click(this)'><span></span></label>\n"
            "<input type='hidden' value='0' name='");
  data += name;
  data += "'>\n";
  GP.SEND(data);
}
void GP_LINE_BAR(const String& name, int value = 0, int min = 0, int max = 100, int step = 1, PGM_P st = GP_GREEN) {
  String data = "";

  data += F("<input type='range' name='");
  data += name;
  data += F("' id='");
  data += name;
  data += F("' value='");
  data += value;
  data += F("' min='");
  data += min;
  data += F("' max='");
  data += max;
  data += F("' step='");
  data += step;
  data += F("' style='filter:brightness(1);box-shadow:0 0 15px rgba(0, 0, 0, 0.7);background-color:#1a1a1a;background-image:linear-gradient(");
  data += FPSTR(st);
  data += ',';
  data += FPSTR(st);
  data += F(");background-size:0% 100%;display:block;width:124px;height:8px;margin-top:3px;margin-bottom:6px;cursor:default' onload='GP_change(this)' disabled>\n");

  data += F("<output style='display:none' id='");
  data += name;
  data += F("_val'></output>\n");

  GP.SEND(data);
}
void GP_PLOT_STOCK_BEGIN(boolean local = 0) {
  String data = "";
  if (local) data += F("<script src='/gp_data/PLOT_STOCK.js'></script>\n<script src='/gp_data/PLOT_STOCK_DARK.js'></script>\n");
  else data += F("<script src='https://code.highcharts.com/stock/highstock.js'></script>\n<script src='https://code.highcharts.com/themes/dark-unica.js'></script>\n");
  GP.SEND(data);
}
void GP_PLOT_STOCK_ADD(uint32_t time, int16_t val, uint8_t dec) {
  String data = "";
  data += '[';
  data += time;
  data += F("000");
  data += ',';
  if (dec) data += (float)val / dec;
  else data += val;
  data += "],\n";
  GP.SEND(data);
}
void GP_PLOT_STOCK_DARK(const String& id, const char** labels, uint32_t* times, int16_t* vals_0, int16_t* vals_1, uint8_t size, uint8_t dec = 0, uint16_t height = 400, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  String data = "";

  data += F("<div class='chartBlock' style='width:95%;height:");
  data += height;
  data += F("px' id='");
  data += id;
  data += F("'></div>");

  data += F("<script>Highcharts.setOptions({colors:['");
  data += FPSTR(st_0);
  data += F("','");
  data += FPSTR(st_1);
  data += F("']});\nHighcharts.stockChart('");
  data += id;
  data += F("',{chart:{},\n"
            "rangeSelector:{buttons:[\n"
            "{count:1,type:'minute',text:'1M'},\n"
            "{count:1,type:'hour',text:'1H'},\n"
            "{count:1,type:'day',text:'1D'},\n"
            "{type:'all',text:'All'}],\n"
            "inputEnabled:false,selected:3},\n"
            "time:{useUTC:true},\n"
            "credits:{enabled:false},series:[\n"
           );

  if (vals_0 != NULL) {
    data += F("{name:'");
    data += labels[0];
    data += F("',data:[\n");
    GP.SEND(data);
    data = "";
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_0[s], dec);
    }
    data += "]},\n";
  }
  if (vals_1 != NULL) {
    data += F("{name:'");
    data += labels[1];
    data += F("',data:[\n");
    GP.SEND(data);
    data = "";
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_1[s], dec);
    }
    data += "]},\n";
  }
  data += F("]});</script>\n");
  GP.SEND(data);
}
void GP_BLOCK_SHADOW_BEGIN(void) {
  GP.SEND(F("<div style='box-shadow:0 0 15px rgb(0 0 0 / 45%);border-radius:25px;margin:5px 10px 5px 10px;'>\n"));
}
void GP_BLOCK_SHADOW_END(void) {
  GP.SEND(F("</div>\n"));
}
void GP_FOOTER_BEGIN(void) {
  GP.SEND("<div style='flex-grow:1;display:block;padding:0px;'></div>\n<footer>");
}
void GP_FOOTER_END(void) {
  GP.SEND("</footer>");
}
void GP_BUILD_END(void) {
  GP.SEND(F("</div>\n<div id='onlBlock' class='onlBlock'>Нет соединения</div>\n"));
  GP.JS_BOTTOM();
  GP.PAGE_END();
}
void GP_FIX_SCRIPTS(void) {
  GP.SEND(F(
            "<script>var _err=0;\n"
            "function GP_send(req,r=null,upd=null){\n"
            "var xhttp=new XMLHttpRequest();\n"
            "xhttp.open(upd?'GET':'POST',req,true);\n"
            "xhttp.send();\n"
            "xhttp.timeout=_tout;\n"
            "xhttp.onreadystatechange=function(){\n"
            "if(this.status||(++_err>=5)){onlShow(!this.status);_err=0;}\n"
            "if(this.status||upd){\n"
            "if(this.readyState==4&&this.status==200){\n"
            "if(r){\n"
            "if(r==1)location.reload();\n"
            "else location.href=r;}\n"
            "if(upd)GP_apply(upd,this.responseText);}}}}\n"
            "function GP_spinc(arg){\n"
            "if (arg.className=='spin_inp'){\n"
            "arg.value-=arg.value%arg.step;}}\n"
            "function GP_change(arg){\n"
            "arg.style.backgroundSize=(arg.value-arg.min)*100/(arg.max-arg.min)+'% 100%';\n"
            "const _output=getEl(arg.id+'_val');\n"
            "const _range=_output.name.split(',');\n"
            "if((arg.value<=Number(arg.min))&&_range[0]){_output.value=_range[0];}\n"
            "else if((arg.value>=Number(arg.max))&&_range[1]){_output.value=_range[1];}\n"
            "else _output.value=arg.value;}</script>\n"
          )
         );
}
void GP_FIX_STYLES(void) {
  GP.SEND(F(
            "<style>.headbar{z-index:3;}\n" //фикс меню в мобильной версии
            ".onlBlock{z-index:3;background:#810000bf;width:15px;height:180px;border-radius:25px 0 0 25px;writing-mode:vertical-lr;text-align:center;}\n" //фикс плашки офлайн
            ".display{border-radius:5px;}\n" //фикс лейбл блоков
            ".sblock{display:flex;flex-direction:column;min-height:98%;margin:0;}\n" //фикс меню
            ".sblock>a{border-radius:25px;}\n" //фикс кнопок меню
            ".spinBtn{font-size:24px!important;padding-left:3.5px;padding-top:0.5px;}\n" //фикс кнопок спинера
            ".check_c>span::before{border-color:#444;background-color:#2a2d35}\n" //фикс чекбоксов
            ".check_c>input:checked+span::before{border-color:#e67b09;background-color:#e67b09}\n" //фикс чекбоксов
            ".miniButton{padding:1px 7px;}\n" //фикс кнопок
            "input[type='submit'],input[type='button'],button{line-height:90%;border-radius:28px;}\n" //фикс кнопок
            "input[type='text'],input[type='password'],input[type='time'],input[type='date'],select,textarea{text-align:center;appearance:none;}\n" //фикс положения текста
            "input[type='time'],input[type='date']{height:34px;border:none!important;}\n" //фикс выбора времени и даты
            "input[type='number']{text-align:center;}\n" //фикс ввода чисел
            "input[type=range]:disabled{filter:brightness(0.6);}\n" //фикс слайдеров
            "input[type=range]::-moz-range-thumb{-moz-appearance:none;border:none;height:0px;width:0px;}\n" //фикс слайдеров
            "output{min-width:50px;border-radius:5px;}\n" //фикс слайдеров
            "select:disabled{filter:brightness(0.6);}\n" //фикс выпадающего списка
            "select{width:200px;cursor:pointer;}\n" //фикс выпадающего списка
            "#ubtn {min-width:34px;border-radius:25px;line-height:160%;}\n" //фикс кнопок загрузки
            "#grid .block{margin:15px 10px;}</style>\n" //фикс таблицы
            "<style type='text/css'>@media screen and (max-width:1100px){\n.grid{display:block;}\n#grid .block{margin:20px 10px;width:unset;}}</style>\n" //отключить таблицу при ширине экрана меньше 1050px
          )
         );
}
