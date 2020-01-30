# CPP-WebUI - C++ Graphical User Interface Library

## Motivation

This library provide a framework for cross-platform GUI (Graphical User Interface) program development by C++. The essential idea is to run as web service, and use web page front technology to improve the user experience. Even in a desktop program, we use the same http server/client model.

## Quick Start

This library is published as single header. You can easily start to use the library by includeing it:

```cpp
#include <cpp-webui.h>
```

After that, define the UI elements and actions in C++ code:

```cpp
int main()
{
    webui::window w("CPP-WebUI Demo");
    webui::textbox t("Hello!");
    webui::button b("Demo");
    b.onclick = [] { t.set_text("Hello, CPP-WebUI!"); }
    w += t + b;
    return webui::run();
}
```

Build and run it:

```sh
$ g++ demo.cpp && ./a.out
```

and then open <http://localhost:1234/> in web browser.
