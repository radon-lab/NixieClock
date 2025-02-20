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
void GP_PASS_EYE(const String& name, const String& place = "", const String& value = "", int maxlength = 0, const String& pattern = "", bool dis = false) {
  *_GPP += F("<div class='inlBlock'><input type='password' name='");
  *_GPP += name;
  *_GPP += F("' id='");
  *_GPP += name;
  *_GPP += F("' value='");
  *_GPP += value;
  *_GPP += F("' style='width:90.1%;max-width:214px;min-width:180px;padding:3px 35px;' placeholder='");
  *_GPP += place;
  *_GPP += F("' onchange='GP_click(this)'");
  if (dis) *_GPP += F(" disabled");
  if (maxlength) {
    *_GPP += F(" maxlength=");
    *_GPP += maxlength;
  }
  if (pattern.length()) {
    *_GPP += F(" pattern=");
    *_GPP += pattern;
  }
  *_GPP += ">\n";
  *_GPP += F("<span class='eyepass' style='margin-top:13px;' onclick='GP_eye(this)'>"
             "<svg viewBox='0 0 20 20' style='width:25px;height:25px;fill:currentcolor;'>"
             "<path fill-rule='evenodd' d='M0 20V0zM20 0v20zm-7.204 6.143C11.889 5.734 10.953 5.5 10 5.5c-2.632 0-5.165 1.59-7.204 4.5.924 1.319 1.953 2.35 3.044 "
             "3.1l1.583-1.584A2.993 2.993 0 0 1 10 7c.526 0 1.048.148 1.516.423zm4.381-2.259-4.6 4.6A2.993 2.993 0 0 1 10 13a3 3 0 0 1-1.516-.423l-4.6 4.6a.5.5 0 0 "
             "1-.707 0l-.354-.354a.5.5 0 0 1 0-.707l1.943-1.942c-1.221-.882-2.373-2.082-3.405-3.616a1.01 1.01 0 0 1-.001-1.116C3.796 5.814 6.898 4 10 4c1.327 0 "
             "2.651.346 3.924 1.016l2.192-2.193a.5.5 0 0 1 .707 0l.354.354a.5.5 0 0 1 0 .707m-6.116 5.055A1.5 1.5 0 0 0 10 8.5a1.502 1.502 0 0 0-1.061 2.561zM10 16a8.2 8.2 0 0 "
             "1-2.44-.378l1.244-1.245c.396.074.794.123 1.196.123 2.632 0 5.165-1.59 7.204-4.5a13.6 13.6 0 0 0-1.867-2.155l1.053-1.053a15.5 15.5 0 0 1 2.251 2.653c.223.331.222.78 0 "
             "1.111C16.205 14.185 13.103 16 10 16'></path></svg></span>");
  *_GPP += F("</div>\n");
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
void GP_MIDDLE_BLOCK_BEGIN() {
  GP.SEND(F("<div style='height:90vh;display:flex;align-items:center'>\n<style>.block{width:500px;}</style>\n"));
}
void GP_CENTER_BOX_BEGIN() {
  GP.SEND(F("<div style='width:100%;justify-content:center;margin-top:8px;margin-bottom:-6px' class='inliner'>\n"));
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
            "select{min-width:130px;width:200px;cursor:pointer;}\n" //фикс выпадающего списка
            "html{scrollbar-width:none;}\n" //фикс прокрутки страницы
            "#ubtn {min-width:34px;border-radius:25px;line-height:160%;}\n" //фикс кнопок загрузки
            "#grid .block{margin:15px 10px;}</style>\n" //фикс таблицы
            "<style type='text/css'>:root{color-scheme:light dark;}@media screen and (max-width:1000px){.page{margin-top:43px;}}</style>\n" //фикс темы по умолчанию
            "<style type='text/css'>@media screen and (max-width:1100px){\n.grid{display:block;}\n#grid .block{margin:20px 10px;width:unset;}}</style>\n" //отключить таблицу при ширине экрана меньше 1050px
          )
         );
}
