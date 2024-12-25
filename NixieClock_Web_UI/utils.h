void GP_PAGE_TITLE(const String& name) {
  GP.PAGE_TITLE(((settings.namePrefix) ? (settings.name + String(" - ")) : "") + name + ((settings.namePostfix) ? (String(" - ") + settings.name) : ""));
}
void GP_LABEL_BLOCK_W(const String& val, const String& name = "", PGM_P st = GP_GREEN, int size = 0, bool bold = 0) {
  GP.TAG_RAW(F("label class='display'"), val, name, GP_WHITE, size, bold, 0, st);
}
void GP_FLOAT_DEC(float val, uint16_t dec) {
  if (!dec) *_GPP += (int)round(val);
  else *_GPP += String(val, (uint16_t)dec);
}
void GP_SLIDER_MAX(const String& lable, const String& min_lable, const String& max_lable, const String& name, float value = 0, float min = 0, float max = 100, float step = 1, uint8_t dec = 0, PGM_P st = GP_GREEN, bool dis = 0, bool oninp = 0) {
  *_GPP += F("<lable style='color:#fff;position:relative;z-index:1;left:17px;bottom:1px;width:0px;pointer-events:none'");
  if (dis) *_GPP += F(" class='dsbl'");
  *_GPP += '>';
  *_GPP += lable;
  *_GPP += F("</lable>\n<input type='range' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += F("' value='");
  *_GPP += value;
  *_GPP += F("' min='");
  *_GPP += min;
  *_GPP += F("' max='");
  *_GPP += max;
  *_GPP += F("' step='");
  *_GPP += step;
  *_GPP += F("' style='background-image:linear-gradient(");
  *_GPP += FPSTR(st);
  *_GPP += ',';
  *_GPP += FPSTR(st);
  *_GPP += F(");background-size:0% 100%;height:30px;width:100%;max-width:430px;margin:10px 4px;border-radius:20px;box-shadow:0 0 15px rgba(0, 0, 0, 0.7)' onload='GP_change(this)' ");
  if (oninp) *_GPP += F("oninput='GP_change(this);GP_click(this)'");
  else *_GPP += F("onchange='GP_click(this)' oninput='GP_change(this)'");
  *_GPP += F(" onmousewheel='GP_wheel(this);GP_change(this);GP_click(this)' ");
  if (dis) *_GPP += F("class='dsbl' disabled");
  *_GPP += F(">\n<output align='center' id='");
  *_GPP += name;
  *_GPP += F("_val' name='");
  *_GPP += min_lable;
  *_GPP += ',';
  *_GPP += max_lable;
  *_GPP += F("' style='position:relative;right:70px;margin-right:-55px;background:none;display:inline-flex;justify-content:end;pointer-events:none'");
  if (dis) *_GPP += F(" class='dsbl'");
  *_GPP += F(">");
  GP_FLOAT_DEC(value, dec);
  *_GPP += F("</output>\n");
  GP.send();
}
void GP_SUBMIT(const String& text, PGM_P st = GP_GREEN, const String& cls = "") {
  *_GPP += F("<input type='submit' value='");
  *_GPP += text;
  *_GPP += F("' style='background:");
  *_GPP += FPSTR(st);
  *_GPP += F(";margin-top:10px;margin-bottom:0");
  if (cls.length()) {
    *_GPP += F("' class='");
    *_GPP += cls;
  }
  *_GPP += F("'>\n");
  GP.send();
}
void GP_SPINNER_BTN(const String& name, float step, PGM_P st, uint8_t dec, bool dis) {
  *_GPP += F("<input type='button' class='spinBtn ");
  *_GPP += (step > 0) ? F("spinR") : F("spinL");
  *_GPP += F("' name='");
  *_GPP += name;
  *_GPP += F("' min='");
  *_GPP += step;
  *_GPP += F("' max='");
  *_GPP += dec;
  *_GPP += F("' onmouseleave='if(_pressId)clearInterval(_spinInt);_spinF=_pressId=null' onmousedown='_pressId=this.name;_spinInt=setInterval(()=>{GP_spin(this);_spinF=1},");
  *_GPP += 200;
  *_GPP += F(")' onmouseup='clearInterval(_spinInt)' onclick='if(!_spinF)GP_spin(this);_spinF=0' value='");
  *_GPP += (step > 0) ? '+' : '-';
  *_GPP += F("' ");
  *_GPP += F(" style='background:");
  *_GPP += FPSTR(st);
  *_GPP += F(";'");
  if (dis) *_GPP += F(" disabled");
  *_GPP += F(">\n");
}
void GP_SPINNER_MAIN(const String& name, float value = 0, float min = NAN, float max = NAN, float step = 1, uint16_t dec = 0, PGM_P st = GP_GREEN, const String& w = "", bool dis = 0) {
  *_GPP += F("<div id='spinner' class='spinner'>\n");
  GP_SPINNER_BTN(name, -step, st, dec, dis);
  *_GPP += F("<input type='number' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  if (w.length()) {
    *_GPP += F("' style='width:");
    *_GPP += w;
  }
  *_GPP += F("' step='");
  GP_FLOAT_DEC(step, dec);
  *_GPP += F("' onkeyup='GP_spinw(this)' onkeydown='GP_spinw(this)' onchange='");
  if (!dec) *_GPP += F("GP_spinc(this);");
  *_GPP += F("GP_click(this);GP_spinw(this)' value='");
  GP_FLOAT_DEC(value, dec);
  if (!isnan(min)) {
    *_GPP += F("' min='");
    GP_FLOAT_DEC(min, dec);
  }
  if (!isnan(max)) {
    *_GPP += F("' max='");
    GP_FLOAT_DEC(max, dec);
  }
  *_GPP += F("' ");
  if (dis) *_GPP += F("disabled ");
  if (!w.length()) *_GPP += F("class='spin_inp'");
  *_GPP += F(">\n");
  GP_SPINNER_BTN(name, step, st, dec, dis);
  *_GPP += F("</div>\n");
  GP.send();
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
  *_GPP += F("<style>a:link.");
  *_GPP += id;
  *_GPP += F("_link{color:");
  *_GPP += FPSTR(color);
  *_GPP += F(";text-decoration:none;} a:visited.");
  *_GPP += id;
  *_GPP += F("_link{color:");
  *_GPP += FPSTR(color);
  *_GPP += F(";} a:hover.");
  *_GPP += id;
  *_GPP += F("_link{filter:brightness(0.75);}</style>\n<a href='");
  *_GPP += url;
  *_GPP += F("' class='");
  *_GPP += id + "_link";
  *_GPP += F("'>");
  *_GPP += text;
  *_GPP += F("</a>\n");
  GP.send();
}
void GP_UI_LINK(const String& url, const String& name, PGM_P color) {
  *_GPP += F("<a href='http://");
  *_GPP += url;
  *_GPP += "'";
  if (WiFi.localIP().toString().equals(url)) {
    *_GPP += F(" class='sbsel' style='background:");
    *_GPP += FPSTR(color);
    *_GPP += F(" !important;'");
  }
  *_GPP += ">";
  *_GPP += name;
  *_GPP += F("</a>\n");
  GP.send();
}
void GP_CHECK_ICON(const String& name, const String& uri, bool state = 0, int size = 30, PGM_P st_0 = GP_GRAY, PGM_P st_1 = GP_GREEN, bool dis = false) {
  *_GPP += F("<style>#__");
  *_GPP += name;
  *_GPP += F(" span::before{background-color:");
  *_GPP += FPSTR(st_0);
  *_GPP += F(";border:none;padding:");
  *_GPP += (size / 2) + 2;
  *_GPP += F("px;}</style>\n");

  *_GPP += F("<style>#__");
  *_GPP += name;
  *_GPP += F(" input:checked+span::before{background-color:");
  *_GPP += FPSTR(st_1);
  *_GPP += F(";background-image:none;}</style>\n");

  *_GPP += F("<label id='__");
  *_GPP += name;
  *_GPP += F("' class='check_c'");
  *_GPP += F(" style='-webkit-mask:center/contain no-repeat url(");
  *_GPP += uri;
  *_GPP += F(");display:inline-block;width:");
  *_GPP += size;
  *_GPP += F("px;'>");
  *_GPP += F("<input type='checkbox' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += "' ";
  if (state) *_GPP += F("checked ");
  if (dis) *_GPP += F("disabled ");
  *_GPP += F("onclick='GP_click(this)' style='height:");
  *_GPP += size;
  *_GPP += F("px;'><span></span></label>\n"
             "<input type='hidden' value='0' name='");
  *_GPP += name;
  *_GPP += F("'>\n");
  GP.send();
}
void GP_HR(PGM_P st, int width = 0) {
  *_GPP += F("<hr style='border-color:");
  *_GPP += FPSTR(st);
  *_GPP += F(";margin:5px ");
  *_GPP += width;
  *_GPP += F("px'>\n");
  GP.send();
}
void GP_HR_TEXT(const String& text, const String& name, PGM_P st_0, PGM_P st_1) {
  *_GPP += F("<label id='");
  *_GPP += name;
  *_GPP += F("' class='thinText' style='color:");
  *_GPP += FPSTR(st_0);
  *_GPP += F("'>");
  *_GPP += text;
  *_GPP += F("</label>\n");
  *_GPP += F("<hr style='border-color:");
  *_GPP += FPSTR(st_1);
  *_GPP += F(";margin-top:-17px;padding-bottom:17px'>\n");
  GP.send();
}
void GP_LINE_LED(const String& name, bool state = 0, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  *_GPP += F("<style>#__");
  *_GPP += name;
  *_GPP += F(" input:checked+span::before{background-color:");
  *_GPP += FPSTR(st_1);
  *_GPP += F(";background-image:none}\n");

  *_GPP += F("#__");
  *_GPP += name;
  *_GPP += F(" span::before{background-color:");
  *_GPP += FPSTR(st_0);
  *_GPP += F(";border:none;display:inline-block;width:100px;height:0px;cursor:default;filter:brightness(1)!important;box-shadow:0 0 15px rgba(0, 0, 0, 0.7)}\n");

  *_GPP += F("#__");
  *_GPP += name;
  *_GPP += F(" input[type=checkbox]{cursor:default;margin-left:0px}</style>\n");

  *_GPP += F("<label id='__");
  *_GPP += name;
  *_GPP += F("' class='check_c' style='display:block;height:30px;margin-top:-13px;cursor:default'><input type='checkbox' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += "' ";
  if (state) *_GPP += F("checked ");
  *_GPP += F("disabled ");
  *_GPP += F("onclick='GP_click(this)'><span></span></label>\n"
             "<input type='hidden' value='0' name='");
  *_GPP += name;
  *_GPP += F("'>\n");
  GP.send();
}
void GP_LINE_BAR(const String& name, int value = 0, int min = 0, int max = 100, int step = 1, PGM_P st = GP_GREEN) {
  *_GPP += F("<input type='range' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += F("' value='");
  *_GPP += value;
  *_GPP += F("' min='");
  *_GPP += min;
  *_GPP += F("' max='");
  *_GPP += max;
  *_GPP += F("' step='");
  *_GPP += step;
  *_GPP += F("' style='filter:brightness(1);box-shadow:0 0 15px rgba(0, 0, 0, 0.7);background-color:#1a1a1a;background-image:linear-gradient(");
  *_GPP += FPSTR(st);
  *_GPP += ',';
  *_GPP += FPSTR(st);
  *_GPP += F(");background-size:0% 100%;display:block;width:124px;height:8px;margin-top:3px;margin-bottom:6px;cursor:default' onload='GP_change(this)' disabled>\n");

  *_GPP += F("<output style='display:none' id='");
  *_GPP += name;
  *_GPP += F("_val'></output>\n");
  GP.send();
}
void GP_PLOT_STOCK_BEGIN(boolean local = 0) {
  if (local) *_GPP += F("<script src='/gp_data/PLOT_STOCK.js'></script>\n<script src='/gp_data/PLOT_STOCK_DARK.js'></script>\n");
  else *_GPP += F("<script src='https://code.highcharts.com/stock/highstock.js'></script>\n<script src='https://code.highcharts.com/themes/dark-unica.js'></script>\n");
  *_GPP += F("<script src='https://code.highcharts.com/modules/exporting.js'></script>\n");

  GP.send();
}
void GP_PLOT_STOCK_ADD(uint32_t time, int16_t val, uint8_t dec) {
  *_GPP += '[';
  *_GPP += time;
  *_GPP += F("000");
  *_GPP += ',';
  if (dec) *_GPP += (float)val / dec;
  else *_GPP += val;
  *_GPP += F("],\n");
  GP.send();
}
void GP_PLOT_STOCK_DARK(const String& id, const char** labels, uint32_t* times, int16_t* vals_0, int16_t* vals_1, uint8_t size, uint8_t type = 0, uint8_t dec = 0, uint16_t height = 400, PGM_P st_0 = GP_RED, PGM_P st_1 = GP_GREEN) {
  *_GPP += F("<div class='chartBlock' style='width:95%;height:");
  *_GPP += height;
  *_GPP += F("px' id='");
  *_GPP += id;
  *_GPP += F("'></div>");

  *_GPP += F("<script>Highcharts.setOptions({"
             "lang:{contextButtonTitle:'Меню',viewFullscreen:'Во весь экран',exitFullscreen:'Свернуть',"
             "printChart:'Печать...',resetZoom:'Сбросить',resetZoomTitle:'Сбросить маштаб'},\n"
             "global:{buttonTheme:{height:12,fill:'#505053',stroke:'#505053',style:{color:'#DDDDDD'},"
             "states:{hover:{fill:'#737373'},select:{fill:'#505053'}}}},\ncolors:['"
            );

  *_GPP += FPSTR(st_0);
  *_GPP += F("','");
  *_GPP += FPSTR(st_1);
  *_GPP += F("']});\nHighcharts.chart('");
  *_GPP += id;
  *_GPP += F("',{chart:{type:'spline',borderRadius:10,panning:true,panKey:'shift',\n"
             "zooming:{type:'x',mouseWheel:false,resetButton:{position:{align:'left',verticalAlign:'top',x:43,y:9},relativeTo:'chart'}}},\n"

             "title:{style:{color:'#00000000'}},\n"
             "rangeSelector:'none',\n"

             "plotOptions:{series:{marker:{enabled:false},lineWidth:3.5}},\n"

             "tooltip:{crosshairs:true,shared:true},\n"
             "legend:{floating:true,backgroundColor:undefined,align:'right',verticalAlign:'top'},\n"

             "exporting:{buttons:{contextButton:{align:'left',x:0,y:-1,menuItems:['printChart','viewFullscreen']}}},\n"
             "navigation:{menuStyle:{background:'#E0E0E0'},menuItemStyle:{color:'#000000'}},\n"

             "time:{useUTC:true},\n"

             "xAxis:{type:'datetime'},\n"
             "yAxis:[{title:{enabled:false},tickLength:0,\n"
             "labels:{align:'left',x:3,y:16},\n"
             "showFirstLabel:false},\n"
             "{title:{enabled:false},tickLength:0,\n"
             "labels:{align:'right',x:-3,y:16},\n"
             "showFirstLabel:false,opposite:true}],\n"

             "credits:{enabled:false},series:[\n"
            );

  if (vals_0 != NULL) {
    *_GPP += F("{name:'");
    *_GPP += labels[0];
    *_GPP += F("',data:[\n");
    GP.send();
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_0[s], dec);
    }
    *_GPP += F("],\n");
    if (type == 1) *_GPP += F("tooltip:{valueSuffix:'°C'}");
    else if (type == 2) *_GPP += F("tooltip:{valueSuffix:'mm.Hg'}");
    *_GPP += F("},\n");
  }
  if (vals_1 != NULL) {
    *_GPP += F("{name:'");
    *_GPP += labels[1];
    *_GPP += F("',data:[\n");
    GP.send();
    if (type == 1) dec = 0;
    for (uint16_t s = 0; s < size; s++) {
      GP_PLOT_STOCK_ADD(times[s], vals_1[s], dec);
    }
    *_GPP += F("],\n");
    if (type == 1) *_GPP += F("tooltip:{valueSuffix:'%'},\n");
    *_GPP += F("marker:{symbol:'circle'},yAxis:1},\n");
  }
  *_GPP += F("]});</script>\n");
  GP.send();
}
void GP_NAV_TABS_M(const String& name, const String& list, int disp) {
  *_GPP += F("<div class='navtab'><ul>\n");
  GP_parser tab(list);
  while (tab.parse()) {
    *_GPP += F("<li ");
    if (tab.count == disp) *_GPP += F("style='background:#2a2d35' ");
    *_GPP += F("' class='");
    *_GPP += name;
    *_GPP += F("' onclick='openTab(\"");
    *_GPP += name;
    *_GPP += '/';
    *_GPP += tab.count;
    *_GPP += F("\",this,\"block_");
    *_GPP += name;
    *_GPP += F("\");GP_send(\"/GP_click?");
    *_GPP += name;
    *_GPP += '/';
    *_GPP += tab.count;
    *_GPP += F("=\");'>");
    *_GPP += tab.str;
    *_GPP += F("</li>\n");
  }
  *_GPP += F("</ul></div>\n");
  GP.send();
}
void GP_NAV_BLOCK_BEGIN(const String& name, int pos, int disp) {
  *_GPP += F("<div class='navblock block_");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += '/';
  *_GPP += pos;
  *_GPP += "' ";
  if (pos == disp) *_GPP += F("style='display:block'");
  *_GPP += F(">\n");
  GP.send();
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
            ".sidebar{scrollbar-width:none;}\n" //фикс меню
            ".sblock{display:flex;flex-direction:column;min-height:98%;margin:0;}\n" //фикс меню
            ".sblock>a{border-radius:25px;}\n" //фикс кнопок меню
            ".spinBtn{font-size:24px!important;padding-left:3.5px;padding-top:0.5px;}\n" //фикс кнопок спинера
            ".check_c>span::before{border-color:#444;background-color:#2a2d35;}\n" //фикс чекбоксов
            ".check_c>input:checked+span::before{border-color:#e67b09;background-color:#e67b09;}\n" //фикс чекбоксов
            ".navtab>ul{background:none!important;margin-bottom:5px;}\n" //фикс блока меню
            ".navtab>ul>li{display:block;border-radius:25px;margin:5px;width:40%;box-shadow:rgba(0, 0, 0, 0.45) 0px 0px 15px;}\n" //фикс кнопки блока меню
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
            "html{scrollbar-width:none;}\n" //фикс прокрутки страницы
            "#ubtn {min-width:34px;border-radius:25px;line-height:160%;}\n" //фикс кнопок загрузки
            "#grid .block{margin:15px 10px;}</style>\n" //фикс таблицы
            "<style type='text/css'>@media screen and (max-width:1100px){\n.grid{display:block;}\n#grid .block{margin:20px 10px;width:unset;}}</style>\n" //отключить таблицу при ширине экрана меньше 1050px
          )
         );
}
