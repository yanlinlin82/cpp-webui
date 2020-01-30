#include "webui.h"

int main()
{
    webui::window w("CPP-WebUI Demo");
    webui::textbox t;
    webui::button b("Click Me!");
	b.set_event("click", [&]{ t.set_text("Hello, CPP-WebUI!"); });
    w += t;
	w += b;
    return webui::run(w, 1234);
}
