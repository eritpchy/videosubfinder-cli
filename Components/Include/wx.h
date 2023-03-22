#pragma once
#include <string>
#include <string.h>

#define wxMessageBox1(message) do { \
    wxString content = message; \
    printf(content.c_str()); \
} while(0)

#define wxMessageBox(message, title) do { \
    wxString content = wxString(title) + " " + wxString(message) + "\n"; \
    printf(content.c_str()); \
} while(0)

using namespace std;
